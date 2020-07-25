// An address ABOVE 0xC0000000 is shared between ALL processes and is used by the kernel.
// An address BELOW 0xC0000000 is only used by the CURRENT process, and may include NON-USER pages, such as the KERNEL STACK

#include <memory.h>
#include "kernel.h"
#include "elf.h"
#include "error.h"
#include "fat.h"

extern "C" {

static void* getpage(unsigned* pgdir, void* vaddr, int, int);
static void* pmapmem(unsigned* pgdir, void* vaddr, unsigned size, int, int);

static const char* elf_error; // FIXME: FIX FOR MULTITHREADING
static void* driver_end = (void*)0xE0000000; // FIXME: FIX FOR MULTITHREADING

void* vm8086_gpfault;

unsigned long elf_hash(const unsigned char *name)
{
    unsigned long h = 0, g;
    while (*name)
    {
        h = (h << 4) + *name++;
        if (g = h & 0xf0000000)
            h ^= g >> 24;
        h &= ~g;
    }
    return h;
}

Elf32_Sym *find_symbol(const char* name)
{
    // Use the ELF hash table to find the symbol.
    int i = kernel_bucket[elf_hash((const unsigned char*)name)%kernel_nbucket];
    while (i != SHN_UNDEF)
    {
        if (kstrcmp(kernel_strtab + kernel_symtab[i].st_name, name) == 0)
            return kernel_symtab + i;
        i = kernel_chain[i];
    }
    return 0;
}
// Creates and maps memory of the given size at the given address
// and maps it into this process, which must be unmapped.
static void* pmapmem(unsigned* pgdir, void* vaddr, unsigned size, int writeable, int user)
{
    void* addr = kfindrange(size);
    unsigned t;
    for (t = 0; t < size; t += 4096)
        page_map((char*)addr+t, getpage(pgdir, (char*)vaddr+t, writeable, user), PF_WRITE);
    return addr;
}

// Given the logical address of the page directory,
// and the desired logical address, this function ensures
// that the page exists in memory, and returns its physical address
static void* getpage(unsigned* pgdir, void* vaddr, int writeable, int user)
{
    // Offset into the page directory
    unsigned pgdir_off = ((unsigned)vaddr)>>22;
    // Offset into the page table
    unsigned pgtbl_off = (((unsigned)vaddr)>>12)&0x3FF;
    // Get or allocate physical address of page table
    void* pgtbl_p;
    if (pgdir[pgdir_off]&1)
    {
        pgtbl_p = (void*)(pgdir[pgdir_off]&~0xFFF);
    }
    else
    {
        pgtbl_p = page_alloc(1);
        pgdir[pgdir_off] = (unsigned)pgtbl_p+(PF_PRESENT|PF_WRITE|PF_USER);
    }
    // Map in page table in kernel space
    unsigned* pgtbl = (unsigned*)kfindrange(0x1000);
    page_map(pgtbl,pgtbl_p,PF_WRITE);
    // Allocate the page and map it in user space
    void* pg_p;
    if (pgtbl[pgtbl_off] & 1)
    {
        pg_p = (void*)(pgtbl[pgtbl_off]&0xFFFFF000);
    }
    else
    {
        pg_p = page_alloc(1);
        pgtbl[pgtbl_off] = (((unsigned)pg_p)&~0xFFF)+(PF_PRESENT|(writeable ? PF_WRITE : 0)|(user ? PF_USER : 0));
    }
    // unmap page table
    page_unmap(pgtbl);
    // return the page
    return pg_p;
}

const char* elf_last_error()
{
    return elf_error;
}

} // extern "C"

int process_start(std::shared_ptr<kernel::FileSystemDriver> fs_driver, const char* filename)
{
    elf_error = "";
    Elf32_Ehdr ehdr;
    int i;
    std::unique_ptr<kernel::File> elf;
    try
    {
        elf = fs_driver->file_open(filename);
    }
    catch (const kernel::NotFoundError&)
    {
        elf_error = "Could not locate executable.";
        return 0;
    }
    if ((elf->read((char*)(void*)&ehdr, sizeof(ehdr)) < sizeof(ehdr)) ||
        (ehdr.e_ident[EI_MAG0] != ELFMAG0) || 
        (ehdr.e_ident[EI_MAG1] != ELFMAG1) || 
        (ehdr.e_ident[EI_MAG2] != ELFMAG2) || 
        (ehdr.e_ident[EI_MAG3] != ELFMAG3))
    {
        elf_error = "Invalid ELF header.";
        return 0;
    }
    if (ehdr.e_version != EV_CURRENT || ehdr.e_ident[EI_VERSION] != EV_CURRENT)
    {
        elf_error = "Unsupported ELF version.";
        return 0;
    }
    if (ehdr.e_type != ET_EXEC)
    {
        elf_error = "ELF is not an executable.";
        return 0;
    }
    if (ehdr.e_machine != K_ELF_MACHINE)
    {
        elf_error = "Unsupported ELF machine.";
        return 0;
    }
    if (ehdr.e_ident[EI_CLASS] != K_ELF_CLASS)
    {
        elf_error = "Unsupported ELF class.";
        return 0;
    }
    if (ehdr.e_ident[EI_DATA] != K_ELF_DATA)
    {
        elf_error = "Unsupported ELF data encoding.";
        return 0;
    }
    // Allocate a page directory for the process
    void *pgdir_p = page_alloc(1);
    unsigned *pgdir = (unsigned*)kfindrange(4096); // Find some address space to use to access the process' page directory
    page_map(pgdir, pgdir_p, PF_WRITE);

    // Wipe lower page directory
    for (i = 0; i < 768; i++) pgdir[i] = 0;

    // Map the kernel into process
    for (i = 768; i < 1023; i++) pgdir[i] = system_pdt[i];

    // Map in the PDT
    pgdir[1023] = ((unsigned)pgdir_p)+(PF_PRESENT|PF_WRITE);

    // DPL0 stack is 0xBFFFF000-0xC0000000
    // DPL3 stack is 0xBFFFE000-0xBFFFF000

    // Create a DPL3 stack
    getpage(pgdir, (void*)0xBFFFE000, 1, 1);

    // Set up the process's DPL0 stack
    unsigned* kstack = (unsigned*)pmapmem(pgdir, (void*)0xBFFFF000, 0x1000, 1, 0)+0x1000;
    *--kstack = 0x43;                   // SS
    *--kstack = 0xBFFFF000;             // ESP initial value
    *--kstack = 0x0202;                 // EFLAGS
    *--kstack = 0x3B;                   // CS; 0x38 is DPL3 code. (0x3B: RPL=3)
    *--kstack = (unsigned)ehdr.e_entry; // EIP; set to entry point for initial value
    *--kstack = 0;                      // These values are POPAD'd
    *--kstack = 0;
    *--kstack = 0;
    *--kstack = 0;
    *--kstack = 0;
    *--kstack = 0;
    *--kstack = 0;
    *--kstack = 0;
    *--kstack = 0x43;                   // Segment selectors for DS, ES, FS, & GS. 0x40 is DPL3 data. (0x43: RPL=3)
    *--kstack = 0x43;
    *--kstack = 0x43;
    *--kstack = 0x43;
    page_unmap(kstack);
    
    // load program segments
    for (i = 0; i < ehdr.e_phnum; i++)
    {
        Elf32_Phdr phdr;
        elf->seek(ehdr.e_phoff+ehdr.e_phentsize*i);
        elf->read((char*)(void*)&phdr, sizeof(phdr));
        if (phdr.p_type == PT_LOAD)
        {
            elf->seek(phdr.p_offset);
            char* pmap = (char*)pmapmem(pgdir, phdr.p_vaddr, phdr.p_memsz, phdr.p_flags&PF_W, 1);
            elf->read((char*)pmap, phdr.p_filesz);
            char* t;
            for (t = pmap + phdr.p_filesz; t < pmap + phdr.p_memsz; t++)
                *t = 0;
            for (t = pmap; t < pmap + phdr.p_memsz; t += 0x1000);
                page_unmap(t);
        }
    }
    elf->close();
    page_unmap(pgdir);
    // Create the process and schedule it for execution
    Process* p = process_create(filename);
    p->cr3 = (unsigned)pgdir_p;
    p->esp = 0xBFFFFFBC;
    p->priority = 3;
    p->vm8086 = 0;
    p->fpu_saved = 0;
    process_enqueue(p);
    return 1;
}

int load_driver(std::shared_ptr<kernel::FileSystemDriver> fs_driver, const char* filename)
{
    int ret = 0;
    elf_error = "";
    std::unique_ptr<kernel::File> elf;
    Elf32_Ehdr ehdr;
    int i;
    try
    {
        elf = fs_driver->file_open(filename);
    }
    catch (const kernel::NotFoundError&)
    {
        elf_error = "Could not locate executable.";
        return 0;
    }
    if ((elf->read((char*)(void*)&ehdr, sizeof(ehdr)) < sizeof(ehdr)) ||
        (ehdr.e_ident[EI_MAG0] != ELFMAG0) || 
        (ehdr.e_ident[EI_MAG1] != ELFMAG1) || 
        (ehdr.e_ident[EI_MAG2] != ELFMAG2) || 
        (ehdr.e_ident[EI_MAG3] != ELFMAG3))
    {
        elf_error = "Invalid ELF header.";
        return 0;
    }
    if (ehdr.e_version != EV_CURRENT || ehdr.e_ident[EI_VERSION] != EV_CURRENT)
    {
        elf_error = "Unsupported ELF version.";
        return 0;
    }
    if (ehdr.e_type != ET_REL)
    {
        elf_error = "ELF is not a valid driver.";
        return 0;
    }
    if (ehdr.e_machine != K_ELF_MACHINE)
    {
        elf_error = "Unsupported ELF machine.";
        return 0;
    }
    if (ehdr.e_ident[EI_CLASS] != K_ELF_CLASS)
    {
        elf_error = "Unsupported ELF class.";
        return 0;
    }
    if (ehdr.e_ident[EI_DATA] != K_ELF_DATA)
    {
        elf_error = "Unsupported ELF data encoding.";
        return 0;
    }

    volatile Elf32_Shdr *shdrs = (volatile Elf32_Shdr *)kmalloc(sizeof(Elf32_Shdr)*ehdr.e_shnum);
    elf->seek(ehdr.e_shoff);
    volatile Elf32_Sym *symtab = 0;

    // read section headers
    for (i = 0; i < ehdr.e_shnum; i++)
    {
        elf->seek(ehdr.e_shoff+ehdr.e_shentsize*i);
        elf->read((char*)(void*)&shdrs[i], sizeof(Elf32_Shdr));
    }

    // Read the section header string table
    volatile char *shstrtab = (volatile char *)kmalloc(shdrs[ehdr.e_shstrndx].sh_size);
    elf->seek(shdrs[ehdr.e_shstrndx].sh_offset);
    elf->read((char*)(void*)shstrtab, shdrs[ehdr.e_shstrndx].sh_size);

    int j, cnt;
    volatile Elf32_Shdr *s;
    volatile char *strtab;
    // Load sections and symbol table
    for (i = 1; i < ehdr.e_shnum; i++)
    {
        s = &shdrs[i];
        switch (s->sh_type)
        {
        case SHT_PROGBITS:
            if (s->sh_flags & SHF_ALLOC)
            {
                s->sh_addr = driver_end;
                driver_end = (void*)(((unsigned)driver_end + s->sh_size + 0xFFF) & 0xFFFFF000);
                for (j = 0; j < s->sh_size; j += 4096)
                    page_map((char*)s->sh_addr + j, page_alloc(1), s->sh_flags & SHF_WRITE ? PF_WRITE : PF_NONE);
                elf->seek(s->sh_offset);
                elf->read((char*)s->sh_addr, s->sh_size);
            }
            break;
        case SHT_NOBITS:
            if (s->sh_flags & SHF_ALLOC)
            {
                s->sh_addr = driver_end;
                driver_end = (void*)(((unsigned)driver_end + s->sh_size + 0xFFF) & 0xFFFFF000);
                for (j = 0; j < s->sh_size; j += 4096)
                    page_map((char*)s->sh_addr + j, page_alloc(1), s->sh_flags & SHF_WRITE ? PF_WRITE : PF_NONE);
                kzeromem(s->sh_addr, s->sh_size);
            }
            break;
        case SHT_SYMTAB:
            if (kstrcmp(".symtab", (const char*)shstrtab+s->sh_name) != 0) break;
            cnt = s->sh_size/s->sh_entsize;
            symtab = (volatile Elf32_Sym*)kmalloc(cnt*sizeof(Elf32_Sym));
            for (j = 0; j < cnt; j++)
            {
                elf->seek(s->sh_offset+s->sh_entsize*j);
                elf->read((char*)(void*)&symtab[j], sizeof(Elf32_Sym));
            }
    
            // Read the linked string table
            strtab = (volatile char*)kmalloc(shdrs[s->sh_link].sh_size);
            elf->seek(shdrs[s->sh_link].sh_offset);
            elf->read((char*)(void*)strtab, shdrs[s->sh_link].sh_size);
            break;
        }
    }

    volatile void* drivermain = 0;
    static char t[1024];
    // Update symbols
    for (i = 1; i < cnt; i++)
    {
        if (symtab[i].st_shndx == SHN_UNDEF)
        {
            Elf32_Sym *ksym = find_symbol((const char*)strtab + symtab[i].st_name);
            if (ksym)
            {
                symtab[i].st_shndx = SHN_ABS;
                symtab[i].st_value = ksym->st_value;
            }
            else if (ELF32_ST_BIND(ksym->st_info) != STB_WEAK)
            {
                ksprintf(t, "Undefined symbol '%s'", strtab+symtab[i].st_name);
                elf_error = (const char*)t;
                goto err1;
            }
        }
        else if (symtab[i].st_shndx < SHN_LORESERVE)
        {
            symtab[i].st_value = (char*) symtab[i].st_value + (unsigned)shdrs[symtab[i].st_shndx].sh_addr;
            if (!kstrcmp("driver_main", (const char*)strtab+symtab[i].st_name)) drivermain = symtab[i].st_value;
        }
    }

    if (!drivermain)
    {
        elf_error = "Could not find entry driver_main.";
        goto err1;
    }

    // Perform relocations
    for (i = 1; i < ehdr.e_shnum; i++)
    {
        s = &shdrs[i];
        if (s->sh_type == SHT_REL)
        {
            // Avoid relocating debug symbols if we didn't load that section
            int loaded = 0;
            switch (shdrs[s->sh_info].sh_type)
            {
                case SHT_PROGBITS:
                case SHT_NOBITS:
                    if (shdrs[s->sh_info].sh_flags & SHF_ALLOC)
                    {
                        loaded = 1;
                    }
                    break;
            }
            if (loaded)
            {
                int count = s->sh_size/s->sh_entsize;
                for (j = 0; j < count; j++)
                {
                    Elf32_Rel rel;
                    elf->seek(s->sh_offset+j*s->sh_entsize);
                    elf->read((char*)(void*)&rel, sizeof(Elf32_Rel));
                    unsigned soff = (unsigned)shdrs[s->sh_info].sh_addr;
                    volatile unsigned* ptr = (unsigned*)(void*)((char*)rel.r_offset + soff);
                    Elf32_Sym sym = const_cast<Elf32_Sym*>(symtab)[ELF32_R_SYM(rel.r_info)];
                    unsigned symval = (unsigned)sym.st_value;
                    switch (ELF32_R_TYPE(rel.r_info))
                    {
                    case R_386_32:
                        *ptr += symval;
                        break;
                    case R_386_PC32:
                        *ptr += symval - (unsigned)ptr;
                        break;
                    }
                }
            }
        }
    }

    int retval;
    asm volatile("call *%1":"=a"(retval):"g"(drivermain));
    if (retval == 0)
        ret = 1;
    else
    {
        ksprintf(t, "Driver exited with status %l.", retval);
        elf_error = (const char*)t;
    }
err1:
    kfree((void*)strtab);
    kfree((void*)symtab);
    kfree((void*)shdrs);
    return ret;
}

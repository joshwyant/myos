// An address ABOVE 0xC0000000 is shared between ALL processes and is used by the kernel.
// An address BELOW 0xC0000000 is only used by the CURRENT process, and may include NON-USER pages, such as the KERNEL STACK

#include "kernel.h"
#include "elf.h"
#include "fat.h"

static void* getpage(unsigned* pgdir, void* vaddr, int, int);
static void* pmapmem(unsigned* pgdir, void* vaddr, unsigned size, int, int);

static char* elf_error; // FIXME: FIX FOR MULTITHREADING
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

int process_start(const char* filename)
{
    elf_error = "";
    FileStream elf;
    Elf32_Ehdr ehdr;
    int i;
    if (!file_open(filename, &elf))
    {
        elf_error = "Could not locate executable.";
        return 0;
    }
    if ((file_read(&elf, &ehdr, sizeof(ehdr)) < sizeof(ehdr)) ||
        (ehdr.e_ident[EI_MAG0] != ELFMAG0) || 
        (ehdr.e_ident[EI_MAG1] != ELFMAG1) || 
        (ehdr.e_ident[EI_MAG2] != ELFMAG2) || 
        (ehdr.e_ident[EI_MAG3] != ELFMAG3))
    {
        file_close(&elf);
        elf_error = "Invalid ELF header.";
        return 0;
    }
    if (ehdr.e_version != EV_CURRENT || ehdr.e_ident[EI_VERSION] != EV_CURRENT)
    {
        file_close(&elf);
        elf_error = "Unsupported ELF version.";
        return 0;
    }
    if (ehdr.e_type != ET_EXEC)
    {
        file_close(&elf);
        elf_error = "ELF is not an executable.";
        return 0;
    }
    if (ehdr.e_machine != K_ELF_MACHINE)
    {
        file_close(&elf);
        elf_error = "Unsupported ELF machine.";
        return 0;
    }
    if (ehdr.e_ident[EI_CLASS] != K_ELF_CLASS)
    {
        file_close(&elf);
        elf_error = "Unsupported ELF class.";
        return 0;
    }
    if (ehdr.e_ident[EI_DATA] != K_ELF_DATA)
    {
        file_close(&elf);
        elf_error = "Unsupported ELF data encoding.";
        return 0;
    }
    // Allocate a page directory for the process
    void *pgdir_p = page_alloc(1);
    unsigned *pgdir = kfindrange(4096); // Find some address space to use to access the process' page directory
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
    unsigned* kstack = pmapmem(pgdir, (void*)0xBFFFF000, 0x1000, 1, 0)+0x1000;
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
        file_seek(&elf, ehdr.e_phoff+ehdr.e_phentsize*i);
        file_read(&elf, &phdr, sizeof(phdr));
        if (phdr.p_type == PT_LOAD)
        {
            file_seek(&elf, phdr.p_offset);
            void* pmap = pmapmem(pgdir, phdr.p_vaddr, phdr.p_memsz, phdr.p_flags&PF_W, 1);
            file_read(&elf, pmap, phdr.p_filesz);
            void* t;
            for (t = pmap + phdr.p_filesz; t < pmap + phdr.p_memsz; t++)
                *((char*)t) = 0;
            for (t = pmap; t < pmap + phdr.p_memsz; t += 0x1000);
                page_unmap(t);
        }
    }
    file_close(&elf);
    page_unmap(pgdir);
    // Create the process and schedule it for execution
    Process* p = process_create(filename);
    p->cr3 = (unsigned)pgdir_p;
    p->esp = 0xBFFFFFBC;
    p->priority = 3;
    p->vm8086 = 0;
    process_enqueue(p);
    return 1;
}

int load_driver(const char* filename)
{
    int ret = 0;
    elf_error = "";
    FileStream elf;
    Elf32_Ehdr ehdr;
    int i;
    if (!file_open(filename, &elf))
    {
        elf_error = "Could not locate executable.";
        return 0;
    }
    if ((file_read(&elf, &ehdr, sizeof(ehdr)) < sizeof(ehdr)) ||
        (ehdr.e_ident[EI_MAG0] != ELFMAG0) || 
        (ehdr.e_ident[EI_MAG1] != ELFMAG1) || 
        (ehdr.e_ident[EI_MAG2] != ELFMAG2) || 
        (ehdr.e_ident[EI_MAG3] != ELFMAG3))
    {
        elf_error = "Invalid ELF header.";
        goto err2;
    }
    if (ehdr.e_version != EV_CURRENT || ehdr.e_ident[EI_VERSION] != EV_CURRENT)
    {
        elf_error = "Unsupported ELF version.";
        goto err2;
    }
    if (ehdr.e_type != ET_REL)
    {
        elf_error = "ELF is not a valid driver.";
        goto err2;
    }
    if (ehdr.e_machine != K_ELF_MACHINE)
    {
        elf_error = "Unsupported ELF machine.";
        goto err2;
    }
    if (ehdr.e_ident[EI_CLASS] != K_ELF_CLASS)
    {
        elf_error = "Unsupported ELF class.";
        goto err2;
    }
    if (ehdr.e_ident[EI_DATA] != K_ELF_DATA)
    {
        elf_error = "Unsupported ELF data encoding.";
        goto err2;
    }

    Elf32_Shdr *shdrs = kmalloc(sizeof(Elf32_Shdr)*ehdr.e_shnum);
    file_seek(&elf, ehdr.e_shoff);
    Elf32_Sym *symtab = 0;

    // read section headers
    for (i = 0; i < ehdr.e_shnum; i++)
    {
        file_seek(&elf, ehdr.e_shoff+ehdr.e_shentsize*i);
        file_read(&elf, &shdrs[i], sizeof(Elf32_Shdr));
    }
    int j, cnt, stlink;
    Elf32_Shdr *s;
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
                    page_map(s->sh_addr + j, page_alloc(1), s->sh_flags & SHF_WRITE ? PF_WRITE : PF_NONE);
                file_seek(&elf, s->sh_offset);
                file_read(&elf, s->sh_addr, s->sh_size);
            }
            break;
        case SHT_NOBITS:
            if (s->sh_flags & SHF_ALLOC)
            {
                s->sh_addr = driver_end;
                driver_end = (void*)(((unsigned)driver_end + s->sh_size + 0xFFF) & 0xFFFFF000);
                for (j = 0; j < s->sh_size; j += 4096)
                    page_map(s->sh_addr + j, page_alloc(1), s->sh_flags & SHF_WRITE ? PF_WRITE : PF_NONE);
                kzeromem(s->sh_addr, s->sh_size);
            }
            break;
        case SHT_SYMTAB:
            cnt = s->sh_size/s->sh_entsize;
            symtab = kmalloc(cnt*sizeof(Elf32_Sym));
            stlink = s->sh_link;
            for (j = 0; j < cnt; j++)
            {
                file_seek(&elf, s->sh_offset+s->sh_entsize*j);
                file_read(&elf, &symtab[j], sizeof(Elf32_Sym));
            }
            break;
        }
    }
    
    // Read the string table
    char *strtab = kmalloc(shdrs[stlink].sh_size);
    file_seek(&elf, shdrs[stlink].sh_offset);
    file_read(&elf, strtab, shdrs[stlink].sh_size);

    void* drivermain = 0;
    static char t[1024];
    // Update symbols
    for (i = 1; i < cnt; i++)
    {
        if (symtab[i].st_shndx == SHN_UNDEF)
        {
            Elf32_Sym *ksym = find_symbol(strtab + symtab[i].st_name);
            if (ksym)
            {
                symtab[i].st_shndx = SHN_ABS;
                symtab[i].st_value = ksym->st_value;
            }
            else
            {
                elf_error = t;
                ksprintf(elf_error, "Undefined symbol '%s'", strtab+symtab[i].st_name);
                goto err1;
            }
        }
        else if (symtab[i].st_shndx < SHN_LORESERVE)
        {
            symtab[i].st_value += (unsigned)shdrs[symtab[i].st_shndx].sh_addr;
            if (!kstrcmp("driver_main", strtab+symtab[i].st_name)) drivermain = symtab[i].st_value;
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
            int count = s->sh_size/s->sh_entsize;
            for (j = 0; j < count; j++)
            {
                Elf32_Rel rel;
                file_seek(&elf, s->sh_offset+j*s->sh_entsize);
                file_read(&elf, &rel, sizeof(Elf32_Rel));
                unsigned soff = (unsigned)shdrs[s->sh_info].sh_addr;
                unsigned* ptr = rel.r_offset + soff;
                unsigned symval = (unsigned)symtab[ELF32_R_SYM(rel.r_info)].st_value;
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

    int retval;
    asm volatile("call *%1":"=a"(retval):"g"(drivermain));
    if (retval == 0)
        ret = 1;
    else
    {
        ksprintf(t, "Driver exited with status %l.", retval);
        elf_error = t;
    }
err1:
    kfree(strtab);
    kfree(symtab);
    kfree(shdrs);
err2:
    file_close(&elf);
    return ret;
}

// Creates and maps memory of the given size at the given address
// and maps it into this process, which must be unmapped.
static void* pmapmem(unsigned* pgdir, void* vaddr, unsigned size, int writeable, int user)
{
    void* addr = kfindrange(size);
    unsigned t;
    for (t = 0; t < size; t += 4096)
        page_map(addr+t, getpage(pgdir, vaddr+t, writeable, user), PF_WRITE);
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
    unsigned* pgtbl = kfindrange(0x1000);
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

char* elf_last_error()
{
    return elf_error;
}

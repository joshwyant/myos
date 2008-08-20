// An address ABOVE 0xC0000000 is shared between ALL processes and is used by the kernel.
// An address BELOW 0xC0000000 is only used by the CURRENT process, and may include NON-USER pages, such as the KERNEL STACK

#include "elf.h"
#include "fat.h"
#include "klib.h"

static void* getpage(unsigned* pgdir, void* vaddr, int, int);
static void* pmapmem(unsigned* pgdir, void* vaddr, unsigned size, int, int);

static char* elf_error;

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

int process_start(char* filename)
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
    process_enqueue(p);
    return 1;
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

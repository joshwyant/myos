#include "elf.h"

static void* new_page(unsigned* pgdir, void* vaddr);

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
    if (ehdr.e_version != EV_CURRENT)
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
        elf_error = "Unsupported ELF data encoding.\r\n";
        return 0;
    }
    // Allocate a page directory for the process
    void *pgdir_p = page_alloc(1);
    unsigned *pgdir = kfindrange(4096); // Find some address space to use to access the process' page directory
    page_map(pgdir, pgdir_p);
    // Wipe page directory
    for (i = 0; i < 1024; i++) pgdir[i] = 0;
    
    // load program segments
    for (i = 0; i < ehdr.e_phnum; i++)
    {
        Elf32_Phdr phdr;
        file_seek(&elf, ehdr.e_phoff+ehdr.e_phentsize*i);
        file_read(&elf, &phdr, sizeof(phdr));
        
    }
    
    file_close(&elf);
    page_unmap(pgdir);
    return 1;
}

// Logical (mapped) address of the page dir, address of new page, 
// returns mapped address of new page, must be unmapped
static void* new_page(unsigned* pgdir, void* vaddr)
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
        pgdir[pgdir_off] = (unsigned)pgtbl_p+3;
    }
    // Map in page table in kernel space
    unsigned* pgtbl = kfindrange(0x1000);
    page_map(pgtbl,pgtbl_p);
    // Allocate the page and map it in user space
    void* pg_p = page_alloc(1);
    pgtbl[pgtbl_off] = (((unsigned)pg_p)&~0xFFF)+3; // TODO: Use permissions
    // unmap page table
    page_unmap(pgtbl);
    // Map the new page in kernel space
    void* pg = kfindrange(0x1000);
    page_map(pg,pg_p);
    // return kernel mapped address
    return pg;
}

char* elf_last_error()
{
    return elf_error;
}

#include "elf.h"

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

int load_kernel(loader_info *li)
{
    elf_error = "";
    FileStream elf;
    Elf32_Ehdr ehdr;
    int i;
    if (!file_open("/system/bin/kernel", &elf))
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
    if ((ehdr.e_type != ET_EXEC) && (ehdr.e_type != ET_DYN))
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
    void* const obj_base = (void*)0xC0000000;
    unsigned  memsz = 0;
    Elf32_Phdr phdr;
    // Dynamic parameters
    unsigned* hash;
    Elf32_Rel *rel = 0;
    Elf32_Sym *symtab;
    char* strtab;
    unsigned relsz = sizeof(Elf32_Rel), relent = 0, relcount = 0;
    // load program segments
    for (i = 0; i < ehdr.e_phnum; i++)
    {
        file_seek(&elf, ehdr.e_phoff+ehdr.e_phentsize*i);
        file_read(&elf, &phdr, sizeof(phdr));
        if (phdr.p_type == PT_LOAD)
        {
            unsigned extent = (unsigned)phdr.p_vaddr + phdr.p_memsz;
            if (extent > memsz) memsz = extent;
            file_seek(&elf, phdr.p_offset);
            int j;
            for (j = 0; j < phdr.p_filesz; j += 4096)
            {
                void* p = findpage();
                page_map(obj_base+(unsigned)phdr.p_vaddr+j, p, PF_WRITE);
                file_read(&elf, p, phdr.p_filesz - j > 4096 ? 4096 : phdr.p_filesz - j);
            }
            for (j = (phdr.p_filesz+0xFFF)&0xFFFFF000; j < phdr.p_memsz; j += 4096)
            {
                void* p = findpage();
                page_map(obj_base+(unsigned)phdr.p_vaddr+j, p, PF_WRITE);
                kzeromem(p, phdr.p_memsz - j > 4096 ? 4096 : phdr.p_memsz - j);
            }
        }
        else if (phdr.p_type = PT_DYNAMIC)
        {
            Elf32_Dyn d;
            file_seek(&elf, phdr.p_offset);
            file_read(&elf, &d, sizeof(d));
            while (d.d_tag != DT_NULL)
            {
                switch (d.d_tag)
                {
                    case DT_HASH:
                        hash = obj_base + d.d_un.d_val;
                        break;
                    case DT_STRTAB:
                        strtab = obj_base + d.d_un.d_val;
                        break;
                    case DT_SYMTAB:
                        symtab = obj_base + d.d_un.d_val;
                        break;
                    case DT_REL:
                        rel = obj_base + d.d_un.d_val;
                        break;
                    case DT_RELSZ:
                        relsz = d.d_un.d_val;
                        break;
                    case DT_RELENT:
                        relent = d.d_un.d_val;
                        break;
                }
                file_read(&elf, &d, sizeof(d));
            }
        }
    }
    file_close(&elf);
    // Map the page directory into itself
    ((unsigned*)pgdir)[1023] = (unsigned)pgdir|PF_WRITE|PF_PRESENT;
    void** stack = findpage();
    stack[1023] = li;
    // map the stack
    page_map((void*)0xF0000000, stack, PF_WRITE);
    // Identity map first megabyte
    for (i = 0; i < 0x100000; i += 0x1000)
        page_map((void*)i, (void*)i, PF_WRITE);
    // Set the PDBR (page directory base register)
    asm volatile ("mov %%eax, %%cr3"::"a"(pgdir));
    // enable paging by setting bit 31 of control register 0
    unsigned cr0;
    asm volatile ("mov %%cr0, %0":"=a"(cr0)); // the 'a' means use %eax register
    asm volatile ("mov %0, %%cr0"::"a"(cr0|0x80000000));
    // Update symbol values
    for (i = 1; i < hash[1]; i++)
        if (symtab[i].st_shndx != SHN_ABS) symtab[i].st_value += (unsigned)obj_base;
    // Perform relocations
    if (rel)
    {
        if (relent == 0) relent = sizeof(Elf32_Rel);
        relcount = relsz/relent;
        for (i = 0; i < relcount; i++)
        {
            unsigned* ptr = obj_base + (unsigned)rel[i].r_offset;
            int type = ELF32_R_TYPE(rel[i].r_info);
            int sym = ELF32_R_SYM(rel[i].r_info);
            unsigned symval = (unsigned)symtab[sym].st_value;
            switch (type)
            {
                case R_386_32:
                    *ptr += symval;
                    break;
                case R_386_PC32:
                    *ptr += symval - (unsigned)ptr;
                    break;
                case R_386_RELATIVE:
                    *ptr += (unsigned)obj_base;
                    break;
            }
        }
    }
    // Set up the loader_info structure
    li->memsize = memsz;
    li->loaded = obj_base;
    li->freemem = next_page;
    // set the stack pointer
    asm volatile ("mov $0xF0000FFC, %esp");
    // Jump to the entry point
    asm volatile ("jmp *%0"::"g"(obj_base + (unsigned)ehdr.e_entry));
    return 1;
}

char* elf_last_error()
{
    return elf_error;
}

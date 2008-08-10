# ifndef __elf_h__
# define __elf_h__

#include "klib.h"
#include "fat.h"

typedef struct
{
    char *name;
    void *ptr;
} kernel_symbol;

extern const kernel_symbol kernel_symbols[];

#define EI_MAG0        0
#define EI_MAG1        1
#define EI_MAG2        2
#define EI_MAG3        3
#define EI_CLASS       4
#define EI_DATA        5
#define EI_VERSION     6
#define EI_PAD         7
#define EI_NIDENT      16

#define ELFMAG0        0x7f
#define ELFMAG1        'E'
#define ELFMAG2        'L'
#define ELFMAG3        'F'

#define ET_NONE        0
#define ET_REL         1
#define ET_EXEC        2
#define ET_DYN         3
#define ET_CORE        4
#define ET_LOPROC      0xFF00
#define ET_HIPROC      0xFFFF

#define EM_NONE        0
#define EM_M32         1
#define EM_SPARC       2
#define EM_386         3
#define EM_68K         4
#define EM_88K         5
#define EM_680         7
#define EM_MIPS        8
#define EM_MIPS_RS4_BE 10

#define EV_NONE        0
#define EV_CURRENT     1

#define ELFCLASSNONE   0
#define ELFCLASS32     1
#define ELFCLASS64     2

#define ELFDATANONE    0
#define ELFDATA2LSB    1
#define ELFDATA2MSB    2

#define Elf32_Addr     void*
#define Elf32_Half     unsigned short
#define Elf32_Off      unsigned int
#define Elf32_Sword    int
#define Elf32_Word     unsigned int

#define SHN_UNDEF      0
#define SHN_LORESERVE  0xFF00
#define SHN_LOPROC     0xFF00
#define SHN_HIPROC     0xFF1F
#define SHN_ABS        0xFFF1
#define SHN_COMMON     0xFFF2
#define SHN_HIRESERVE  0xFFFF

#define SHT_NULL       0
#define SHT_PROGBITS   1
#define SHT_SYMTAB     2
#define SHT_STRTAB     3
#define SHT_RELA       4
#define SHT_HASH       5
#define SHT_DYNAMIC    6
#define SHT_NOTE       7
#define SHT_NOBITS     8
#define SHT_REL        9
#define SHT_SHLIB      10
#define SHT_DYNSYM     11
#define SHT_LOPROC     0x70000000
#define SHT_HIPROC     0x7fffffff
#define SHT_LOUSER     0x80000000
#define SHT_HIUSER     0xffffffff

#define SHF_WRITE      0x1
#define SHF_ALLOC      0x2
#define SHF_EXECINSTR  0x4
#define SHF_MASKPROC   0xf0000000

#define PT_NULL        0
#define PT_LOAD        1
#define PT_DYNAMIC     2
#define PT_INTERP      3
#define PT_NOTE        4
#define PT_SHLIB       5
#define PT_PHDR        6
#define PT_LOPROC      0x70000000
#define PT_HIPROC      0x7fffffff

#define STB_LOCAL      0
#define STB_GLOBAL     1
#define STB_WEAK       2
#define STB_LOPROC     13
#define STB_HIPROC     15

#define STT_NOTYPE     0
#define STT_OBJECT     1
#define STT_FUNC       2
#define STT_SECTION    3
#define STT_FILE       4
#define STT_LOPROC     13
#define STT_HIPROC     15

#define PF_X	       1
#define PF_W	       2
#define PF_R	       4
#define PF_MASKPROC	   0xf0000000

#define K_ELF_MACHINE  EM_386
#define K_ELF_CLASS    ELFCLASS32
#define K_ELF_DATA     ELFDATA2LSB

#define ELF32_ST_BIND (i) ((i)>>4)
#define ELF32_ST_TYPE (i) ((i)&0xf)
#define ELF32_ST_INFO (b,t) (((b)<<4)+((t)&0xf))

typedef struct {
        unsigned char       e_ident[EI_NIDENT];
        Elf32_Half          e_type;
        Elf32_Half          e_machine;
        Elf32_Word          e_version;
        Elf32_Addr          e_entry;
        Elf32_Off           e_phoff;
        Elf32_Off           e_shoff;
        Elf32_Word          e_flags;
        Elf32_Half          e_ehsize;
        Elf32_Half          e_phentsize;
        Elf32_Half          e_phnum;
        Elf32_Half          e_shentsize;
        Elf32_Half          e_shnum;
        Elf32_Half          e_shstrndx;
} Elf32_Ehdr;

typedef struct {
        Elf32_Word          sh_name;
        Elf32_Word          sh_type;
        Elf32_Word          sh_flags;
        Elf32_Addr          sh_addr;
        Elf32_Off           sh_offset;
        Elf32_Word          sh_size;
        Elf32_Word          sh_link;
        Elf32_Word          sh_info;
        Elf32_Word          sh_addralign;
        Elf32_Word          sh_entsize;  
} Elf32_Shdr;

typedef struct {
        Elf32_Word          p_type;
        Elf32_Off           p_offset;
        Elf32_Addr          p_vaddr;
        Elf32_Addr          p_paddr;
        Elf32_Word          p_filesz;
        Elf32_Word          p_memsz;
        Elf32_Word          p_flags;
        Elf32_Word          p_align;
} Elf32_Phdr;

typedef struct {
        Elf32_Word          st_name;
        Elf32_Addr          st_value;
        Elf32_Word          st_size;
        unsigned char       st_info;
        unsigned char       st_other;
        Elf32_Half          st_shndx;
} Elf32_Sym;

static char* elf_error;

#endif /* __elf_h__ */

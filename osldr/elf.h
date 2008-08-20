// Josh Wyant
// Structures and #defines taken from Tool Interface Standard (TIS) Executable and Linking Format (ELF) Specification Version 1.2

# ifndef __elf_h__
# define __elf_h__

#include "ldrlib.h"
#include "fat.h"

/* Book I: Executable and Linking Format (ELF) */

/* 32-Bit Data Types */
#define Elf32_Addr	void*
#define Elf32_Half	unsigned short
#define Elf32_Off	unsigned int
#define Elf32_Sword	int
#define Elf32_Word	unsigned int

/* Object Files */

/* ELF Header */

#define EI_NIDENT	16

// ELF Header
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

// e_type
#define ET_NONE		0
#define ET_REL		1
#define ET_EXEC		2
#define ET_DYN		3
#define ET_CORE		4
#define ET_LOPROC	0xFF00
#define ET_HIPROC	0xFFFF

// e_machine
#define EM_NONE		0
#define EM_M32		1
#define EM_SPARC	2
#define EM_386		3
#define EM_68K		4
#define EM_88K		5
#define EM_680		7
#define EM_MIPS		8
#define EM_MIPS_RS4_BE	10

// e_version
#define EV_NONE		0
#define EV_CURRENT	1

// ELF Identification

// e_ident[] Identification Indexes
#define EI_MAG0		0
#define EI_MAG1		1
#define EI_MAG2		2
#define EI_MAG3		3
#define EI_CLASS	4
#define EI_DATA		5
#define EI_VERSION	6
#define EI_PAD		7

// e_ident[EI_MAG0] - e_ident[EI_MAG3]
#define ELFMAG0		0x7f
#define ELFMAG1		'E'
#define ELFMAG2		'L'
#define ELFMAG3		'F'

// e_ident[EI_CLASS]
#define ELFCLASSNONE	0
#define ELFCLASS32	1
#define ELFCLASS64	2

// e_ident[EI_CLASS]
#define ELFDATANONE	0
#define ELFDATA2LSB	1
#define ELFDATA2MSB	2

/* Sections */

// Special Section Indexes
#define SHN_UNDEF	0
#define SHN_LORESERVE	0xFF00
#define SHN_LOPROC	0xFF00
#define SHN_HIPROC	0xFF1F
#define SHN_ABS		0xFFF1
#define SHN_COMMON	0xFFF2
#define SHN_HIRESERVE	0xFFFF

// Section Header
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

// Section Types, sh_type
#define SHT_NULL	0
#define SHT_PROGBITS	1
#define SHT_SYMTAB	2
#define SHT_STRTAB	3
#define SHT_RELA	4
#define SHT_HASH	5
#define SHT_DYNAMIC	6
#define SHT_NOTE	7
#define SHT_NOBITS	8
#define SHT_REL		9
#define SHT_SHLIB	10
#define SHT_DYNSYM	11
#define SHT_LOPROC	0x70000000
#define SHT_HIPROC	0x7fffffff
#define SHT_LOUSER	0x80000000
#define SHT_HIUSER	0xffffffff

// Section Attribute Flags, sh_flags
#define SHF_WRITE	0x1
#define SHF_ALLOC	0x2
#define SHF_EXECINSTR	0x4
#define SHF_MASKPROC	0xf0000000

/* Symbol Table */

#define STN_UNDEF	0

// Symbol Table Entry
typedef struct {
        Elf32_Word          st_name;
        Elf32_Addr          st_value;
        Elf32_Word          st_size;
        unsigned char       st_info;
        unsigned char       st_other;
        Elf32_Half          st_shndx;
} Elf32_Sym;

// st_info
#define ELF32_ST_BIND (i) ((i)>>4)
#define ELF32_ST_TYPE (i) ((i)&0xf)
#define ELF32_ST_INFO (b,t) (((b)<<4)+((t)&0xf))

// Symbol Binding, ELF32_ST_BIND
#define STB_LOCAL	0
#define STB_GLOBAL	1
#define STB_WEAK	2
#define STB_LOPROC	13
#define STB_HIPROC	15

// Symbol Types, ELF32_ST_TYPE
#define STT_NOTYPE	0
#define STT_OBJECT	1
#define STT_FUNC	2
#define STT_SECTION	3
#define STT_FILE	4
#define STT_LOPROC	13
#define STT_HIPROC	15

/* Relocation */

// Relocation Entries
typedef struct {
	Elf32_Addr	r_offset;
	Elf32_Word	r_info;
} Elf32_Rel;

typedef struct {
	Elf32_Addr	r_offset;
	Elf32_Word	r_info;
	Elf32_Sword	r_addend;
} Elf32_Rela;

// r_info
#define ELF32_R_SYM(i)		((i)>>8)
#define ELF32_R_TYPE(i)		((unsigned char)(i))
#define ELF32_R_INFO(s,t)	(((s)<<8)+(unsigned char)(t))

/* Program Loading and Dynamic Linking */

/* Program Header */

// Program Header
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

// Segment Types, p_type
#define PT_NULL		0
#define PT_LOAD		1
#define PT_DYNAMIC	2
#define PT_INTERP	3
#define PT_NOTE		4
#define PT_SHLIB	5
#define PT_PHDR		6
#define PT_LOPROC	0x70000000
#define PT_HIPROC	0x7fffffff

/* Book II: Processer Specific (Intel Architecture) */
// Relocation Types
#define R_386_NONE	0
#define R_386_32	1
#define R_386_PC32	2

/* Book III: Operating System Specific (UNIX System V Release 4) */

// Segment Flag Bits, p_flags
#define PF_X		1
#define PF_W		2
#define PF_R		4
#define PF_MASKPROC	0xf0000000

// Dynamic Structure
typedef struct {
	Elf32_Sword	d_tag;
	union {
		Elf32_Word	d_val;
		Elf32_Addr	d_ptr;
	} d_un;
} Elf32_Dyn;
extern Elf32_Dyn _DYNAMIC[];

// Dynamic Array Tags, d_tag
#define DT_NULL		0
#define DT_NEEDED	1
#define DT_PLTRELSZ	2
#define DT_PLTGOT	3
#define DT_HASH		4
#define DT_STRTAB	5
#define DT_SYMTAB	6
#define DT_RELA		7
#define DT_RELASZ	8
#define DT_RELAENT	9
#define DT_STRSZ	10
#define DT_SYMENT	11
#define DT_INIT		12
#define DT_FINI		13
#define DT_SONAME	14
#define DT_RPATH	15
#define DT_SYMBOLIC	16
#define DT_REL		17
#define DT_RELSZ	18
#define DT_RELENT	19
#define DT_PLTREL	20
#define DT_DEBUG	21
#define DT_TEXTREL	22
#define DT_JMPREL	23
#define DT_JMP_REL	23
#define DT_BIND_NOW	24
#define DT_LOPROC	0x70000000
#define DT_HIPROC	0x7FFFFFFF

/* Appendix A: Intel Architecture and System V Release 4 Dependencies */

// Relocation Types
#define R_386_GOT32	3
#define R_386_PLT32	4
#define R_386_COPY	5
#define R_386_GLOB_DAT	6
#define R_386_JMP_SLOT	7
#define R_386_RELATIVE	8
#define R_386_GOTOFF	9
#define R_386_GOTPC	10

// Global Offset Table
extern Elf32_Addr	_GLOBAL_OFFSET_TABLE_[];

/* Our OS-specific constants */
#define K_ELF_MACHINE	EM_386
#define K_ELF_CLASS	ELFCLASS32
#define K_ELF_DATA	ELFDATA2LSB

extern unsigned long elf_hash(const unsigned char *name);

#endif /* __elf_h__ */

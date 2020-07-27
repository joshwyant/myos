#ifndef __klib_h__
#define __klib_h__

// Include inline IO functions with klib
#include "io.h"
#include "dictionary.h"
#include "elf.h"
#include "string.h"
#include "memory.h"

// defines
#define PF_NONE		0
#define PF_READ		0
#define PF_SUPERVISOR	0
#define PF_PRESENT	1<<0
#define PF_WRITE	1<<1
#define PF_USER		1<<2
#define PF_WRITETHROUGH	1<<3
#define PF_CACHEDISABLE 1<<4
#define PF_ACCESSED	1<<5
#define PF_DIRTY	1<<6
#define PF_PAT		1<<7
#define PF_GLOBAL	1<<8
#define PF_AVAIL1	1<<9
#define PF_AVAIL2	1<<10
#define PF_AVAIL3	1<<11

#define PF_LOCKED	PF_AVAIL1

#ifdef __cplusplus
#include <memory>
#include "fs.h"

extern "C" {
#endif

// Structures

typedef unsigned char TBYTE[10];
typedef struct
{
    unsigned short  control_word;
    unsigned short  unused1;
    unsigned short  status_word;
    unsigned short  unused2;
    unsigned short  tag_word;
    unsigned short  unused3;
    unsigned int    instruction_pointer;
    unsigned short  code_segment;
    unsigned short  unused4;
    unsigned int    operand_address;
    unsigned short  data_segment;
    TBYTE           st0;
    TBYTE           st1;
    TBYTE           st2;
    TBYTE           st3;
    TBYTE           st4;
    TBYTE           st5;
    TBYTE           st6;
    TBYTE           st7;
} FPUFile;

typedef struct
{
    unsigned int     esp;       // kernel esp
    unsigned short   ss, __ssh; // kernel ss
    unsigned int     cr3;       // process cr3
    unsigned int     vm8086;    // vm8086 process
    void*            gpfault;   // vm8086 gpfault handler
    unsigned int     pid;       // process id
    unsigned int     timeslice; // Amount of time to run
    unsigned int     priority;  // Process priority
    unsigned char    name[64];  // Name of process
    void*	     node;	// Process_Node struct for scheduler
    int             blocked;
    int             fpu_saved;
    FPUFile          fpu_file;
} Process;

// Functions
extern Elf32_Sym *find_symbol(const char* name);
// exceptions
extern void register_isr(int num, int dpl, void* offset);
extern void register_trap(int num, int dpl, void* offset);
extern void register_task(int num, int dpl, unsigned short selector);
// processes
extern void	process_yield();
extern Process*	process_create(const char* name);
extern void	process_enqueue(Process* p);

typedef struct {
    int Second;
    int Minute;
    int Hour;
    int Day;
    int Month;
    int Year;
} DateTime;

// Globals

extern Process				*current_process;
extern volatile unsigned*		system_pdt;
extern volatile unsigned*		process_pdt;
extern unsigned*		kernel_hashtable;
extern unsigned			kernel_nbucket;
extern unsigned			kernel_nchain;
extern unsigned*		kernel_bucket;
extern unsigned*		kernel_chain;
extern Elf32_Sym*		kernel_symtab;
extern char*			kernel_strtab;
extern Elf32_Dyn*		kernel_dynamic;

// elf.c
extern const char *elf_last_error();

#ifdef __cplusplus
}  // extern "C"

// elf.c
extern int process_start(std::shared_ptr<kernel::FileSystemDriver> fs_driver, const char* filename);
extern int load_driver(std::shared_ptr<kernel::FileSystemDriver> fs_driver, const char* filename);
#endif

#endif

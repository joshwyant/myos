#ifndef __klib_h__
#define __klib_h__

// Include inline IO functions with klib
#include "io.h"

// defines
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


// Structures

typedef struct
{
    unsigned int     esp;       // kernel esp
    unsigned short   ss, __ssh; // kernel ss
    unsigned int     cr3;       // process cr3
    unsigned int     pid;       // process id
    unsigned int     timeslice; // Amount of time to run
    unsigned int     priority;  // Process priority
    unsigned char    name[64];  // Name of process
} Process;

// Functions

// exceptions
void register_isr(int num, int dpl, void* offset);
void register_trap(int num, void* offset);
void register_task(int num, unsigned short selector);
// Memory
void  page_free(void*, int);
void* page_alloc(int);
void* extended_alloc(int);
void* base_alloc(int);
void  page_map(void *logical,void *physical,unsigned flags);
void  page_unmap(void *logical);
void* kmalloc(int);
void  kfree(void*);
void* kfindrange(int size);
void* get_physaddr(void* logical);
static inline void kmemcpy(void* dest, const void* src, unsigned bytes)
{
    asm volatile (
        "cld; rep; movsb":
        "=c"(bytes),"=S"(src),"=D"(dest):
        "c"(bytes),"S"(src),"D"(dest)
    );
}
static inline void kzeromem(void* dest, unsigned bytes)
{
    asm volatile (
        "cld; rep; stosb":
        "=c"(bytes),"=D"(dest):
        "c"(bytes),"D"(dest),"a"(0)
    );
}
// processes
Process*	process_create(const char* name);
void		process_enqueue(Process* p);
// various
static inline char ktoupper(char c)
{
    if ((c < 'a') || (c > 'z')) return c;
    return c-'a'+'A';
}
static inline char ktolower(char c)
{
    if ((c < 'A') || (c > 'Z')) return c;
    return c-'A'+'a';
}
int kstrlen(char* str);
const char* ksprintf(char* dest, const char* format, ...);
const char* sprinthexb(char*, char);
const char* sprinthexw(char*, short);
const char* sprinthexd(char*, int);
const char* sprintdec(char*, int);
const char* kstrcpy(char*, const char*);
const char* kstrcat(char*, const char*);
int kstrcmp(const char*, const char*);

// Globals

Process				*current_process;
volatile unsigned*		system_pdt;
#define				process_pdt ((unsigned*)0xFFFFF000)

// elf.c
int process_start(char* filename);
char *elf_last_error();

#endif

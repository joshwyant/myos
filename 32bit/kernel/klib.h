#ifndef __klib_h__
#define __klib_h__

// Include inline IO functions with klib
#include "io.h"

// Structures

// Task State Segment
typedef struct
{
    unsigned short   link,        r0;
    unsigned         esp0;
    unsigned short   ss0,         r1;
    unsigned         esp1;
    unsigned short   ss1,         r2;
    unsigned         esp2;
    unsigned short   ss2,         r3;
    unsigned         cr3;
    unsigned         eip;
    unsigned         eflags;
    unsigned         eax;
    unsigned         ecx;
    unsigned         edx;
    unsigned         ebx;
    unsigned         esp;
    unsigned         ebp;
    unsigned         esi;
    unsigned         edi;
    unsigned short   es,          r4;
    unsigned short   cs,          r5;
    unsigned short   ss,          r6;
    unsigned short   ds,          r7;
    unsigned short   fs,          r8;
    unsigned short   gs,          r9;
    unsigned short   ldtr,        r10;
    unsigned short   iopboff,     r11;
} TaskStateSegment;

typedef struct
{
    unsigned int     esp;
    unsigned short   ss;
    unsigned int     pid;
    unsigned int     timeslice;
    unsigned int     priority;
    unsigned int     cr3;
    int              killme;
    unsigned char    name[64];
} Process;

typedef struct _ProcessNode
{
    Process                  *process;
    struct _ProcessNode      *prev;
    struct _ProcessNode      *next;
} ProcessNode;

typedef struct
{
    ProcessNode      *first;
    ProcessNode      *last;
} ProcessQueue;

// Functions

// exceptions
void register_isr(int num, void* offset);
void register_trap(int num, void* offset);
void register_task(int num, unsigned short selector);
// Memory
void  page_free(void*, int);
void* page_alloc(int);
void* extended_alloc(int);
void* base_alloc(int);
void  page_map(void *logical,void *physical);
void  page_unmap(void *logical);
void* kmalloc(int);
void  kfree(void*);
void* kfindrange(int size);
void* get_physaddr(void* logical);
// processes
Process* process_create(char* name);
void     process_enqueue(Process* p);
Process* process_dequeue();
void     process_switch();
void     killme();
void     process_node_delete(ProcessNode* n);
void     process_node_after(Process* p, ProcessNode* prev);
void     process_node_before(Process* p, ProcessNode* prev);

// Globals

ProcessQueue         processes;
unsigned             process_count;
Process              *current_process;
ProcessNode          *current_process_node;
TaskStateSegment     system_tss;

// elf.c
int process_start(char* filename);
char *elf_last_error();

#endif

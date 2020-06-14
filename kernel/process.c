#include "kernel.h"

unsigned	*kernel_hashtable;
unsigned	kernel_nbucket;
unsigned	kernel_nchain;
unsigned	*kernel_bucket;
unsigned	*kernel_chain;
Elf32_Sym	*kernel_symtab;
char		*kernel_strtab;
Elf32_Dyn	*kernel_dynamic;

Process					*current_process;
volatile unsigned		*system_pdt;
volatile unsigned		*process_pdt = (unsigned*)0xFFFFF000;
ProcessQueue			processes;
unsigned				process_inc;
int						switch_voluntary = 0; // Whether the current task switch was explicit

void init_processes()
{
    processes.first = 0;
    processes.last = 0;
    process_inc = 0;
    current_process = 0;
}

// Yield to other processes (give up this process' time slice)
void process_yield()
{
    // Tell the timer handler the switch was voluntary
    switch_voluntary = 1;
    // IRQ0, the timer handler; Switches tasks
    int86(0x20);
}

// Do not use in an ISR.
Process* process_create(const char* name)
{
    Process *p = kmalloc(sizeof(Process));
    kstrcpy(p->name, name);
    p->pid = process_inc++;
    return p;
}

// Safe to use without any kind of lock.
// NOT safe to use in an ISR.
void process_enqueue(Process* p)
{
    ProcessNode *n = kmalloc(sizeof(ProcessNode));
    n->process = p;
    int i = int_disable();
    process_node_link(n);
    int_restore(i);
}

// MUST CLEAR INTS BEFORE USING
void process_node_unlink(ProcessNode* n)
{
    if (!n) return;
    if (n == processes.first) processes.first = n->next;
    if (n == processes.last) processes.last = n->prev;
    if (n->prev) n->prev->next = n->next;
    if (n->next) n->next->prev = n->prev;
}

// CLEAR INTS BEFORE USING
void process_node_link(ProcessNode* n)
{
    if (!n) return;
    if (processes.last) processes.last->next = n;
    n->prev = processes.last;
    n->next = 0;
    processes.last = n;
    if (!processes.first) processes.first = n;
}

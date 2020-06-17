#ifndef __PROCESS_H__
#define __PROCESS_H__

#ifdef __cplusplus
extern "C" {
#endif

// Processes
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

extern void	process_yield();
extern void	process_node_unlink(ProcessNode* n);
extern void	process_node_link(ProcessNode* n);
extern void init_processes();
extern void	init_tss();

extern unsigned		*kernel_hashtable;
extern unsigned		kernel_nbucket;
extern unsigned		kernel_nchain;
extern unsigned		*kernel_bucket;
extern unsigned		*kernel_chain;
extern Elf32_Sym	*kernel_symtab;
extern char			*kernel_strtab;
extern Elf32_Dyn	*kernel_dynamic;

extern Process					*current_process;
extern volatile unsigned		*system_pdt;
extern volatile unsigned		*process_pdt;
extern ProcessQueue				processes;
extern unsigned					process_inc;
extern int						switch_voluntary; // Whether the current task switch was explicit

#ifdef __cplusplus
}  // extern "C"
#endif

#endif  // __PROCESS_H__

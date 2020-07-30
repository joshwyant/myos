#ifndef __PROCESS_H__
#define __PROCESS_H__

#include <stdbool.h>
#include "fs.h"
#include "io.h"

#ifdef __cplusplus
#include <memory>

extern "C" {
#endif

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
extern Process*	process_create(const char* name);
extern void	process_enqueue(Process* p);
extern void init_processes();
extern void	init_tss();

extern Process					*current_process;
extern volatile unsigned		*system_pdt;
extern volatile unsigned		*process_pdt;
extern ProcessQueue				processes;
extern unsigned					process_inc;
extern bool						switch_voluntary; // Whether the current task switch was explicit

#ifdef __cplusplus
}  // extern "C"

extern int process_start(std::shared_ptr<kernel::FileSystem> fs, const char* filename);

namespace kernel
{
// RAII wrapper around lock()
class ScopedLock
{
public:
    ScopedLock(int& val)
        : ptr(&val)
    {
        while (lock(ptr)) process_yield();
    }
    ~ScopedLock()
    {
        *ptr = 0;
    }
private:
    int *ptr;
};
} // namespace kernel
#endif  // __cplusplus
#endif  // __PROCESS_H__

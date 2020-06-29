#include "kernel.h"

#define SYSCALL_EXIT    0
#define SYSCALL_CLOSE   1
#define SYSCALL_ENVC    2
#define SYSCALL_ENVLEN  3
#define SYSCALL_ENVCP   4
#define SYSCALL_ARGC    5
#define SYSCALL_ARGLEN  6
#define SYSCALL_ARGCP   7
#define SYSCALL_START   8
#define SYSCALL_PID     9
#define SYSCALL_SEEK    10
#define SYSCALL_READ    11
#define SYSCALL_WRITE   12
#define SYSCALL_MAP     13
#define SYSCALL_CLS     14
#define SYSCALL_YIELD   15
#define SYSCALL_OPEN    16
#define SYSCALL_TTY     17
#define SYSCALL_FORK    18
#define SYSCALL_FSTAT   19
#define SYSCALL_STAT    20
#define SYSCALL_LINK    21
#define SYSCALL_SBRK    22
#define SYSCALL_TIMES   23
#define SYSCALL_UNLINK  24
#define SYSCALL_WAIT    25
#define SYSCALL_TOFDAY  26
#define SYSCALL_KILL    27
#define SYSCALL_SIGNAL  28

struct MYOS_STAT
{
    unsigned st_mode;
    // TODO
};

void init_syscalls()
{
    register_trap(0x30, 3, (void*)int30);
}

// syscall, called using a trap gate int. Interrupts are still enabled, so the code must
// be fully thread-safe, and not cause any conflicts.
void syscall(unsigned edi, unsigned esi, unsigned ebp, unsigned esp, unsigned ebx, unsigned edx, unsigned ecx, unsigned eax)
{
    // eax = system call number.
    // extra info normally in edx
    // returns any return value in eax
    int m;
    switch (eax)
    {
        // End process (ebx = exit code)
        case SYSCALL_EXIT:
            m = int_disable();
            process_node_unlink(processes.first);
            int_restore(m);
            process_yield();
            break;
        // Close a file #edx.
        case SYSCALL_CLOSE:
            eax = 0;
            break;
        // Get environment variable count
        case SYSCALL_ENVC:
            eax = 0;
            break;
        // Get the length of environment variable #edx
        case SYSCALL_ENVLEN:
            eax = 0;
            break;
        // Copy the environment variable number edx to ebx
        case SYSCALL_ENVCP:
            *((char*)ebx) = 0;
            break;
        // Get argument count
        case SYSCALL_ARGC:
            eax = 0;
            break;
        // Get the length of an argument #edx
        case SYSCALL_ARGLEN:
            eax = 0;
            break;
        // Copy argument edx into location ebx
        case SYSCALL_ARGCP:
            *((char*)ebx) = 0;
            break;
        // Start a new process (ebx = *name, esi = **argv, edi = **env)
        case SYSCALL_START:
            eax = process_start((char*)ebx);
            break;
        // Get Process ID
        case SYSCALL_PID:
            eax = current_process->pid;
            break;
        // Seek in a file (file edx to esi, from reference point ecx)
        case SYSCALL_SEEK:
            eax = 0;
            break;
        // Read from a file (file edx, ptr edi, count ecx)
        case SYSCALL_READ:
            eax = 0;
            break;
        // Write to a file (file edx, ptr esi, count ecx)
        case SYSCALL_WRITE:
            if (edx == 0)
            {
                printlen((const char*)esi, ecx);
                eax = ecx;
            }
            else
            {
                eax = 0;
            }
            break;
        // Map address space (ptr edi, bytes ecx)
        case SYSCALL_MAP:
            eax = 0;
            break;
        // Clear the screen
        case SYSCALL_CLS:
            cls();
            break;
        // Yield to other processes
        case SYSCALL_YIELD:
            process_yield();
            break;
        // Open a file (name = esi, flags = ecx, mode = edx)
        case SYSCALL_OPEN:
            eax = (unsigned)-1;
            break;
        // Is a tty (file edx)
        case SYSCALL_TTY:
            eax = 1;
            break;
        // Fork
        case SYSCALL_FORK:
            eax = -1;
            ebx = 35;  // EAGAIN
            break;
        // fstat (edx file, edi MyOS stat struct)
        case SYSCALL_FSTAT:
            {
                struct MYOS_STAT *_stat = (struct MYOS_STAT*)(edi);
                _stat->st_mode = 0020000;  // S_IFCHR;
                eax = 0;
                break;
            }
        // stat (esi filename, edi MyOS stat struct)
        case SYSCALL_STAT:
            {
                struct MYOS_STAT *_stat = (struct MYOS_STAT*)(edi);
                _stat->st_mode = 0020000;  // S_IFCHR;
                eax = 0;
                break;
            }
        // link (esi old, edi new, errno in ebx)
        case SYSCALL_LINK:
            ebx = 31;  // EMLINK
            eax = -1;
            break;
        // sbrk (ebx heap_end, ecx incr)
        case SYSCALL_SBRK:
            eax = 0;
            break;
        // times (edi buffer)
        case SYSCALL_TIMES:
            eax = -1;
            break;
        // unlink (edi fname)
        case SYSCALL_UNLINK:
            ebx = 2;  // ENOENT
            eax = -1;
            break;
        // wait (edi = int *status)
        case SYSCALL_WAIT:
            ebx = 10;  // ECHILD
            eax = -1;
            break;
        // Get time of day (esi = *timeval, edi = *timezone)
        case SYSCALL_TOFDAY:
            ebx = 78;  // ENOSYS (function not implemented)
            eax = 0;
            break;
        // Kill (ecx = pid, edx = sig, ebx = errno)
        case SYSCALL_KILL:
            ebx = 22;  // EINVAL
            eax = -1;
            break;
        // Kill (edx = sig, edi = func)
        case SYSCALL_SIGNAL:
            // TODO
            eax = edi;
            break;
    }
}

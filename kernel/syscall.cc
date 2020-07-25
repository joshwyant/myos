#include <errno.h>
#include <reent.h>
#include <sys/stat.h>
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
//#define SYSCALL_TOFDAY  26
#define SYSCALL_KILL    27
#define SYSCALL_SIGNAL  28

extern "C"
{

struct status_tuple
{
    int status;
    int eno;
};

void init_syscalls()
{
    register_trap(0x30, 3, (void*)int30);
}

void user__exit(int exit_code);
// Close a file.
int user_close(int file);
// Transfer control to a new process.
int user_execve(char *name, char **argv, char **env);
// Create a new process.
struct status_tuple user_fork();
// Status of an open file.
// For consistency with other minimal implementations in these examples,
// all files are regarded as character special devices.
// The `sys/stat.h' header file is distributed in the `include'
// subdirectory for this C library.
int user_fstat(int file, struct stat *st);
// Query whether output stream is a terminal.
int user_isatty(int file);
// Send a signal.
struct status_tuple user_kill(int pid, int sig);
// Establish a new name for an existing file.
struct status_tuple user_link(char *old, char *_new);
// Set position in a file.
int user_lseek(int file, int ptr, int dir);
// Open a file.
int user_open(const char *name, int flags, mode_t mode);
// Read from a file.
int user_read(int file, char *ptr, int len);
// Increase program data space.
caddr_t user_sbrk(char *heap_end, int incr);
// Status of a file (by name).
int user_stat(const char *file, struct stat *st);
// Timing information for current process.
clock_t user_times(struct tms *buf);
// Remove a file's directory entry.
struct status_tuple user_unlink(char *name);
// Wait for a child process.
struct status_tuple user_wait(int *status);
// Write a character to a file.
int user_write(int file, char *ptr, int len);
void (*user_signal(int sig, void (*func)(int)))(int);
int user_map(void *ptr, int bytes);

// syscall, called using a trap gate int. Interrupts are still enabled, so the code must
// be fully thread-safe, and not cause any conflicts.
void syscall(unsigned edi, unsigned esi, unsigned ebp, unsigned esp, unsigned ebx, unsigned edx, unsigned ecx, unsigned eax)
{
    // eax = system call number.
    // extra info normally in edx
    // returns any return value in eax
    switch (eax)
    {
        // End process (ebx = exit code)
        case SYSCALL_EXIT:
            user__exit(ebx);
            break;
        // Close a file #edx.
        case SYSCALL_CLOSE:
            eax = user_close(edx);
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
            eax = process_start(kernel::FileSystemDriver::get_root(), (char*)ebx);
            break;
        // Get Process ID
        case SYSCALL_PID:
            eax = current_process->pid;
            break;
        // Seek in a file (file edx to esi, from reference point ecx)
        case SYSCALL_SEEK:
            eax = user_lseek(edx, edi, ecx);
            break;
        // Read from a file (file edx, ptr edi, count ecx)
        case SYSCALL_READ:
            eax = user_read(edx, (char *)(void *)edi, ecx);
            break;
        // Write to a file (file edx, ptr esi, count ecx)
        case SYSCALL_WRITE:
            eax = user_write(edx, (char *)esi, ecx);
            break;
        // Map address space (ptr edi, bytes ecx)
        case SYSCALL_MAP:
            eax = user_map((void *)edi, ecx);
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
            eax = user_open((const char *)(void *)esi, ecx, edx);
            break;
        // Is a tty (file edx)
        case SYSCALL_TTY:
            eax = user_isatty(edx);
            break;
        // Fork
        case SYSCALL_FORK:
        {
            auto result = user_fork();
            eax = result.status;
            ebx = result.eno;
            break;
        }
        // fstat (edx file, edi MyOS stat struct)
        case SYSCALL_FSTAT:
            eax = user_fstat(edx, (struct stat*)edi);
            break;
        // stat (esi filename, edi MyOS stat struct)
        case SYSCALL_STAT:
            eax = user_stat((const char *)edx, (struct stat*)edi);
            break;
        // link (esi old, edi new, errno in ebx)
        case SYSCALL_LINK:
        {
            auto result = user_link((char *)(void *)esi, (char *)(void *)edi);
            eax = result.status;
            ebx = result.eno;
            break;
        }
        // sbrk (ebx heap_end, ecx incr)
        case SYSCALL_SBRK:
            eax = (unsigned)user_sbrk((char *)(void *)ebx, ecx);
            break;
        // times (edi buffer)
        case SYSCALL_TIMES:
            eax = user_times((tms *)(void *)edi);
            break;
        // unlink (edi fname)
        case SYSCALL_UNLINK:
        {
            auto result = user_unlink((char *)(void *)edi);
            eax = result.status;
            ebx = result.eno;
            break;
        }
        // wait (edi = int *status)
        case SYSCALL_WAIT:
        {
            auto result = user_wait((int *)(void *)edi);
            eax = result.status;
            ebx = result.eno;
            break;
        }
        // Kill (ecx = pid, edx = sig, ebx = errno)
        case SYSCALL_KILL:
        {
            auto result = user_kill(ecx, edx);
            eax = result.status;
            ebx = result.eno;
            break;
        }
        // Kill (edx = sig, edi = func)
        case SYSCALL_SIGNAL:
            eax = (unsigned)user_signal(edx, (void(*)(int))edi);
            break;
    }
}

void user__exit(int exit_code)
{
    int m = int_disable();
    process_node_unlink(processes.first);
    int_restore(m);
    process_yield();
}
// Close a file.
int user_close(int file)
{
    return 0;
}
// Transfer control to a new process.
int user_execve(char *name, char **argv, char **env);
// Create a new process.
struct status_tuple user_fork()
{
    return { 1, EAGAIN };
}
// Status of an open file.
// For consistency with other minimal implementations in these examples,
// all files are regarded as character special devices.
// The `sys/stat.h' header file is distributed in the `include'
// subdirectory for this C library.
int user_fstat(int file, struct stat *st)
{
    st->st_mode = S_IFCHR;
    return 0;
}
// Query whether output stream is a terminal.
int user_isatty(int file)
{
    return 1;
}
// Send a signal.
struct status_tuple user_kill(int pid, int sig)
{
    return { -1, EINVAL };
}
// Establish a new name for an existing file.
struct status_tuple user_link(char *old, char *_new)
{
    return { -1, EMLINK };
}
// Set position in a file.
int user_lseek(int file, int ptr, int dir)
{
    return 0;
}
// Open a file.
int user_open(const char *name, int flags, mode_t mode)
{
    return -1;
}
// Read from a file.
int user_read(int file, char *ptr, int len)
{
    return 0;
}
// Increase program data space.
caddr_t user_sbrk(char *heap_end, int incr)
{
    return 0;
}
// Status of a file (by name).
int user_stat(const char *file, struct stat *st)
{
    st->st_mode = S_IFCHR;
    return 0;
}
// Timing information for current process.
clock_t user_times(struct tms *buf)
{
    return -1;
}
// Remove a file's directory entry.
struct status_tuple user_unlink(char *name)
{
    return { -1, ENOENT };
}
// Wait for a child process.
struct status_tuple user_wait(int *status)
{
    return { -1, ECHILD };
}
// Write a character to a file.
int user_write(int file, char *ptr, int len)
{
    if (file == 0)
    {
        printlen((const char*)ptr, len);
        return len;
    }
    return 0;
}
void (*user_signal(int sig, void (*func)(int)))(int)
{
    return func;
}
int user_map(void *ptr, int bytes)
{
    return 0;
}

} // extern "C"

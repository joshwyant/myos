#include "kernel.h"

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
        // End process
        case 0:
            m = int_disable();
            process_node_unlink(processes.first);
            int_restore(m);
            break;
        // Close a file.
        case 1:
            eax = 0;
            break;
        // Get environment variable count
        case 2:
            eax = 0;
            break;
        // Get the length of environment variable
        case 3:
            eax = 0;
            break;
        // Copy the environment variable number edx to ebx
        case 4:
            *((char*)ebx) = 0;
            break;
        // Get argument count
        case 5:
            eax = 0;
            break;
        // Get the length of an argument
        case 6:
            eax = 0;
            break;
        // Copy argument edx into location ebx
        case 7:
            *((char*)ebx) = 0;
            break;
        // Start a new process (ebx = *name, esi = **argv, edi = **env)
        case 8:
            eax = process_start((char*)ebx);
            break;
        // Get Process ID
        case 9:
            eax = current_process->pid;
            break;
        // Seek in a file (file edx to esi)
        case 10:
            eax = 0;
            break;
        // Read from a file (file edx, ptr edi, count ecx)
        case 11:
            eax = 0;
            break;
        // Write to a file (file edx, ptr esi, count ecx)
        case 12:
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
        case 13:
            eax = 0;
            break;
        // Clear the screen
        case 14:
            cls();
            break;
        // Yield to other processes
        case 15:
            process_yield();
            break;
    }
}

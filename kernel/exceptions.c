#include "kernel.h"

// Register exceptions into the IDT
void init_exceptions()
{
    // Division error
    register_trap(0x0,0,(void*)int0);
    // FPU task switch
    register_trap(0x7,0,(void*)int7);
    // Double fault
    register_trap(0x8,0,(void*)int8);
    // General protection fault
    register_trap(0xd,0,(void*)intd);
    // Page fault
    register_trap(0xe,0,(void*)inte);
}

// Called when a divide exception occurs
void divide_error(unsigned edi, unsigned esi, unsigned ebp, unsigned esp, unsigned ebx, unsigned edx, unsigned ecx, unsigned eax, unsigned eip, unsigned cs, unsigned eflags)
{
    dump_stack("A division error occured.", edi, esi, ebp, esp+12, ebx, edx, ecx, eax, eip, cs);
}

// Double fault handler
void double_fault(unsigned edi, unsigned esi, unsigned ebp, unsigned esp, unsigned ebx, unsigned edx, unsigned ecx, unsigned eax, unsigned errorcode, unsigned eip, unsigned cs, unsigned eflags)
{
    dump_stack("A double fault occured.", edi, esi, ebp, esp+16, ebx, edx, ecx, eax, eip, cs);
}

// General Protection Fault handler
void gpfault(unsigned edi, unsigned esi, unsigned ebp, unsigned esp, unsigned ebx, unsigned edx, unsigned ecx, unsigned eax, unsigned errorcode, unsigned eip, unsigned cs, unsigned eflags)
{
    char t[80];
    ksprintf(t, "A general protection fault occurred with error code %l.", errorcode);
    dump_stack(t, edi, esi, ebp, esp+16, ebx, edx, ecx, eax, eip, cs);
}

// Page fault handler, called after pusha in 'inte' handler
void pgfault(unsigned edi, unsigned esi, unsigned ebp, unsigned esp, unsigned ebx, unsigned edx, unsigned ecx, unsigned eax, unsigned errorcode, unsigned eip, unsigned cs, unsigned eflags)
{
    volatile unsigned int cr2;
    asm volatile ("mov %%cr2,%0":"=r"(cr2));
    if (((errorcode & 1) == 0) && (cr2 >= 0xc0000000))
    {
        unsigned t = cr2 >> 22;
        if (!(process_pdt[t]&1))
        {
            if (system_pdt[t]&1)
            {
                process_pdt[t] = system_pdt[t];
                invlpg((void*)cr2);
                goto nodump;
            }
        }
    }
    char t[100];
    ksprintf(t, "A page fault occurred at address %l, with error code %l.", cr2, errorcode);
    dump_stack(t, edi, esi, ebp, esp+16, ebx, edx, ecx, eax, eip, cs);
    nodump:
    return;
}

void bsod(const char* msg)
{
    int i;
    cli();
    show_cursor(0);
    cls_color(C_WHITE, C_BLUE);
    print(msg);
    do hlt(); while (1);
}

// Dump the stack
void dump_stack(const char* msg, unsigned edi, unsigned esi, unsigned ebp, unsigned esp, unsigned ebx, unsigned edx, unsigned ecx, unsigned eax, unsigned eip, unsigned cs)
{
    cli();
    show_cursor(0);
    cls_color(C_WHITE, C_BLUE);
    print(msg);
    print("\nDumping stack and halting CPU.\n");
    int i;
    volatile unsigned /*esp, ebp, esi, edi, eax, ebx, ecx, edx, cs,*/ ds, es, fs, gs, ss;
  /*asm volatile ("mov %%esp,%0":"=g"(esp));
    asm volatile ("mov %%ebp,%0":"=g"(ebp));
    asm volatile ("mov %%esi,%0":"=g"(esi));
    asm volatile ("mov %%edi,%0":"=g"(edi));
    asm volatile ("mov %%eax,%0":"=g"(eax));
    asm volatile ("mov %%ebx,%0":"=g"(ebx));
    asm volatile ("mov %%ecx,%0":"=g"(ecx));
    asm volatile ("mov %%edx,%0":"=g"(edx));
    asm volatile ("mov %%cs, %0":"=r"(cs));*/
    asm volatile ("mov %%ds, %0":"=r"(ds));
    asm volatile ("mov %%es, %0":"=r"(es));
    asm volatile ("mov %%fs, %0":"=r"(fs));
    asm volatile ("mov %%gs, %0":"=r"(gs));
    asm volatile ("mov %%ss, %0":"=r"(ss));
    kprintf("  ESP: %l  EBP: %l  ESI: %l  EDI: %l\n", esp, ebp, esi, edi);
    kprintf("  EAX: %l  EBX: %l  ECX: %l  EDX: %l\n", eax, ebx, ecx, edx);
    kprintf("  EIP: %l", eip);
    print("  CS: "); printhexw(cs);
    print("  DS: "); printhexw(ds);
    print("  ES: "); printhexw(es);
    print("  FS: "); printhexw(fs);
    print("  GS: "); printhexw(gs);
    print("  SS: "); printhexw(ss);
    // Print the stack
    for (i = 0; i < 20; i++, esp+=4)
    {
        int val;
        asm volatile("mov %%ss:(%1),%0":"=g"(val):"p"(esp));
        kprintf("\n  SS:%l %l", esp, val);
    }
    do hlt(); while (1);
}

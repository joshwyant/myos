#include "kernel.h"

// Based on https://wiki.osdev.org/Fpu

// If the EM bit is set, all FPU and vector operations will cause
// a #UD so they can be EMulated in software. Should be off to be
// actually able to use the FPU.
#define CR0_EM          (1 << 2)
// This bit is used on the 386 to tell it how to communicate with
// the coprocessor, which is 0 for an 287, and 1 for a 387 or later.
// This bit is hardwired on 486+.
#define CR0_ET          (1 << 4)
// When set, enables Native Exception handling which will use the
// FPU exceptions. When cleared, an exception is sent via the
// interrupt controller. Should be on for 486+, but not on 386s
// because they lack that bit.
#define CR0_NE          (1 << 5)
// Task switched. The FPU state is designed to be lazily switched
// to save read and write cycles. If set, all meaningful operations
// will cause a #NM exception so that the OS can backup the FPU state.
// This bit is automatically set on a hardware task switch, and can
// be cleared with the CLTS opcode. Software task switching may want
// to manually set this bit on a reschedule if they want to lazily
// store FPU state.
#define CR0_TS          (1 << 3)
// This does little else other than saying if an FWAIT opcode is
// exempted from responding to the TS bit. Since FWAIT will force
// serialisation of exceptions, it should normally be set to the
// inverse of the EM bit, so that FWAIT will actually cause a FPU
// state update when FPU instructions are asynchronous, and not when
// they are emulated.
#define CR0_MP          (1 << 1)
// Enables 128-bit SSE support. When clear, most SSE instructions
// will cause an invalid opcode, and FXSAVE and FXRSTOR will only
// include the legacy FPU state. When set, SSE is allowed and the
// XMM and MXCSR registers are accessible, which also means that your
// OS should maintain those additional registers. Trying to set this
// bit on a CPU without SSE will cause an exception, so you should
// check for SSE (or long mode) support first.
#define CR4_OSFXSR      (1 << 9)
// Enables the #XF exception. When clear, SSE will work until an
// exception is generated, after which all SSE instructions will
// fail with an invalid opcode. When set, the exception handler is
// called instead and the problem may be diagnosed and reported.
// Again, you can't set this bit without ensuring SSE support is
// present.
#define CR4_OSXMMEXCPT  (1 << 10)
// Enables the XSAVE extension, which is able to save SSE state as
// well as other next-generation register states. Again, check CPUID
// before setting: Long mode support is not sufficient in this case.
#define CR4_OSXSAVE     (1 << 18)

extern "C" {

Process *fpu_process = 0;

static void fpu_load_control_word(unsigned short control)
{
    asm volatile("fldcw %0;"::"m"(control)); 
}

static bool sse_enable = false;

void init_fpu()
{
    unsigned eax, ebx, ecx, edx;
    unsigned cr0, cr4;
    bool has_fpu = false;
    cpuid(CPUID_GETFEATURES, &eax, &ebx, &ecx, &edx);
    asm volatile ("mov %%cr0, %0":"=r"(cr0));
    if (edx & CPUID_FEAT_EDX_FPU)
    {
        has_fpu = true;
    }
    else if (!(cr0 & CR0_EM) && (cr0 & CR0_ET))
    {
        has_fpu = true;
    }
    else
    {
        // Start probe, get CR0
        asm volatile ("mov %%cr0, %0":"=r"(cr0));
        // clear TS and EM to force fpu access
        asm volatile ("mov %0, %%cr0"::"r"(cr0&~(unsigned)(CR0_TS|CR0_EM)));
        // Load defaults to FPU
        asm volatile ("fninit");
        short testword = -1;
        asm volatile ("fnstsw":"=m"(testword));
        if (testword == 0)
        {
            has_fpu = true;
        }
    }
    if (has_fpu)
    {
        // Set up control registers
        unsigned cr0_status_on = CR0_ET | CR0_NE | CR0_MP;
        unsigned cr0_status_off = CR0_EM | CR0_TS;
        unsigned cr4_status_on = 0;
        unsigned cr4_status_off = 0;
        if (edx & CPUID_FEAT_EDX_SSE)
        {
            sse_enable = true;
            cr4_status_on |= CR4_OSFXSR | CR4_OSXMMEXCPT;
        }
        else
        {
            cr4_status_off |= CR4_OSFXSR | CR4_OSXMMEXCPT;
        }
        if (ecx & CPUID_FEAT_ECX_OSXSAVE)
        {
            cr4_status_on |= CR4_OSXSAVE;
        }
        else
        {
            cr4_status_off |= CR4_OSXSAVE;
        }
        asm volatile ("mov %%cr0, %0":"=r"(cr0));
        asm volatile ("mov %0, %%cr0"::"r"((cr0&~(unsigned)cr0_status_off)|cr0_status_on));
        asm volatile ("mov %%cr4, %0":"=r"(cr4));
        asm volatile ("mov %0, %%cr4"::"r"((cr4&~(unsigned)cr4_status_off)|cr4_status_on));

        // Finish initialization
        asm volatile ("fninit");
        fpu_load_control_word(0x37A); // defaults plus enable division by zero and invalid operand exceptions.

        // Turn on TS flag so the first process using FPU can be identified
        asm volatile ("mov %0, %%cr0"::"r"(cr0|CR0_TS));
    }
}

void fpu_task_switch()
{
    if (fpu_process)
    {
        fpu_process->fpu_saved = true;
        asm volatile ("fsave %0":"=m"(fpu_process->fpu_file));
        
    }
    fpu_process = current_process;
    if (current_process && current_process->fpu_saved)
    {
        asm volatile ("frstor %0"::"m"(current_process->fpu_file));
        current_process->fpu_saved = false;
        
    }
    // Clear TS flag
    asm volatile ("clts");
}

}  // extern "C"

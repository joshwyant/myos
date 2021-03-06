#ifndef __EXCEPTIONS_H__
#define __EXCEPTIONS_H__

#ifdef __cplusplus
extern "C" {
#endif

extern void init_exceptions();

extern void int0(); // divide error
extern void int7(); // FPU task switch
extern void int8(); // double fault
extern void intd(); // gpf
extern void inte(); // pgf

extern void divide_error(unsigned edi, unsigned esi, unsigned ebp, unsigned esp, unsigned ebx, unsigned edx, unsigned ecx, unsigned eax, unsigned eip, unsigned cs, unsigned eflags);
extern void gpfault(unsigned edi, unsigned esi, unsigned ebp, unsigned esp, unsigned ebx, unsigned edx, unsigned ecx, unsigned eax, unsigned errorcode, unsigned eip, unsigned cs, unsigned eflags);
extern void pgfault(unsigned edi, unsigned esi, unsigned ebp, unsigned esp, unsigned ebx, unsigned edx, unsigned ecx, unsigned eax, unsigned errorcode, unsigned eip, unsigned cs, unsigned eflags);
extern void dobule_fault(unsigned edi, unsigned esi, unsigned ebp, unsigned esp, unsigned ebx, unsigned edx, unsigned ecx, unsigned eax, unsigned errorcode, unsigned eip, unsigned cs, unsigned eflags);

extern void dump_stack(const char*, unsigned, unsigned, unsigned, unsigned, unsigned, unsigned, unsigned, unsigned, unsigned, unsigned);
extern void bsod(const char*);

#ifdef __cplusplus
}  // extern "C"
#endif

#endif  // __EXCEPTIONS_H__

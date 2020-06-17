#ifndef __SYSCALL_H__
#define __SYSCALL_H__

#ifdef __cplusplus
extern "C" {
#endif

extern void init_syscalls();
extern void syscall(unsigned edi, unsigned esi, unsigned ebp, unsigned esp, unsigned ebx, unsigned edx, unsigned ecx, unsigned eax);
extern void int30(); // syscall

#ifdef __cplusplus
}  // extern "C"
#endif

#endif  // __SYSCALL_H__
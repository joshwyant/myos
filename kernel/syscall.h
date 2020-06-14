#ifndef __SYSCALL_H__
#define __SYSCALL_H__

extern void init_syscalls();
extern void syscall(unsigned edi, unsigned esi, unsigned ebp, unsigned esp, unsigned ebx, unsigned edx, unsigned ecx, unsigned eax);
extern void int30(); // syscall

#endif  // __SYSCALL_H__
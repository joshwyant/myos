#ifndef __SYSCALLS_H__
#define __SYSCALLS_H__

int _strlen(const char* s)
{
    int c = 0;
    while (*s++) c++;
    return c;
}

inline int write(int file, const char* ptr, int len)
{
    int retval;
    asm volatile ("int $0x30":"=a"(retval):"a"(12),"S"(ptr),"c"(len),"d"(file));
    return retval;
}

inline void cls()
{
    asm volatile ("int $0x30"::"a"(14));
}

inline void process_yield()
{
    asm volatile ("int $0x30"::"a"(15));
}

inline void print(const char* msg) {
    write(0, msg, _strlen(msg));
}

#endif /* __SYSCALLS_H__ */

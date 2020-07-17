#ifndef __SYSCALLS_H__
#define __SYSCALLS_H__

static int _strlen(const char* s)
{
    int c = 0;
    while (*s++) c++;
    return c;
}

static inline int write(int file, const char* ptr, int len)
{
    int retval;
    asm volatile ("int $0x30":"=a"(retval):"a"(12),"S"(ptr),"c"(len),"d"(file));
    return retval;
}

static inline void cls()
{
    asm volatile ("int $0x30"::"a"(14));
}

static inline void process_yield()
{
    asm volatile ("int $0x30"::"a"(15));
}

static inline void print(const char* msg) {
    write(0, msg, _strlen(msg));
}

#endif /* __SYSCALLS_H__ */

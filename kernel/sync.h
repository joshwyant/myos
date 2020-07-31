#ifndef __KERNEL_SYNC_H__
#define __KERNEL_SYNC_H__

#ifdef __cplusplus

extern "C" {
#endif

// Spins while value int ptr is set and then sets 1 in the pointer.
static inline void spinlock(int* ptr)
{
    int prev;
    // This seems to be the perfect way to do it.
    // The Intel manual says the LOCK prefix is alway assummed
    // with the xchg instruction with mem op, so we can save a byte by not
    // using the prefix.
    // >> SAME AS XCHG
    do asm volatile ("lock xchgl %0,%1":"=a"(prev):"m"(*ptr),"a"(1)); while (prev);
}

// Sets the value, and returns whether it was previously set.
// Allows multiple threads to wait for a value to be unset
static inline int lock(int* ptr)
{
    int prev;
    // >> SAME AS XCHG
    asm volatile ("lock xchgl %0,%1":"=a"(prev):"m"(*ptr),"a"(1));
    return prev;
}

// process.c
extern void	process_yield();

#ifdef __cplusplus
}  // extern "C"

namespace kernel
{
// RAII wrapper around lock()
class ScopedLock
{
public:
    inline ScopedLock(int& val)
        : ptr(&val)
    {
        if (lock(ptr)) [[unlikely]]
        {
            process_yield();
            while (lock(ptr)) [[unlikely]] process_yield(); // If this works, we don't need the if
                                                            // otherwise, use: do process_yield(); while(lock(ptr));
        }
    }
    ~ScopedLock()
    {
        *ptr = 0;
    }
private:
    int *ptr;
};
} // namespace kernel
#endif  // __cplusplus
#endif // __KERNEL_SYNC_H__

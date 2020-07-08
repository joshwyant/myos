// Based on https://wiki.osdev.org/C%2B%2B
//   and on https://wiki.osdev.org/Calling_Global_Constructors

#include <stddef.h>
#include "kernel.h"

extern "C" void __cxa_pure_virtual()
{
    // Do nothing or print an error message.
	print("Pure virtual function called erroneously in kernel.\n");  // TODO: remove
}

void *operator new(size_t size) 
{
    return kmalloc(size);
}
 
void *operator new[](size_t size)
{
    return kmalloc(size);
}
 
void operator delete(void *p)
{
    kfree(p);
}
 
void operator delete(void *p, size_t size)
{
    kfree(p);
}
 
void operator delete[](void *p)
{
    kfree(p);
}

namespace __cxxabiv1 
{
	/* guard variables */
 
	/* The ABI requires a 64-bit type.  */
	__extension__ typedef int __guard __attribute__((mode(__DI__)));
 
	extern "C" int __cxa_guard_acquire (__guard *);
	extern "C" void __cxa_guard_release (__guard *);
	extern "C" void __cxa_guard_abort (__guard *);
 
	extern "C" int __cxa_guard_acquire (__guard *g) 
	{
		return lock((int*)g) == 0;
	}
 
	extern "C" void __cxa_guard_release (__guard *g)
	{
		*(int *)g = 0;
	}
 
	extern "C" void __cxa_guard_abort (__guard *)
	{
 
	}
}

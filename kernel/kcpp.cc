// Based on https://wiki.osdev.org/C%2B%2B
//   and on https://wiki.osdev.org/Calling_Global_Constructors

#include <stddef.h>
#include <reent.h>
#include "kernel.h"

extern "C" {
	
void __cxa_pure_virtual()
{
    // Do nothing or print an error message.
	print("Pure virtual function called erroneously in kernel.\n");  // TODO: remove
}

void *memcpy(void *dest, const void *src, size_t bytes)
{
	kmemcpy(dest, src, bytes);
	return dest;
}

void *malloc(size_t bytes)
{
	return kmalloc(bytes);
}

void free(void *ptr)
{
	return kfree(ptr);
}

void abort()
{
	print ("ABORT\n");
	do hlt(); while (1);
}

void *realloc(void *ptr, size_t size)
{
	return krealloc(ptr, size);
}

int strcmp(const char * str1, const char * str2)
{
	return kstrcmp(str1, str2);
}

int sprintf(char *str, const char *format, ...)
{
	str[0] = '\0';
	// TODO
	return 0;
}

int fputc (int character, void /*FILE*/ *stream)
{
	print_char(character);
	return character;
}

size_t fwrite(const void *ptr, size_t size, size_t count, void /*FILE*/ *stream)
{
	for (int i = 0; i < size * count; i++)
	{
		print_char(((char*)ptr)[i]);
	}
	return count;
}

size_t strlen(const char * str)
{
	return kstrlen(str);
}

int fputs(const char * str, void /*FILE*/ *stream)
{
	int count = 0;
	while (*str)
	{
		print_char(*str++);
		count++;
	}
	return count;
}

// Stubs from newlib
static struct _reent impure_data = _REENT_INIT (impure_data);
struct _reent *_impure_ptr __ATTRIBUTE_IMPURE_PTR__ = &impure_data;

} // extern "C"

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

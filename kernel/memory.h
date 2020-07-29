#ifndef __MEMORY_H__
#define __MEMORY_H__

#include "../include/loader_info.h"

#undef LIKELY
#undef UNLIKELY
#ifdef __cplusplus
#define LIKELY [[likely]]
#define UNLIKELY [[unlikely]]
#else
#define LIKELY
#define UNLIKELY
#endif

// defines
#define PF_NONE		0
#define PF_READ		0
#define PF_SUPERVISOR	0
#define PF_PRESENT	1<<0
#define PF_WRITE	1<<1
#define PF_USER		1<<2
#define PF_WRITETHROUGH	1<<3
#define PF_CACHEDISABLE 1<<4
#define PF_ACCESSED	1<<5
#define PF_DIRTY	1<<6
#define PF_PAT		1<<7
#define PF_GLOBAL	1<<8
#define PF_AVAIL1	1<<9
#define PF_AVAIL2	1<<10
#define PF_AVAIL3	1<<11

#define PF_LOCKED	PF_AVAIL1

#ifdef __cplusplus
#include <stddef.h>
#include <utility>

extern "C" {
#endif

// count of RAM
extern unsigned int total_memory;

// Paging
extern void* page_bitmap;
extern void* page_directory;
extern int page_bitmap_size;

// Memory Allocation
extern void* heap_start;
extern void* heap_end;
extern void* heap_brk;
extern void* first_free;
extern void* last_free;

// Memory
static inline void* palloc(int,void*,void*);
static inline void  mark_pages(void*, int, int);
static inline void  mark_page(void*, int);

extern int ksbrk(int);
extern void* kfindrange(int size);
extern void page_map(void *logical, void *physical, unsigned flags);
extern void page_unmap(void* logical);
extern void* get_physaddr(void* logical);
extern void init_paging(loader_info *li);
extern void* kmalloc(int size);
extern void* kcalloc(int size);
extern void *krealloc(void *ptr, int size);
// Duplicates a heap object
extern void *kdupheap(void *ptr);
extern void kfree(void* ptr);
extern void page_free(void* addr, int count);
extern void* page_alloc(int size);
extern void* extended_alloc(int size);
extern void* base_alloc(int size);

extern void init_heap();
extern void init_paging(loader_info *li); // nonstatic so compiler doesn't optimize it, and we can jump right over it in bochs

static inline void kmemcpy(void* dest, const void* src, unsigned bytes)
{
    if (!dest || !src) UNLIKELY return;

    // Undefined for backwards overlapped case.

    unsigned words = bytes / 4;
    unsigned rem = bytes %= 4;

    if (words) LIKELY
    {
        asm volatile (
            "rep; movsl":
            "=c"(words),"=S"(src),"=D"(dest):
            "c"(words),"S"(src),"D"(dest)
        );
    }

    if (rem) UNLIKELY
    {
        asm volatile (
            "rep; movsb":
            "=c"(rem),"=S"(src),"=D"(dest):
            "c"(rem),"S"(src),"D"(dest)
        );
    }
}
static inline void kmemmove(void* dest, const void* src, unsigned bytes)
{
    if (!dest || !src) UNLIKELY return;

    if (dest > src) UNLIKELY  // Optimize for overlapped case
    {
        kmemcpy(dest, src, bytes); // Use this because it's faster
        return;
    }
    // Backwards, potentially overlapped case:
    dest = (char*)dest + bytes - 1;
    src = (char*)src + bytes - 1;
    asm volatile (
        "std; rep; movsb": // using movsl first is complicated
        "=c"(bytes),"=S"(src),"=D"(dest):
        "c"(bytes),"S"(src),"D"(dest)
    );
}
static inline void kzeromem(void* dest, unsigned bytes)
{
    if (dest) LIKELY
    {
        unsigned words = bytes / 4;
        bytes %= 4;
        if (words) LIKELY
        {
            asm volatile (
                "cld; rep; stosl":
                "=c"(words),"=D"(dest):
                "c"(words),"D"(dest),"a"(0)
            );
        }
        if (bytes) UNLIKELY
        {
            asm volatile (
                "cld; rep; stosb":
                "=c"(bytes),"=D"(dest):
                "c"(bytes),"D"(dest),"a"(0)
            );
        }
    }
}

#ifdef __cplusplus
}  // extern "C"

namespace kernel
{
// Safe, RAII wrapper around page_map
template <typename T = char>
class MappedMemory
{
public:
    MappedMemory()
        : ptr(nullptr), bytes(0) {}
    MappedMemory(T *logical, void *physical, unsigned flags, unsigned bytes = 4096)
        : ptr(logical), bytes(bytes)
    {
        for (unsigned t = 0; t < bytes; t += 4096)
        {
            page_map((char*)(void*)ptr+t, (char*)physical+t, flags);
        }
    }
    MappedMemory(void *physical, unsigned flags, unsigned bytes = 4096)
        : MappedMemory((T*)kfindrange(bytes), physical, flags) {}
    MappedMemory(MappedMemory&) = delete;
    MappedMemory(MappedMemory&& other) noexcept
        : MappedMemory()
    {
        swap(*this, other);
    }
    MappedMemory& operator=(MappedMemory&& other) // move assign
    {
        swap(*this, other);
        return *this;
    }
    friend void swap(MappedMemory& a, MappedMemory& b)
    {
        using std::swap;
        swap(a.ptr, b.ptr);
        swap(a.bytes, b.bytes);
    }
    ~MappedMemory()
    {
        if (ptr == nullptr || bytes <= 0) return;
        for (unsigned t = 0; t < bytes; t += 4096)
        {
            page_unmap((char*)(void*)ptr+t);
        }
    }
    const T *get() const { return ptr; }
	T *get() { return const_cast<T*>(static_cast<const MappedMemory&>(*this).get()); }
	T& operator[](int index)
	{
		return const_cast<T&>(static_cast<const MappedMemory&>(*this)[index]);
	}
	const T& operator[](int index) const
	{
		if (index < 0 || index * sizeof(T) >= bytes) [[unlikely]]
		{
			throw OutOfBoundsError();
		}
		return ptr[index];
	}
private:
    T *ptr;
    unsigned bytes;
}; // class MappedMemory
} // namespace kernel

#endif

#endif  // __MEMORY_H__

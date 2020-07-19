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

#ifdef __cplusplus
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




#endif

#endif  // __MEMORY_H__

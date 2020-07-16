#include "kernel.h"

// count of RAM
unsigned int total_memory;

// Paging
void* page_bitmap;
void* page_directory;
int page_bitmap_size;

// Memory Allocation
void* heap_start;
void* heap_end;
void* heap_brk;
void* first_free;
void* last_free;

// Spinlocks
static int	heap_busy = 0;
static int	page_map_locked = 0;
static int	palloc_lock = 0;

// Takes a tag and tells if it's free
static inline int ktagisfree(unsigned tag)
{
    return (tag & 0x01000000) == 0;
}

// Uses the tag to tell if an address is at the beginning of the heap
static inline int ktagisbegin(unsigned tag)
{
    return (tag & 0x02000000) != 0;
}

// Tag is at the end of the heap
static inline int ktagisend(unsigned tag)
{
    return (tag & 0x04000000) != 0;
}

// The size of the block
static inline int ktagsize(unsigned tag)
{
    return (tag & 0x00FFFFFF) * 16;
}

// The size of the block, excluding the boundary tags
static inline int ktaginsize(unsigned tag)
{
    return (tag & 0x00FFFFFF) * 16 - 8;
}

// Returns the tag for the memory address
static inline unsigned ktag(void* ptr)
{
    return *((unsigned*)(ptr+(unsigned)(-4)));
}

// Sets the tags (tag must have the right length encoded)
static inline unsigned ksettag(void* ptr, unsigned tag)
{
    *((unsigned*)(ptr+(unsigned)(-4))) = tag;
    *((unsigned*)(ptr+ktaginsize(tag))) = tag;
}

// Makes a tag
static inline unsigned kmaketag(int blocksize, int used, int begin, int end)
{
    unsigned tag = (blocksize/16);
    if (used) tag |= 0x01000000;
    if (begin) tag |= 0x02000000;
    if (end) tag |= 0x04000000;
    return tag;
}

// The tag of the previous block
static inline unsigned ktagprev(void* ptr)
{
    return *((unsigned*)(ptr+(unsigned)(-8)));
}

// The tag of the next block
static inline unsigned ktagnext(void* ptr)
{
    return *((unsigned*)(ptr+ktagsize(ktag(ptr))));
}

// The pointer to the previous adjacent block
static inline void* kadjprev(void* ptr)
{
    return ptr+(unsigned)(-ktagsize(ktagprev(ptr)));
}

// The pointer to the next adjacent block
static inline void* kadjnext(void* ptr)
{
    return ptr+ktagsize(ktag(ptr));
}

// Returns the pointer previous to this free pointer 
static inline void* kprevfree(void* ptr)
{
    return ((void**)ptr)[0];
}

// returns the next free pointer
static inline void* knextfree(void* ptr)
{
    return ((void**)ptr)[1];
}

// mark the memory tag's used bit
static inline void kuseptr(void* ptr, int use)
{
    if (use)
        ksettag(ptr, ktag(ptr) | 0x01000000);
    else
        ksettag(ptr, ktag(ptr) & 0xFEFFFFFF);
}

static inline void kptrlink(void* ptr)
{
    ((void**)ptr)[0] = last_free;
    ((void**)ptr)[1] = 0;
    if (last_free)
        ((void**)last_free)[1] = ptr;
    last_free = ptr;
    if (!first_free)
        first_free = ptr;
}

static inline void kptrunlink(void* ptr)
{
    void* prev = ((void**)ptr)[0];
    void* next = ((void**)ptr)[1];

    if (prev)
        ((void**)prev)[1] = next;
    else
        first_free = next;

    if (next)
        ((void**)next)[0] = prev;
    else
        last_free = prev;
}

void page_map(void *logical, void *physical, unsigned flags)
{
    volatile unsigned* dir     = (volatile unsigned*)0xFFFFF000; // The page directory's address
    volatile unsigned* tbl     = (volatile unsigned*)0xFFC00000+(((unsigned)logical>>12)&~0x3FF); // The page table's address
    unsigned           dir_ind = ((unsigned)logical>>22);       // The index into the page directory
    unsigned           tbl_ind = ((unsigned)logical>>12)&0x3FF; // The index into the page table
    // If a page table doesn't exist
    while (lock(&page_map_locked)) process_yield();
    if (!(dir[dir_ind]&1))
    {
        // Page table
        void* ptr = page_alloc(1);
        if (!ptr) bsod("Insufficient memory.");
        dir[dir_ind] = (unsigned)ptr+(PF_PRESENT|PF_WRITE|(logical < (void*)0xC0000000 ? PF_USER : 0)); // add to directory
        // wipe table (after we mapped it! stumped me for about 5 minutes...)
        int i;
        for (i = 0; i < 1024; i++) tbl[i] = 0;
        invlpg((void*)(unsigned)tbl);
    }
    page_map_locked = 0;
    tbl[tbl_ind] = ((unsigned)physical&~0xFFF)+(flags|PF_PRESENT);
    // Invalidate the page
    invlpg((void*)(((unsigned)logical)&~0xFFF));
}

void page_unmap(void* logical)
{
    volatile unsigned* dir     = (volatile unsigned*)0xFFFFF000; // The page directory's address
    volatile unsigned* tbl     = (volatile unsigned*)0xFFC00000+(((unsigned)logical>>12)&~0x3FF); // The page table's address
    unsigned           dir_ind = ((unsigned)logical>>22);       // The index into the page directory
    unsigned           tbl_ind = ((unsigned)logical>>12)&0x3FF; // The index into the page table
    if (!(dir[dir_ind]&1)) return; // Page table is not present, we needn't do any unmapping.
    tbl[tbl_ind] = 0; // Unmap it
    // Remove an empty page table if this is not a kernel page table (is below 0xC0000000).
    // Do not remove empty kernel page tables, to make it easy to map the kernel in user processes,
    // because we don't have to worry about a mapped page table becoming invalid because the kernel used
    // a new page table in a different process! This can only make up to 1 megabyte of unused memory,
    // and chances are, that memory will be used later.
    if (logical < (void*)0xc0000000)
    {
        int found = 0, i;
        // If the table is empty, free the table.
        for (i = 0; i < 1024; i++) if (tbl[i]&1) { found = 1; break; }
        if (!found)
        {
            // THE BUG: freed tbl instead of its physical address, and there was only the page_free call, besides the page_unmap.
            // dir[dir_ind] = 0;
            // page_free((void*)tbl, 1);
            // invlpg((void*)(unsigned)tbl);
            while (lock(&page_map_locked)) process_yield();
            page_free((void*)(dir[dir_ind]&0xFFFFF000), 1);
            dir[dir_ind] = 0;
            page_map_locked = 0;
            page_unmap((void*)tbl);
            invlpg((void*)(unsigned)tbl);
        }
    }
    // Invalidate the page
    invlpg(logical);
}

// Returns a free virtual address range of 'size' bytes
void* kfindrange(int size)
{
	// Kernel pages that are not code, heap, bss, etc. are mapped in the top
	// 252MB(0xF0000000-0xFFBFFFFF). The remaining 4mb is for page tables
	// (starting at 0xFFC00000), and the page directory itself is at 0xFFFFF000.
	//
	// The page tables for our designated 0xF0000000 (256M) range are actually stored 
	// in the highest 1/16 of the page table memory: 0xFFFC0000-0xFFFFF000.
	
	// Start enumerating the page tables for the designated range (0xF0000000+).
    volatile void* volatile* p = (volatile void* volatile*)0xFFFC0000; // Pointer to a PTE
    void* ptr; // pointer to range, what we will eventually return
    int bytes = 0; // bytes found
    static int locked = 0;
    while (lock(&locked)) process_yield();
    while ((void*)p < (void*)0xFFFFF000)
    {
        // Addr = (p-0xFFFC0000)*4096/4+0xF0000000
        if (!bytes) ptr = (void*)((((unsigned)p-0xFFFC0000)<<12)/4+0xF0000000);
        // Is there a page table for this range?
        if (!(*((volatile unsigned*)0xFFFFF000+((unsigned)ptr>>22))&1))
        {
            // No; A whole 4MB range of memory is mappable.
            bytes += 0x400000;
            p += 1024; // Skip over these 1024 missing page table entries.
        }
        else
        {
            bytes += 4096; // Increment available bytes,
            if (((unsigned)*p)&1)
            {
				// but only if we have contiguous free space.
                bytes = 0;
                ptr = 0;
            }
            p++; // move past this page table entry.
        }
        if (bytes >= size)
        {
            // commit pages TODO quick solution, make this better
            int i;
            //for (i = 0; i < size; i++)
            //    page_map(ptr+i*4096,0,PF_NONE); // Temporarily map the pages to 0 to keep kfindrange from reallocating them in another process
            locked = 0; // unlock spinlock
            return ptr;
        }
    }
    locked = 0;
    return 0;
}

// Gets the physical address represented by the given virtual address
void* get_physaddr(void* logical)
{
    unsigned pgdir_index = (unsigned)logical>>22;
    unsigned pgtbl_index = ((unsigned)logical>>12)&0x3FF;
    unsigned offset = (unsigned)logical&0xFFF;
    unsigned* pgdir = (unsigned*)0xFFFFF000;
    if (!(pgdir[pgdir_index]&1)) return 0;
    unsigned* pgtbl = (unsigned*)(0xFFC00000+0x1000*pgdir_index);
    if (!(pgtbl[pgtbl_index]&1)) return 0;
    return (void*)((pgtbl[pgtbl_index]&0xFFFFF000)|offset);
};

void init_paging(loader_info *li)
{
    // The system memory map was stored by the kernel loader,
    // using the BIOS, at address 0xF0001000. It marks free
    // and reserved memory areas. x is the pointer to
    // the system memory map. next_page is defined by
    // kernel.ld, and marks the end of reserved
    // kernel memory rounded up to a page bounary.
    // page_bitmap is not rounded up.
    // memory map layout:
    //   entry,entry,entries...,times 6 dd 0
    // entry:
    //   uint64 start;
    //   uint64 size;
    //   uint32 type;
    //   uint32 flags;
    volatile unsigned int* x = (volatile unsigned int*)li->memmap, *px = x; // pointer to memory map
    void* ptr;
    total_memory = 0;
    // determine maximum amount of free memory.
    while (1)
    {
        if ((px[0]|px[1]|px[2]|px[3]|px[4]|px[5]) == 0) break;
        if (px[5]&1) // bit 0 not set means ignore
        {
            switch (px[4])
            {
            case 1: // Free memory
            case 3: // ACPI reclaim memory
                if ((px[0]+px[2]) > total_memory) total_memory = px[0]+px[2];
                break;
            }
        }
        px += 6;
    }
    px = x;
    page_bitmap = (void*)(((unsigned int)li->loaded+li->memsize+3)&~3);
    page_bitmap_size = total_memory >> 15;
    // Cautiously (right now it's safe to do) map in the pages needed for the page bitmap.
    // It's safe because most likely, page_map wont't allocate a new page table. We have
    // to extend the image beyond the 1MB mark instead of using palloc right now.
    unsigned kend = (unsigned)page_bitmap+page_bitmap_size;
    unsigned kpages = (li->memsize+0xFFF)>>12;
    unsigned tpages = (kend-0xC0000000+0xFFF)>>12;
    volatile int i;
    for (i = kpages; i < tpages; i++)
        page_map((void*)0xC0000000+(i<<12), li->freemem+((i-kpages)<<12), PF_LOCKED|PF_WRITE);
    // Initialize physical memory bitmap.
    // Number of pages in memory = mem/4k   = mem>>12
    // Number of uints in bitmap = pages/32 = mem>>17
    // Number of bytes in bitmap = pages/8  = mem>>15
    // Wipe bitmap
    for (i = 0; i < total_memory>>17; i++)
        *((unsigned int*)page_bitmap+i) = 0;
    while (1) // Mask used memory using memory map
    {
        if ((px[0]|px[1]|px[2]|px[3]|px[4]|px[5]) == 0) break;
        if (px[5]&1) // bit 0 not set means ignore
        {
            switch (px[4])
            {
            case 1: // Free memory
            case 3: // ACPI reclaim memory
                break;
            default:
                if (px[0] >= total_memory) break;
                // mark used pages
                ptr = (void*)(px[0]&~0xFFF);
                for (i = 0; ptr < (void*)px[2]; i++, ptr += 0x1000)
                    mark_page(ptr,1);
                break;
            }
        }
        px += 6;
    }
    // PAGING is now stable enough AT THIS POINT to MARK PAGES:
    // Mark other pages as used:
    //   IVT, BDA, Free
    mark_page(0,1);
    //   Boot sector
    mark_page((void*)0x7000,1);
    //   Video memory, vga rom, bios rom
    mark_pages((void*)0xA0000,96,1);
    //   kernel image, page bitmap
    ptr = li->loaded;
    for (i = 0; i < tpages; i++, ptr += 0x1000)
        mark_page(get_physaddr(ptr), 1);
    //   (Whoops, don't forget these! >>>)
    //   kernel page tables
    ptr = (void*)0xFFC00000;
    // Optimization changes i < 1024 to ptr < 0x100000000, but due to 32 bits, it wraps to 0 and the condition is optimized out.
    // Change to volatile?
    for (i = 0; i < 1024; i++, ptr += 0x1000)
    {
        void* ph = get_physaddr(ptr);
        if (ph) mark_page(ph, 1);
    }
    //   (or the stack! 8-23-2008)
    mark_page(get_physaddr((void*)0xF0000000), 1);
    // PAGING is now setup enough AT THIS POINT to ALLOCATE AND FREE PAGES, AND MAPPING (MAPPING MAY ALLOCATE OR FREE PAGE TABLES):
    // Unmap first megabyte
    for (ptr = (void*)0x00000000; ptr < (void*)0x00100000; ptr+=0x1000)
        page_unmap(ptr);
    // Map a copy of the system's Page Directory Table
    system_pdt = kfindrange(4096);
    page_map((void*)system_pdt, get_physaddr((void*)0xFFFFF000), PF_LOCKED|PF_WRITE);
}

void init_heap()
{
    heap_start = heap_end = heap_brk = (void*)0xD0000000;
    first_free = last_free = 0;
}

// Adds n pages to the heap. Returns the actual number of pages added.
// ONLY CALLED IN KMALLOC
int ksbrk(int n)
{
    int i;
    for (i = 0; i < n; i++)
    {
        // Heap's range is from 0xD0000000 - 0xE0000000
        if ((heap_brk + 4096) > (void*)0xE0000000) return i;
        void* pg = page_alloc(1);
        if (!pg) return i;
        page_map(heap_brk, pg, PF_WRITE);
        heap_brk += 4096;
    }
    return i;
}

// KMALLOC ONLY
static void* kbestfreeblock(int size)
{
    void* best = 0;
    void* ptr = first_free;
    while (ptr)
    {
        int s = ktaginsize(ktag(ptr));
        if (size < s && ((best == 0) || s < ktaginsize(ktag(best)))) best = ptr;
        ptr = knextfree(ptr);
    }
    return best;
}

static void* kfirstfreeblock(int size)
{
    void* ptr = first_free;
    while (ptr)
    {
        if (size < ktaginsize(ktag(ptr))) return ptr;
        ptr = knextfree(ptr);
    }
    return 0;
}

void *krealloc(void *ptr, int size)
{
    void *newptr = kmalloc(size);
    kmemcpy(newptr, ptr, ktaginsize(ktag(ptr)));
    return newptr;
}

void *kcalloc(int size)
{
    void *ptr = kmalloc(size);
    kzeromem(ptr, size);
    return ptr;
}

void* kmalloc(int size)
{
    // ALLOCATIONS:
    // All aligned 4 bytes.
    // All must be at least 16 bytes.
    // Free list points to first and last free blocks.
    // All blocks have boundary tags.
    // Boundary tag is a 32-bit integer.
    // Top byte is flags. Bottom 24 bits is block size/16
    // Flag bits:
    // 0: used bit
    // 1: start of memory flag
    // 2: end of memory flag
    // The block + 4 bytes is returned.
    // The interior of a block is the block minus the boundary tags.
    // A block of 16 bytes at address 0xD0000000:
    // kmalloc(8) = 0xD0000004
    // 10 00 00 03 ?? ?? ?? ?? ?? ?? ?? ?? 10 00 00 03
    // Two freed adjacent blocks:
    // 10 00 00 02 00 00 00 00 14 00 00 D0 10 00 00 02 - 10 00 00 00 04 00 00 D0 00 00 00 00 10 00 00 00
    // The previous- and next-free pointers located at bytes 4 and 8 of the block, respectively.
    // first_free and last_free store user pointers, like 0xD0000004. So do prev- and next-free.
    // If there is no free block, the pointer is 0.
    // The kernel heap is at 0xD0000000. It has address space for up to 256 megabytes.
    // The size of each block is perfect in this case. There can be up to 16777216 16-byte blocks in 256MB.
    // 16777216 is 2^24, the same as the size value in the tag. There is room for flags in a byte.
    // 2 tags + 2 pointers = 16 bytes.
    while (lock(&heap_busy)) process_yield();
    void* ptr = kbestfreeblock(size); // faster: kfirstfreeblock(int)
    int m = (size + 23) & 0xFFFFFFF0; // Minimum size needed for a block. ((size + 8) + 15) / 16 * 16;
    if (ptr)
    {
        // We've got the best free block. Just unlink it and return it. And don't forget to break it up if needed.
        kuseptr(ptr, 1);
        kptrunlink(ptr);
        if (m != ktagsize(ktag(ptr)))
        {
            unsigned tag = ktag(ptr); // tag is the current tag.
            int s = ktagsize(tag);    // s = the actual size of the tag.
            ksettag(ptr+m, kmaketag(s-m, 0, 0,   ktagisend(tag))); // A new tag to be made. ptr+m = the address of the new tag
            ksettag(ptr,   kmaketag(m,   1, ktagisbegin(tag), 0)); // Set the tags for our resized block
            // link in the new block because it's free
            kptrlink(ptr+m);
        }
        heap_busy = 0;
        return ptr;
    }
    else
    {
        // There are no free blocks; Try and expand the heap and make a new block.
        void* t = heap_end;
        while (t+m > heap_brk) if (!ksbrk(1)) { heap_busy = 0; return 0; }
        heap_end = t+m;
        ptr = t+4;
        ksettag(ptr, kmaketag(m, 1, ptr == (void*)0xD0000004, 1));
        if (ptr != (void*)0xD0000004)
            ksettag(kadjprev(ptr), ktag(kadjprev(ptr))&0xFBFFFFFF); // The previous block is no longer at the end of the memory.
        heap_busy = 0;
        return ptr;
    }
}

void kfree(void* ptr)
{
    while (lock(&heap_busy)) process_yield();
    // Unlink any adjacent free blocks and coalesce them.
    int s = ktagsize(ktag(ptr)); // The total size of the free block
    void* ptr2 = ptr; // The leftmost adjacent free block
    void* ptr3 = ptr; // The rightmost adjacent free block
    // If this is not the first block and the previous block is free
    if (!ktagisbegin(ktag(ptr)) && ktagisfree(ktag(kadjprev(ptr))))
    {
        ptr2 = kadjprev(ptr);
        kptrunlink(ptr2);
        s += ktagsize(ktag(ptr2));
    }
    // If this is not the last block and the next block is free
    if (!ktagisend(ktag(ptr)) && ktagisfree(ktag(kadjnext(ptr))))
    {
        ptr3 = kadjnext(ptr);
        kptrunlink(ptr3);
        s += ktagsize(ktag(ptr3));
    }
    if (ktagisend(ktag(ptr3)))
    {
        if (!ktagisbegin(ktag(ptr2))) ksettag(kadjprev(ptr2), ktag(kadjprev(ptr2))|0x04000000); // Make the previous block the end of memory
        heap_end = ptr2 + (unsigned)(-4);
    }
    else
    {
        ksettag(ptr2, kmaketag(s, 0, ktagisbegin(ktag(ptr2)), ktagisend(ktag(ptr3))));
        kptrlink(ptr2);
    }
    heap_busy = 0;
}

// Allocates a number of pages in the given range
// FOR USE IN PAGE_ALLOC FUNCTIONS
static inline void* palloc(int size,void* start,void* end)
{
    while (lock(&palloc_lock)) process_yield();
    void register *ptr = page_bitmap+((unsigned int)start>>15), *ptr2 = page_bitmap+((unsigned int)end>>15), *startptr;
    int i, count = 0;
    unsigned char volatile bit;
    while (ptr < ptr2)
    {
        if (*(unsigned int*)ptr == 0xFFFFFFFF)
        {
            count = 0;
            ptr += 4;
            continue;
        }
        bit = 1;
        for (i = 0; i < 8; i++, bit <<= 1)
        {
            if ((*(unsigned char*)ptr)&bit)
                count = 0;
            else
            {
                if (count == 0) startptr = (void*)(((((unsigned int)ptr-(unsigned int)page_bitmap)<<3)+i)<<12);
                count++;
                if (count >= size) goto found;
            }
        }
        ptr++;
    }
    palloc_lock = 0;
    return (void*)0;
found:
    mark_pages(startptr, size, 1);
    palloc_lock = 0;
    return startptr;
}

// Marks the given page range as used or free.
// DON'T USE OUTSIDE OF PALLOC OR INIT_PAGING
static void inline mark_pages(void* page, int count, int used)
{
    register int i;
    for (i = 0; i < count; i++) mark_page(page+i*4096, used);
}

// Marks the given page as used or free.
// PALLOC, INIT_PAGING
static void inline mark_page(void* page, int used)
{
    if (used)
        *(unsigned char*)(page_bitmap+((unsigned int)page>>15)) |= 1<<(((unsigned int)page>>12)%8);
    else
        *(unsigned char*)(page_bitmap+((unsigned int)page>>15)) &= ~(unsigned char)(1<<(((unsigned int)page>>12)%8));
}

// Frees a number of contiguous pages at the given PHYSICAL ADDRESS by marking them unused.
// This only marks the physical pages as free. It does nothing with virtual addresses.
// For that, use page_unmap before marking the physical page as unused.
void page_free(void* phys_addr, int count)
{
    while (lock(&palloc_lock)) process_yield();
    mark_pages(phys_addr, count, 0);
    palloc_lock = 0;
}

// Returns the physical address of a set of free contiguous pages.
void* page_alloc(int size)
{
    // search from 16MiB to end of memory.
    void* ptr = palloc(size, (void*)0x1000000, (void*)total_memory);
    if (!ptr) return extended_alloc(size);
    return ptr;
}

// Allocates pages under the 16MB mark.
void* extended_alloc(int size)
{
    // search from 1MiB to 16MiB.
    void* ptr = palloc(size, (void*)0x100000, (void*)0x1000000);
    if (!ptr) return base_alloc(size);
    return ptr;
}

// Allocates pages under the 1MB mark.
void* base_alloc(int size)
{
    // search from 0MiB to 1MiB.
    return palloc(size, (void*)0, (void*)0x100000);
}

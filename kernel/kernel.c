#include "kernel.h"

void kmain()
{
    // System initialization
    cli(); // Clear interrupt flag
    lgdt(gdt, sizeof(gdt)); // Load global descriptor table
    // Initialize interrupt system
    init_pic(); // Programmable Interrupt Controller - remap IRQs
    init_idt(); // Interrupt Descriptor Table
    sti(); // Allow interrupts

    // Initialize memory management
    init_paging(); // Now we must init exceptions, to enable page faults
    init_heap(); // Initialize the heap

    // Initialize exceptions
    init_exceptions(); // Dependent on IDT; Start right after paging for page faults
    
    // Initialize video so we can display errors
    init_video(); // Dependent on paging, Dependent on IDT (Page faults) (Maps video memory)

    // Processes
    init_processes();

    // Interrupt related
    kbd_init(); // dependent on IDT and PIC
    init_timer(); // dependent on IDT and PIC

    // Filestystem
    fat_init();
    
    // System TSS
    init_tss();

    // System call interface (int 0x30)
    init_syscalls();

    // Load the shell
    start_shell();

    // Unmask timer IRQ
    irq_unmask(0);
}

void start_shell()
{
    // Load the shell
    if (!process_start("/system/bin/shell"))
    {
        kprintf("Error: Could not load the shell: %s\n", elf_last_error());
        freeze();
    }
}

void syscall(unsigned edi, unsigned esi, unsigned ebp, unsigned esp, unsigned ebx, unsigned edx, unsigned ecx, unsigned eax)
{
    // eax = system call number.
    // extra info normally in edx
    // returns any return value in eax
    switch (eax)
    {
        // End process
        case 0:
            process_node_unlink(processes.first);
            break;
        // Close a file.
        case 1:
            eax = 0;
            break;
        // Get environment variable count
        case 2:
            eax = 0;
            break;
        // Get the length of environment variable
        case 3:
            eax = 0;
            break;
        // Copy the environment variable number edx to ebx
        case 4:
            *((char*)ebx) = 0;
            break;
        // Get argument count
        case 5:
            eax = 0;
            break;
        // Get the length of an argument
        case 6:
            eax = 0;
            break;
        // Copy argument edx into location ebx
        case 7:
            *((char*)ebx) = 0;
            break;
        // Start a new process (ebx = *name, esi = **argv, edi = **env)
        case 8:
            eax = process_start((char*)ebx);
            break;
        // Get Process ID
        case 9:
            eax = current_process->pid;
            break;
        // Seek in a file (file edx to esi)
        case 10:
            eax = 0;
            break;
        // Read from a file (file edx, ptr edi, count ecx)
        case 11:
            eax = 0;
            break;
        // Write to a file (file edx, ptr esi, count ecx)
        case 12:
            if (edx == 0)
            {
                printlen((const char*)esi, ecx);
                eax = ecx;
            }
            else
            {
                eax = 0;
            }
            break;
        // Map address space (ptr edi, bytes ecx)
        case 13:
            eax = 0;
            break;
        // Clear the screen
        case 14:
            cls();
            break;
    }
}

void init_tss()
{
    kzeromem(&system_tss, sizeof(system_tss));
    system_tss.ss0 = 16;
    system_tss.esp0 = 0xC0000000;
    gdt[18] = 0x00000067 | (((unsigned)&system_tss)<<16);
    gdt[19] = 0x00008900 | (((unsigned)&system_tss&0x00FF0000)>>16) | ((unsigned)&system_tss&0xFF000000);
    asm volatile("ltr %0"::"r"((unsigned short)0x48));
}

void init_syscalls()
{
    register_isr(0x30, 3, (void*)int30);
}

void init_processes()
{
    processes.first = 0;
    processes.last = 0;
    process_inc = 0;
    current_process = 0;
}

void page_map(void *logical, void *physical, unsigned flags)
{
    volatile unsigned* dir     = (volatile unsigned*)0xFFFFF000; // The page directory's address
    volatile unsigned* tbl     = (volatile unsigned*)0xFFC00000+(((unsigned)logical>>12)&~0x3FF); // The page table's address
    unsigned           dir_ind = ((unsigned)logical>>22);       // The index into the page directory
    unsigned           tbl_ind = ((unsigned)logical>>12)&0x3FF; // The index into the page table
    // If a page table doesn't exist
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
            page_free((void*)(dir[dir_ind]&0xFFFFF000), 1);
            page_unmap((void*)tbl);
            dir[dir_ind] = 0;
            invlpg((void*)(unsigned)tbl);
        }
    }
    // Invalidate the page
    invlpg(logical);
}

// Returns a free virtual address range of 'size' bytes
void* kfindrange(int size)
{
    volatile void* volatile* p = (volatile void* volatile*)0xFFFC0000; // Pointer to a PTE
    void* ptr; // pointer to range
    int bytes = 0;
    while ((void*)p < (void*)0xFFFFF000)
    {
        // Addr = (p-0xFFFC0000)*4096/4+0xF0000000
        if (!bytes) ptr = (void*)((((unsigned)p-0xFFFC0000)<<12)/4+0xF0000000);
        // Is there a page table for this range?
        if (!(*((volatile unsigned*)0xFFFFF000+((unsigned)ptr>>22))&1))
        {
            // No; A whole 4MB range of memory is mappable.
            bytes += 0x400000;
            p += 1024;
        }
        else
        {
            bytes += 4096;
            if (((unsigned)*p)&1)
            {
                bytes = 0;
                ptr = 0;
            }
            p++;
        }
        if (bytes >= size) return ptr;
    }
    return 0;
}

// Gets the physical address represented by the given virtual address
void* get_physaddr(void* logical)
{
    unsigned val = ((unsigned)*((volatile void* volatile*)0xFFC00000+((unsigned int)logical>>12)));
    if (!(val&1)) return 0;
    return (void*)((val&~0xFFF)+((unsigned)logical)&0xFFF);
};

void init_paging()
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
    volatile unsigned int* x = (volatile unsigned int*)0xF0001000, *px = x; // pointer to memory map
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
    page_bitmap = (void*)(((unsigned int)image_end+3)&~3);
    page_bitmap_size = total_memory >> 15;
    // Cautiously (right now it's safe to do) map in the pages needed for the page bitmap.
    // It's safe because most likely, page_map wont't allocate a new page table. We have
    // to extend the image beyond the 1MB mark instead of using palloc right now.
    unsigned kend = (unsigned)page_bitmap+page_bitmap_size;
    unsigned kpages = ((unsigned)kernel_end-0xC0000000+0xFFF)>>12;
    unsigned tpages = (kend-0xC0000000+0xFFF)>>12;
    int i;
    for (i = kpages; i < tpages; i++)
        page_map((void*)0xC0000000+(i<<12), (void*)0x100000+(i<<12), PF_LOCKED|PF_WRITE);
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
    //   IVT-BDB-Free-Stack-SMAP-PD-PT1-PT2-bootsect-VESAinfo
    mark_pages(0,9,1);
    //   Video memory-vga rom-bios rom
    mark_pages((void*)0xA0000,96,1);
    //   kernel image-page bitmap
    ptr = (void*)0x100000;
    for (i = 0; ptr < (void*)((unsigned int)page_bitmap-(unsigned int)kernel)+page_bitmap_size+0x100000; i++, ptr += 0x1000)
        mark_page(ptr,1);
    // PAGING is now stable enough AT THIS POINT to ALLOCATE AND FREE PAGES, AND MAPPING (MAPPING MAY ALLOCATE OR FREE PAGE TABLES):
    // Unmap first megabyte
    for (ptr = (void*)0x00000000; ptr < (void*)0x00100000; ptr+=0x1000)
        page_unmap(ptr);
    // Free the system memory map
    page_unmap((void*)0xF0001000);
    page_free((void*)0x00002000, 1);
    // Map a copy of the system's Page Directory Table
    system_pdt = kfindrange(4096);
    page_map((void*)system_pdt, (void*)0x00003000, PF_LOCKED|PF_WRITE);
}

void init_heap()
{
    heap_start = heap_end = heap_brk = (void*)0xD0000000;
    first_free = last_free = 0;
}

// Adds n pages to the heap. Returns the actual number of pages added.
static int ksbrk(int n)
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

void* kbestfreeblock(int size)
{
    void* best = 0;
    void* ptr = first_free;
    while (ptr)
    {
        int s = ktagsize(ktag(ptr));
        if (size < s && ((best == 0) || s < ktagsize(ktag(best)))) best = ptr;
        ptr = knextfree(ptr);
    }
    return best;
}

void* kfirstfreeblock(int size)
{
    void* ptr = first_free;
    while (ptr)
    {
        if (size < ktagsize(ktag(ptr))) return ptr;
        ptr = knextfree(ptr);
    }
    return 0;
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
        return ptr;
    }
    else
    {
        // There are no free blocks; Try and expand the heap and make a new block.
        void* t = heap_end;
        while (t+m > heap_brk) if (!ksbrk(1)) return 0;
        heap_end = t+m;
        ptr = t+4;
        ksettag(ptr, kmaketag(m, 1, ptr == (void*)0xD0000004, 1));
        if (ptr != (void*)0xD0000004)
            ksettag(kadjprev(ptr), ktag(kadjprev(ptr))&0xFBFFFFFF); // The previous block is no longer at the end of the memory.
        return ptr;
    }
}

void kfree(void* ptr)
{
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
}

// Allocates a number of pages in the given range
static inline void* palloc(int size,void* start,void* end)
{
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
    return (void*)0;
found:
    mark_pages(startptr, size, 1);
    return startptr;
}

// Marks the given page range as used or free.
static void inline mark_pages(void* page, int count, int used)
{
    register int i;
    for (i = 0; i < count; i++) mark_page(page+i*4096, used);
}

// Marks the given page as used or free.
static void inline mark_page(void* page, int used)
{
    if (used)
        *(unsigned char*)(page_bitmap+((unsigned int)page>>15)) |= 1<<(((unsigned int)page>>12)%8);
    else
        *(unsigned char*)(page_bitmap+((unsigned int)page>>15)) &= ~(unsigned char)(1<<(((unsigned int)page>>12)%8));
}

// Frees a number of contiguous pages at the given physical address
void page_free(void* addr, int count)
{
    mark_pages(addr, count, 0);
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

// Initialize the Interrupt Descriptor Table
void init_idt()
{
    int i;
    // zero all entries
    for (i = 0; i < 512; i++) idt[i] = 0;
    // load idt
    lidt((void*)idt,sizeof(idt));
}

// Initialize the Programmable Interrupt Controller
void init_pic()
{
    // Re-program Programmable Interrupt Controller,
    // setting Interrupt Request vectors to 0x20-0x2F.
    outb(0x20, 0x11); // start the initialization sequence
    io_wait();
    outb(0xa0, 0x11);
    io_wait();
    outb(0x21, 0x20); // define PIC vectors
    io_wait();
    outb(0xa1, 0x28);
    io_wait();
    outb(0x21, 4);    // continue initialization sequence
    io_wait();
    outb(0xa1, 2);
    io_wait();

    outb(0x21, 1);    // ICW4_8086
    io_wait();
    outb(0xa1, 1);
    io_wait();
    
    outb(0x21, 0xFF); // mask irqs
    outb(0xa1, 0xFF);
}

// Add a normal ISR to the IDT
void register_isr(int num, int dpl, void* offset)
{
    volatile struct IDTDescr* gate;
    gate=&(((struct IDTDescr*)idt)[num]);
    gate->offset_1  = (unsigned short)(unsigned int)offset;
    gate->selector  = 8;
    gate->zero      = 0;
    gate->type_attr = 0x8E|(dpl<<5);
    gate->offset_2  = (unsigned short)(((unsigned int)offset)>>16);
}

// Add a trap gate to the IDT
void register_trap(int num, void* offset)
{
    volatile struct IDTDescr* gate;
    gate=&(((struct IDTDescr*)idt)[num]);
    gate->offset_1  = (unsigned short)(unsigned int)offset;
    gate->selector  = 8;
    gate->zero      = 0;
    gate->type_attr = 0x8F;
    gate->offset_2  = (unsigned short)(((unsigned int)offset)>>16);
}

// Add a task gate to the IDT
void register_task(int num, unsigned short selector)
{
    volatile struct IDTDescr* gate;
    gate=&(((struct IDTDescr*)idt)[num]);
    gate->offset_1  = 0;
    gate->selector  = selector;
    gate->zero      = 0;
    gate->type_attr = 0x85;
    gate->offset_2  = 0;
}

// Register exceptions into the IDT
void init_exceptions()
{
    // Division error
    register_isr(0x0,0,(void*)int0);
    // Double fault
    register_isr(0x8,0,(void*)int8);
    // General protection fault
    register_isr(0xd,0,(void*)intd);
    // Page fault
    register_isr(0xe,0,(void*)inte);
}

// Keyboard initialization
void kbd_init()
{
    // fill descriptor 0x21 (irq 1) for keyboard handler
    register_isr(0x21,0,(void*)irq1);
    // unmask IRQ
    irq_unmask(1);
    // Keyboard variables
    kbd_escaped = 0;
    kbd_shift = 0;
}

// Initialize the system timer - the Programmable Interval Timer
void init_timer()
{
    outb(0x43, 0x34); // Channel 0: rate generator
    // 17898 = roughly every 15ms
    outb(0x40, 17898&0xFF); // reload value lo byte channel 0
    outb(0x40, 17898>>8); // reload value hi byte channel 0
    // fill descriptor 0x20 (irq 0) for timer handler
    register_isr(0x20,0,(void*)irq0);
    // unmask IRQ later
}

// Called when a divide exception occurs
void divide_error(unsigned edi, unsigned esi, unsigned ebp, unsigned esp, unsigned ebx, unsigned edx, unsigned ecx, unsigned eax, unsigned eip, unsigned cs, unsigned eflags)
{
    dump_stack("A division error occured.", edi, esi, ebp, esp+12, ebx, edx, ecx, eax, eip, cs);
}

// Double fault handler
void double_fault(unsigned edi, unsigned esi, unsigned ebp, unsigned esp, unsigned ebx, unsigned edx, unsigned ecx, unsigned eax, unsigned errorcode, unsigned eip, unsigned cs, unsigned eflags)
{
    dump_stack("A double fault occured.", edi, esi, ebp, esp+16, ebx, edx, ecx, eax, eip, cs);
}

// General Protection Fault handler
void gpfault(unsigned edi, unsigned esi, unsigned ebp, unsigned esp, unsigned ebx, unsigned edx, unsigned ecx, unsigned eax, unsigned errorcode, unsigned eip, unsigned cs, unsigned eflags)
{
    char t[100];
    ksprintf(t, "A general protection fault occurred with error code %l.", errorcode);
    dump_stack(t, edi, esi, ebp, esp+16, ebx, edx, ecx, eax, eip, cs);
}

// Page fault handler, called after pusha in 'inte' handler
void pgfault(unsigned edi, unsigned esi, unsigned ebp, unsigned esp, unsigned ebx, unsigned edx, unsigned ecx, unsigned eax, unsigned errorcode, unsigned eip, unsigned cs, unsigned eflags)
{
    volatile unsigned int cr2;
    asm volatile ("mov %%cr2,%0":"=r"(cr2));
    if ((errorcode & 1) == 0 && cr2 >= 0xc0000000)
    {
        unsigned t = cr2 >> 22;
        if (!(process_pdt[t]&1))
        {
            if (system_pdt[t]&1)
            {
                process_pdt[t] = system_pdt[t];
                invlpg((void*)cr2);
                goto nodump;
            }
        }
    }
    char t[100];
    ksprintf(t, "A page fault occurred at address %l, with error code %l.", cr2, errorcode);
    dump_stack(t, edi, esi, ebp, esp+16, ebx, edx, ecx, eax, eip, cs);
    nodump:
    return;
}

static void bsod(const char* msg)
{
    int i;
    cli();
    show_cursor(0);
    cls();
    print(msg);
    // Make the text display white on blue
    for (i = 1; i < 4000; i += 2) videomem[i] = 0x1F;
    do hlt(); while (1);
}

// Dump the stack
static void dump_stack(const char* msg, unsigned edi, unsigned esi, unsigned ebp, unsigned esp, unsigned ebx, unsigned edx, unsigned ecx, unsigned eax, unsigned eip, unsigned cs)
{
    cli();
    show_cursor(0);
    cls();
    print(msg);
    print("\nDumping stack and halting CPU.\n");
    int i;
    volatile unsigned /*esp, ebp, esi, edi, eax, ebx, ecx, edx, cs,*/ ds, es, fs, gs, ss;
  /*asm volatile ("mov %%esp,%0":"=g"(esp));
    asm volatile ("mov %%ebp,%0":"=g"(ebp));
    asm volatile ("mov %%esi,%0":"=g"(esi));
    asm volatile ("mov %%edi,%0":"=g"(edi));
    asm volatile ("mov %%eax,%0":"=g"(eax));
    asm volatile ("mov %%ebx,%0":"=g"(ebx));
    asm volatile ("mov %%ecx,%0":"=g"(ecx));
    asm volatile ("mov %%edx,%0":"=g"(edx));
    asm volatile ("mov %%cs, %0":"=r"(cs));*/
    asm volatile ("mov %%ds, %0":"=r"(ds));
    asm volatile ("mov %%es, %0":"=r"(es));
    asm volatile ("mov %%fs, %0":"=r"(fs));
    asm volatile ("mov %%gs, %0":"=r"(gs));
    asm volatile ("mov %%ss, %0":"=r"(ss));
    kprintf("  ESP: %l  EBP: %l  ESI: %l  EDI: %l\n", esp, ebp, esi, edi);
    kprintf("  EAX: %l  EBX: %l  ECX: %l  EDX: %l\n", eax, ebx, ecx, edx);
    kprintf("  EIP: %l", eip);
    print("  CS: "); printhexw(cs);
    print("  DS: "); printhexw(ds);
    print("  ES: "); printhexw(es);
    print("  FS: "); printhexw(fs);
    print("  GS: "); printhexw(gs);
    print("  SS: "); printhexw(ss);
    // Print the stack
    for (i = 0; i < 20; i++, esp+=4)
    {
        int val;
        asm volatile("mov %%ss:(%1),%0":"=g"(val):"p"(esp));
        kprintf("\n  SS:%l %l", esp, val);
    }
    // Make the text display white on blue
    for (i = 1; i < 4000; i += 2) videomem[i] = 0x1F;
    do hlt(); while (1);
}

// called by wrapper irq0()
int handle_timer()
{
    return process_rotate();
}

Process* process_create(const char* name)
{
    Process *p = kmalloc(sizeof(Process));
    kstrcpy(p->name, name);
    p->pid = process_inc++;
    return p;
}

void process_enqueue(Process* p)
{
    ProcessNode *n = kmalloc(sizeof(ProcessNode));
    n->process = p;
    process_node_link(n);
}

int process_rotate()
{
    if (!processes.first) return 0;
    if (current_process) // If we are already in a process
    {
        if (current_process->timeslice--)
        {
            return 0;
        }
        ProcessNode* n = processes.first;
        process_node_unlink(n); // Take the node off the head of the queue
        process_node_link(n);   // Put it on the rear of the queue
    }
    current_process = processes.first->process;
    current_process->timeslice = 3-current_process->priority;
    return 1;
}

void process_node_unlink(ProcessNode* n)
{
    if (!n) return;
    if (n == processes.first) processes.first = n->next;
    if (n == processes.last) processes.last = n->prev;
    if (n->prev) n->prev->next = n->next;
    if (n->next) n->next->prev = n->prev;
}

void process_node_link(ProcessNode* n)
{
    if (!n) return;
    if (processes.last) processes.last->next = n;
    n->prev = processes.last;
    n->next = 0;
    processes.last = n;
    if (!processes.first) processes.first = n;
}

// called by wrapper irq1()
void handle_keyboard()
{
    char scancode = inb(0x60);
    int escaped = kbd_escaped;
    kbd_escaped = 0;
    if (escaped)
    {
    }
    else switch ((unsigned char)scancode)
    {
    case 0x0E:
        // print("\b\x20\b");
        break;
    case 0x1C:
        // endl();
        break;
    case 0x2A:
    case 0x36:
        kbd_shift = 1;
        break;
    case 0xAA:
    case 0xB6:
        kbd_shift = 0;
        break;
    case 0xE0:
        kbd_escaped = 1;
        break;
    default:
        if (!(scancode&0x80))
        {
            char c = kbd_shift? kbd_uppercase[scancode] : kbd_lowercase[scancode];
            // print_char(c);
        }
        break;
    }
    eoi(1);
}

DateTime get_time()
{
    DateTime dt;
    unsigned char c;
    
    outb(0x70, 10);
    while (inb(0x71) & 0x80) outb(0x70, 10);
    
    outb(0x70, 8);
    c = inb(0x71);
    dt.Month = ((c&0xF0)>>4)*10+(c&0xF);
    
    outb(0x70, 7);
    c = inb(0x71);
    dt.Day = ((c&0xF0)>>4)*10+(c&0xF);
    
    outb(0x70, 9);
    c = inb(0x71);
    dt.Year = 2000 + ((c&0xF0)>>4)*10+(c&0xF);
    
    outb(0x70, 4);
    c = inb(0x71);
    dt.Hour = ((((c&0x70)>>4)*10+(c&0xF)) + (c & 0x80 ? 12 : 0)) % 24; // BCD from 24- or 12-hour format to (0-23)
    
    outb(0x70, 2);
    c = inb(0x71);
    dt.Minute = ((c&0xF0)>>4)*10+(c&0xF);
    
    outb(0x70, 0);
    c = inb(0x71);
    dt.Second = ((c&0xF0)>>4)*10+(c&0xF);
    
    return dt;
}

const char* kstrcpy(char* dest, const char* src)
{
    const char* destorig = dest;
    do *dest++ = *src; while (*src++);
    return destorig;
}
int kstrlen(char* str)
{
    int c = 0;
    while (*str++) c++;
    return c;
}
const char* ksprinthexb(char* str, char c)
{
    str[0] = '0'+((c&0xF0)>>4);
    str[1] = '0'+(c&0xF);
    str[2] = 0;
    if (str[0] > '9') str[0] += 7;
    if (str[1] > '9') str[1] += 7;
    return str;
}

const char* ksprinthexw(char* str, short w)
{
    ksprinthexb(str, w>>8);
    ksprinthexb(str+2, w);
    return str;
}

const char* ksprinthexd(char* str, int d)
{
    ksprinthexw(str, d>>16);
    ksprinthexw(str+4, d);
    return str;
}

const char* ksprintdec(char* str, int x)
{
    if (x == (int)0x80000000)
    {
        kstrcpy(str, "-2147483648");
        return;
    }
    int temp = x < 0 ? -x : x;
    int div;
    int mod;
    char str2[11];
    char *strptr = str;
    char *strptr2 = str2;
    if (x < 0)
    {
        str[0] = '-';
        strptr++;
    }
    do
    {
        div = temp/10;
        mod = temp%10;
        temp = div;
        *strptr = '0'+mod;
        strptr++;
    } while (temp != 0);
    *strptr = 0;
    do
    {
        strptr--;
        *strptr2 = *strptr;
        strptr2++;
    } while (strptr != str);
    *strptr2 = 0;
    return str;
}

const char* kstrcat(char* dest, const char* src)
{
    return kstrcpy(dest+kstrlen(dest), src);
}

int kstrcmp(const char* a, const char* b)
{
    while ((*a && *b) && (*a == *b)) a++, b++;
    return *a - *b;
}

const char* ksprintf(char* dest, const char* format, ...)
{
    const char* destorig = dest;
    int arg = 0;
    while (*format)
    {
        volatile unsigned val;
        // Skip 4 items on the stack at %ebp: prev %epb, %eip, $dest, and $format.
        asm volatile ("movl (%%ebp,%1), %0":"=r"(val):"r"((arg+4)*4));
        if (*format == '%')
        {
            switch (*++format)
            {
                case 'd':
                    arg++;
                    ksprintdec(dest, val);
                    while (*dest) dest++;
                    break;
                case 's':
                    arg++;
                    kstrcpy(dest, (const char*)val);
                    while (*dest) dest++;
                    break;
                case 'c':
                    arg++;
                    *dest++ = (char)val;
                    break;
                case 'b':
                    arg++;
                    kstrcpy(dest, "0x");
                    dest += 2;
                    ksprinthexb(dest, val);
                    dest += 2;
                    break;
                case 'w':
                    arg++;
                    kstrcpy(dest, "0x");
                    dest += 2;
                    ksprinthexw(dest, val);
                    dest += 4;
                    break;
                case 'l':
                    arg++;
                    kstrcpy(dest, "0x");
                    dest += 2;
                    ksprinthexd(dest, val);
                    dest += 8;
                    break;
                case '%':
                    *dest++ = '%';
                    break;
                default:
                    *dest++ = '%';
                    *dest++ = *format;
                    break;
            }
        }
        else
        {
            *dest++ = *format;
        }
        format++;
    }
    *dest = 0;
    return destorig;
}

/*void print_datetime()
{
    DateTime dt = get_time();
again:
    switch (dt.Month)
    {
        case 1:
             print("January");
             break;
        case 2:
             print("February");
             break;
        case 3:
             print("March");
             break;
        case 4:
             print("April");
             break;
        case 5:
             print("May");
             break;
        case 6:
             print("June");
             break;
        case 7:
             print("July");
             break;
        case 8:
             print("August");
             break;
        case 9:
             print("September");
             break;
        case 10:
             print("October");
             break;
        case 11:
             print("November");
             break;
        case 12:
             print("December");
             break;
    }
    print_char('\x20');
    printdec(dt.Day);
    switch (dt.Day % 10)
    {
           case 1:
                print("st");
                break;
           case 2:
                print("nd");
                break;
           case 3:
                print("rd");
                break;
           default:
                print("th");
                break;
    }
    print(", ");
    printdec(dt.Year);
    print("  ");
    printdec((dt.Hour % 12) == 0 ? 12 : dt.Hour % 12);
    print(":");
    if (dt.Minute < 10) print("0");
    printdec(dt.Minute);
    print(" ");
    if (dt.Second < 10) print("0");
    printdec(dt.Second);
    print(" ");
    print(dt.Hour < 12 ? "AM" : "PM");
    endl();
}

int read_bitmap(Bitmap *b, char *filename)
{
       FileStream fs;
       if (!file_open(filename, &fs)) return 0;
       BITMAPFILEHEADER bf;
       file_read(&fs, &bf, sizeof(BITMAPFILEHEADER));
       if (bf.bfType != 0x4D42)
       {
           file_close(&fs);
           return 0;
       }
       BITMAPCOREHEADER bc;
       file_read(&fs, &bc, sizeof(BITMAPCOREHEADER));
       void* buffer = kmalloc(bc.bcWidth*bc.bcHeight*(bc.bcBitCount/8));
       if (!buffer)
       {
           file_close(&fs);
           return 0;
       }
       file_seek(&fs, bf.bfOffBits);
       file_read(&fs, buffer, bc.bcWidth*bc.bcHeight*(bc.bcBitCount/8));
       file_close(&fs);
       b->width = bc.bcWidth;
       b->height = bc.bcHeight;
       b->bits = buffer;
       b->bpp = bc.bcBitCount;
       return 1;
}*/

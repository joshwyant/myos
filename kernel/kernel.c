#include "kernel.h"

static void demo();

void kmain()
{
    // perform all initialization first
    kernel_init();
    // run demo
    demo();
}

static void demo()
{
    // Clear the screen and display greeting (video driver).
    cls();
    print("Greetings from MyOS.\r\n");
    print_datetime();

    // Report amount of RAM.
    printdec(total_memory>>20); print("MiB of RAM\r\n\n");

    // Time to play with the file system.
    // Print the size of file 'fname'
    char* fname = "/user/Josh/docs/hello.txt";
    print(fname);
    print("\r\nFile size: ");
    int size = file_size(fname);
    printdec((size+1023)>>10); print("KiB\r\n\n");

    // Read the file.
    FileStream fs;
    if (file_open(fname, &fs))
    {
        while (file_peekc(&fs) != '\xFF')
            print_char(file_getc(&fs));
        file_close(&fs);
    }
    else
    {
        print("Error: Could not find "); print(fname); print ("!\r\n");
    }

    // Load the shell
    if (!process_start("/system/bin/shell"))
    {
        print("ERROR: Could not load the shell: ");
        print(elf_last_error()); endl();
    }
    else
    {
        print("\r\nShell was successfully loaded.\r\n");
    }
    print("Type stuff: ____________________\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b");

#ifdef VESA
    void* vesa = kfindrange(4096);
    void* vbemode = vesa+512;
    page_map(vesa, (void*)0x00008000);
    int xres, yres, bpp;
    xres = (int)*(short*)(vbemode+18);
    yres = (int)*(short*)(vbemode+20);
    bpp = ((int)*(char*)(vbemode+25))/8;
    unsigned char *ptr = *(unsigned char**)(vbemode+40);
    page_unmap(vesa);
    page_free((void*)0x00008000,1);
    unsigned char *p = kfindrange(xres*yres*3);
    int i;
    for (i = 0; i < xres*yres*bpp+4095; i += 4096)
        page_map(p+i,ptr+i);
    Bitmap b;
    print("\r\nVideo memory: 0x"); printhexd((unsigned)ptr); print(" mapped to 0x"); printhexd((unsigned)p); endl();
    if (read_bitmap(&b, "/system/bin/splash"))
    {
        print("Bitmap read successfully.\r\n");
        print("Width:  "); printdec(b.width); endl();
        print("Height: "); printdec(b.height); endl();
        print("BPP:    "); printdec(b.bpp); endl();
        print("Buffer: 0x"); printhexd((unsigned)b.bits); endl();
        print("Sample: "); printhexb(((char*)b.bits)[0]); print(" "); printhexb(((char*)b.bits)[1]); print(" "); printhexb(((char*)b.bits)[2]); endl();
        while (1)
        {
            int screenstride = xres*bpp;
            int bmpstride = b.width*(b.bpp/8);
            int j;
            float t;
            for (t = 0.0f; t < 1.0f; t += 0.01f)
            for (i = 0; i < yres && i < b.height; i++)
            for (j = 0; j < xres && j < b.width; j++)
            {
                int q = b.width*b.height*(b.bpp/8)-(bmpstride*(i+1))+j*(b.bpp/8);
                int r = i*screenstride+j*bpp;
                p[r] = (unsigned char)(((float)((unsigned char*)b.bits)[q])*t);
                p[r+1] = (unsigned char)(((float)((unsigned char*)b.bits)[q+1])*t);
                p[r+2] = (unsigned char)(((float)((unsigned char*)b.bits)[q+2])*t);
            }
            for (t = 1.0f; t > 0.0f; t -= 0.01f)
            for (i = 0; i < yres && i < b.height; i++)
            for (j = 0; j < xres && j < b.width; j++)
            {
                int q = b.width*b.height*(b.bpp/8)-(bmpstride*(i+1))+j*(b.bpp/8);
                int r = i*screenstride+j*bpp;
                p[r] = (unsigned char)(((float)((unsigned char*)b.bits)[q])*t);
                p[r+1] = (unsigned char)(((float)((unsigned char*)b.bits)[q+1])*t);
                p[r+2] = (unsigned char)(((float)((unsigned char*)b.bits)[q+2])*t);
            }
        }
    }
    else
    {
        print("Bitmap could not be read.\r\n");
        for (i = 0; i < XRES*YRES*3; i++) p[i] = 128;
    }
#endif
}


void kernel_init()
{
    lgdt(gdt, sizeof(gdt));
    // Initialize interrupt system
    init_pic(); // Programmable Interrupt Controller
    init_idt(); // Interrupt Descriptor Table
    sti(); // Allow interrupts

    // Initialize memory management
    init_paging(); // Now we must init exceptions, to enable page faults
    init_heap();

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

    // start page swapping service
    swap_start();
}

void init_processes()
{
    processes.first = 0;
    processes.last = 0;
    process_count = 0;
    current_process = 0;
    current_process_node = 0;
}

void page_map(void *logical, void *physical)
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
        if (!ptr) dump_stack("Insufficient memory.");
        dir[dir_ind] = (unsigned)ptr+3; // add to directory
        // wipe table (after we mapped it! stumped me for about 5 minutes...)
        int i;
        for (i = 0; i < 1024; i++) tbl[i] = 0;
        invlpg((void*)(unsigned)tbl);
    }
    tbl[tbl_ind] = ((unsigned)physical&~0xFFF)+3;
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
        page_map((void*)0xC0000000+(i<<12), (void*)0x100000+(i<<12));
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
        page_map(heap_brk, pg);
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

// TODO: To be implemented...
void swap_start()
{
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
void register_isr(int num, void* offset)
{
    volatile struct IDTDescr* gate;
    gate=&(((struct IDTDescr*)idt)[num]);
    gate->offset_1  = (unsigned short)(unsigned int)offset;
    gate->selector  = 8;
    gate->zero      = 0;
    gate->type_attr = 0x8E;
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
    register_trap(0x0,(void*)int0);
    // Double fault
    register_trap(0x8,(void*)int8);
    // General protection fault
    register_trap(0xd,(void*)intd);
    // Page fault
    register_trap(0xe,(void*)inte);
}

// Keyboard initialization
void kbd_init()
{
    // fill descriptor 0x21 (irq 1) for keyboard handler
    register_isr(0x21,(void*)irq1);
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
    outb(0x40, 3000&0xFF); // reload value lo byte channel 0
    outb(0x40, 3000>>8); // reload value hi byte channel 0
    // fill descriptor 0x20 (irq 0) for timer handler
    register_isr(0x20,(void*)irq0);
    // unmask IRQ
    irq_unmask(0);
}

// Called when a divide exception occurs
void divide_error()
{
    dump_stack("A division error occured.");
}

// Double fault handler
void double_fault()
{
    dump_stack("A double fault occured.");
}

// General Protection Fault handler
void gpfault()
{
    dump_stack("A general protection fault occured.");
}

// Page fault handler
void pgfault(unsigned int errcode)
{
    volatile unsigned int cr2;
    asm volatile ("mov %%cr2,%0":"=r"(cr2));
    dump_stack("A page fault occurred.");
}

// Dump the stack
static void dump_stack(const char* msg)
{

    cli();
    cls();
    print(msg);
    print("\r\nDumping stack and halting CPU.\r\n");
    int i;
    volatile int esp, ebp, esi, edi, eax, ebx, ecx, edx, cs, ds, es, fs, gs, ss;
    asm volatile ("mov %%esp,%0":"=g"(esp));
    asm volatile ("mov %%ebp,%0":"=g"(ebp));
    asm volatile ("mov %%esi,%0":"=g"(esi));
    asm volatile ("mov %%edi,%0":"=g"(edi));
    asm volatile ("mov %%eax,%0":"=g"(eax));
    asm volatile ("mov %%ebx,%0":"=g"(ebx));
    asm volatile ("mov %%ecx,%0":"=g"(ecx));
    asm volatile ("mov %%edx,%0":"=g"(edx));
    asm volatile ("mov %%cs, %0":"=r"(cs));
    asm volatile ("mov %%ds, %0":"=r"(ds));
    asm volatile ("mov %%es, %0":"=r"(es));
    asm volatile ("mov %%fs, %0":"=r"(fs));
    asm volatile ("mov %%gs, %0":"=r"(gs));
    asm volatile ("mov %%ss, %0":"=r"(ss));
    print("  ESP: 0x"); printhexd(esp);
    print("  EBP: 0x"); printhexd(ebp);
    print("  ESI: 0x"); printhexd(esi);
    print("  EDI: 0x"); printhexd(edi); print("\r\n");
    print("  EAX: 0x"); printhexd(eax);
    print("  EBX: 0x"); printhexd(ebx);
    print("  ECX: 0x"); printhexd(ecx);
    print("  EDX: 0x"); printhexd(edx); print("\r\n");
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
        print("\r\n  SS:"); printhexd((unsigned int)esp); print(" 0x"); printhexd(val);
    }
    //show_cursor(0);
    // Make the text display white on blue
    for (i = 1; i < 4000; i += 2) videomem[i] = 0x1F;
a:  hlt();
    goto a;
}

// called by wrapper irq0()
void handle_timer()
{
    
}

Process* process_create(char* name)
{
    Process *p = kmalloc(sizeof(Process));
    return p;
    // TODO:
    // When process is killed, free all memory it uses,
    // and all structures associated with it
}

void process_node_delete(ProcessNode* n)
{
    if (!n) return;
    if (n == processes.first) processes.first = n->next;
    if (n == processes.last) processes.last = n->prev;
    if (n->prev) n->prev->next = n->next;
    if (n->next) n->next->prev = n->prev;
    kfree(n);
}

void process_node_after(Process* p, ProcessNode* prev)
{
    ProcessNode *n = kmalloc(sizeof(ProcessNode));
    n->process = p;
    n->next = prev ? prev->next : 0;
    n->prev = prev;
    if (prev->next) prev->next->prev = n;
    if (prev) prev->next = n;
    if (n->next == 0) processes.last = n;
    if (n->prev == 0) processes.first = n;
}

void process_node_before(Process* p, ProcessNode* next)
{
    ProcessNode *n = kmalloc(sizeof(ProcessNode));
    n->process = p;
    n->next = next;
    n->prev = next ? next->prev : 0;
    if (next->prev) next->prev->next = n;
    if (next) next->prev = n;
    if (n->next == 0) processes.last = n;
    if (n->prev == 0) processes.first = n;
}

void process_enqueue(Process* p)
{
    process_node_before(p, processes.first);
}

Process* process_dequeue()
{
    if (!processes.last) return 0;
    Process* p = processes.last->process;
    process_node_delete(processes.last);
    return p;
}

Process* process_head()
{
    if (!processes.last) return 0;
    return processes.last->process;
}

Process* process_tail()
{
    if (!processes.first) return 0;
    return processes.first->process;
}

void process_switch()
{
    // TODO: kill processes
    if (current_process && current_process->timeslice--) return; 
    Process* p = process_head();
    if (!p) return;
    process_dequeue();
    process_enqueue(p);
    current_process = p;
    current_process->timeslice = 1;
    asm volatile ("mov %0,%%cr3"::"a"(current_process->cr3));
    // isr pops registers and returns to last instruction
}

void killme()
{
    current_process->killme = 1;
    for (;;);
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
        print("\b\x20\b");
        break;
    case 0x1C:
        endl();
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
            char c = kbd_shift?kbd_uppercase[scancode]:kbd_lowercase[scancode];
            print_char(c);
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

void print_datetime()
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
}

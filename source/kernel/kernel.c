#include "kernel.h"

static void demo1();
static void demo2();

void kmain()
{
    // perform all initialization first
    kernel_init();
    // run demo
    demo2();
}

static void demo1()
{
    // Clear the screen and display greeting (video driver).
    cls();
    print("Greetings from MyOS.\r\n");

    // Report amount of RAM.
    printdec(total_memory>>20); print("MiB of RAM\r\n\n");

    // Time to play with the file system.
    // Print the size of file '/user/Josh/docs/hello.txt.'
    print("/user/Josh/docs/hello.txt\r\nFile size: ");
    int size = file_size("/user/Josh/docs/hello.txt");
    printdec((size+1023)>>10); print("KiB\r\n\n");

    // Read the file.
    FileStream fs;
    file_open("/user/Josh/docs/hello.txt", &fs);
    while (file_peekc(&fs) != '\xFF')
        print_char(file_getc(&fs));
    file_close(&fs);
}

static void demo2()
{
    // Clear the screen.
    cls();
    // Load the shell
    if (!process_start("/system/bin/shell.elf"))
    {
        print("ERROR: Could not load the shell: ");
        print(elf_last_error()); endl();
        cli();
        hang();
    }
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
    invlpg(logical);
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
    //   entry,entry,entries...,times 6 db 0
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
    //   IVT-BDB-Free-Stack-SMAP-PD-PT1-PT2-bootsect
    mark_pages(0,8,1);
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

static inline struct FreeNode* kbestfreeblock(int size)
{
    struct FreeNode* free = first_free;
    struct FreeNode* best = 0;
    while (free)
    {
        if ((free->size >= size) && ((best == 0) || (best->size > free->size))) best = free;
        free = free->next;
    }
    return best;
}

static inline struct FreeNode* kfirstfreeblock(int size)
{
    struct FreeNode* free = first_free;
    while (free)
    {
        if (free->size >= size) return free;
        free = free->next;
    }
    return 0;
}

static inline void unlink_freenode(struct FreeNode* free)
{
    // unlink the free node.
    if (free->prev)
        free->prev->next = free->next;
    else
        first_free = free->next;
    if (free->next)
        free->next->prev = free->prev;
    else
        last_free = free->prev;
}

static inline void free_freenode(struct FreeNode* free)
{
    unlink_freenode(free);
    kfree(free);
}

// Returns pointer to header structure of an allocated memory address
static inline struct MemHeader* mhead(void* ptr) { return (struct MemHeader*)((unsigned)ptr-sizeof(struct MemHeader)); }

void* kmalloc(int size)
{
    struct FreeNode* free = kbestfreeblock(size); // faster: kfirstfreeblock()
    void* ptr;
    int s;
    if (free)
    {
        ptr = free->ptr;
        // use the whole block if the remaining space
        // is too small for a header and at least an extra byte.
        if (free->size <= size+sizeof(struct MemHeader))
        {
            mhead(ptr)->size = free->size;
            free_freenode(free);
        }
        else
        {
            // Use part of the free block; update the information
            free->size -= size+sizeof(struct MemHeader);
            free->ptr += size+sizeof(struct MemHeader);
            mhead(ptr)->size = size;
        }
        return ptr;
    }
    else
    {
        // Use the end of the heap
        s = sizeof(struct MemHeader)+size;
        void* t = heap_end;
        while (t+s > heap_brk) if (!ksbrk(1)) return 0;
        heap_end = t+s;
        ((struct MemHeader*)t)->size = size;
        return t+sizeof(struct MemHeader);
    }
}

void kfree(void* ptr)
{
    // find the largest area that we can free.
    // If at the end of heap, just set heap_end.
    // Otherwise, mark that area as free.
    if (!ptr) return;

    struct FreeNode *free, *free2, *freeprev = 0, *freenext = 0, tfree;
    // Find the actual free space determined by any adjacent free blocks
    void* newptr = ptr;
    unsigned newsize = mhead(ptr)->size;
    free2 = first_free;
    while (free2)
    {
        if ((free2->ptr + free2->size + sizeof(struct MemHeader)) == ptr)
        {
            freeprev = free2;
            newptr = free2->ptr;
            newsize += free2->size + sizeof(struct MemHeader);
        }
        else if ((ptr + mhead(ptr)->size + sizeof(struct MemHeader)) == free2->ptr)
        {
            freenext = free2;
            newsize += free2->size + sizeof(struct MemHeader);
        }
        free2 = free2->next;
    }
    // Unlink any freenodes to adjacent free blocks
    if (freeprev) unlink_freenode(freeprev);
    if (freenext) unlink_freenode(freenext);
    // Is this block at the end of the heap?
    if ((newptr + newsize) == heap_end)
    {
        // All we have to do is move the end of the heap!
        heap_end = mhead(newptr);
        // cleanup duty
        if (freeprev) kfree(freeprev);
        if (freenext) kfree(freenext);
        return;
    }
    // our new freenode
    tfree.ptr = newptr;
    tfree.size = newsize;
    tfree.prev = 0;
    tfree.next = first_free;
    // try and reuse one
    if (freeprev)
    {
        free = freeprev;
        if (freenext) kfree(freenext); // don't need it any more
    }
    else if (freenext)
    {
        free = freenext;
        // 'freeprev' takes precedence over 'freenext'
    }
    else
    {
        free = kmalloc(sizeof(struct FreeNode)); // Easy; If we're out of memory, nothing to do
        if (!free) return; // We've reached a dead end! // XXX Try to (carefully) use the block we're freeing
    }
    // Link it in
    
    *free = tfree;
    if (first_free) first_free->prev = free;
    first_free = free;
    /* end of kfree() */

    /* The simple method below does not combine any free blocks */
    /*
    struct FreeNode* free;
    free = kmalloc(sizeof(struct FreeNode)); // assumes there is enough memory
    free->ptr = ptr;
    free->size = mhead(ptr)->size;
    // link it in
    free->prev = 0;
    free->next = first_free;
    if (first_free) first_free->prev = free;
    first_free = free;
    */
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

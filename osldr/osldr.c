#include "osldr.h"

void loader_main()
{
    // System initialization
    cli(); // Clear interrupt flag
    lgdt(gdt, sizeof(gdt)); // Load global descriptor table
    // Initialize interrupt system
    init_pic(); // Programmable Interrupt Controller - remap IRQs
    init_idt(); // Interrupt Descriptor Table
    sti(); // Allow interrupts

    // Initialize video so we can display error messages, menus, etc.
    init_video();

    init_mem();

    // Filestystem: Initialize now so we can write error logs
    fat_init();

    // Initialize exceptions
    init_exceptions(); // Dependent on IDT

    // Interrupt related
    kbd_init(); // dependent on IDT and PIC
    
    loader_info li = { (void*)0x00001000, 0, 0, 0 };
    if (!load_kernel(&li))
    {
        kprintf("Error: could not load the kernel: %s", elf_last_error());
        cli();
        freeze();
    }
}

// crude and fast way to allocate a page (easy, because it is not freed)
void* findpage()
{
    volatile unsigned int* x = (volatile unsigned int*)0x00001000, *px = x; // pointer to memory map
    while (1)
    {
        if ((px[0]|px[1]|px[2]|px[3]|px[4]|px[5]) == 0) break;
        if (px[5]&1) // bit 0 not set means ignore
        {
            switch (px[4])
            {
            case 1: // Free memory
                if ((px[0] <= (unsigned)next_page) && (px[2] >= (unsigned)next_page+0x1000))
                {
                    void* ptr = next_page;
                    next_page += 0x1000;
                    return ptr;
                }
                break;
            }
        }
        px += 6;
    }
    px = x;
    while (1)
    {
        if ((px[0]|px[1]|px[2]|px[3]|px[4]|px[5]) == 0) break;
        if (px[5]&1) // bit 0 not set means ignore
        {
            switch (px[4])
            {
            case 1: // Free memory
                // assumes that the SMAP is in low-to-high-address order
                if (px[0] > (unsigned)next_page) next_page = (void*)px[0];
                if ((px[0] <= (unsigned)next_page) && (px[2] >= (unsigned)next_page+0x1000))
                {
                    void* ptr = next_page;
                    next_page += 0x1000;
                    return ptr;
                }
                break;
            }
        }
        px += 6;
    }
    return 0;
}

void* kmalloc(int size)
{
    // VERY simple. We have to be very careful and conservative when using the heap.
    void* ptr = heap_end;
    heap_end += size;
    return ptr;
}

void kfree(void* ptr)
{
  // How about we just don't free memory in the loader.
  // Why should we?
  // We won't use alot.
  // The memory will get reused later.
  // We should keep the loader as simple as possible.
}

void page_map(void *logical, void *physical, unsigned flags)
{
    unsigned* 		dir     = (unsigned*)pgdir; // The page directory's address
    unsigned* 		tbl     = (unsigned*)(dir[(unsigned)logical>>22]&~0xFFF); // The page table's address
    unsigned           dir_ind = ((unsigned)logical>>22);       // The index into the page directory
    unsigned           tbl_ind = ((unsigned)logical>>12)&0x3FF; // The index into the page table
    // If a page table doesn't exist
    if (!(dir[dir_ind]&1))
    {
        // Page table
        tbl = findpage();
        if (!tbl) bsod("Insufficient memory.");
        dir[dir_ind] = (unsigned)tbl|(PF_PRESENT|PF_WRITE); // add to directory
        // wipe table
        kzeromem(tbl, 4096);
    }
    tbl[tbl_ind] = ((unsigned)physical&~0xFFF)+(flags|PF_PRESENT);
}


// Initialize our crude memory management.
void init_mem()
{
    next_page = (void*)0x100000; // Start at 1 megabyte
    heap_end = image_end;
    pgdir = findpage();
    kzeromem(pgdir, 4096);
}

void page_unmap(void* logical)
{
    volatile unsigned* dir     = (volatile unsigned*)pgdir; // The page directory's address
    volatile unsigned* tbl     = (volatile unsigned*)(dir[(unsigned)logical>>22]&~0xFFF); // The page table's address
    unsigned           dir_ind = ((unsigned)logical>>22);       // The index into the page directory
    unsigned           tbl_ind = ((unsigned)logical>>12)&0x3FF; // The index into the page table
    if (!(dir[dir_ind]&1)) return; // Page table is not present, we needn't do any unmapping.
    tbl[tbl_ind] = 0; // Unmap it
    // Remove an empty page table.
    int found = 0, i;
    // If the table is empty, free the table.
    for (i = 0; i < 1024; i++) if (tbl[i]&1) { found = 1; break; }
    if (!found)
        dir[dir_ind] = 0;
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
    register_isr(0x0,(void*)int0);
    // Double fault
    register_isr(0x8,(void*)int8);
    // General protection fault
    register_isr(0xd,(void*)intd);
    // Page fault
    register_isr(0xe,(void*)inte);
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
    static char t[100];
    ksprintf(t, "A page fault occurred at address %l, with error code %l.", cr2, errorcode);
    dump_stack(t, edi, esi, ebp, esp+16, ebx, edx, ecx, eax, eip, cs);
    return;
}

static void bsod(const char* msg)
{
    cls();
    kprintf("Kernel Loader\nFatal error: %s\nThe system cannot boot.\nPlease power off your system now.", msg);
    cli();
    while (1) hlt();
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
    unsigned temp = (unsigned)(x < 0 ? -x : x);
    unsigned div;
    unsigned mod;
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
                case 't':
                    arg++;
                    ksprintdatetime(dest, *(DateTime*)val);
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

void ksprintdatetime(char* dest, DateTime dt)
{
    const char* month = "";
    switch (dt.Month)
    {
        case 1:
             month = "January";
             break;
        case 2:
             month = "February";
             break;
        case 3:
             month = "March";
             break;
        case 4:
             month = "April";
             break;
        case 5:
             month = "May";
             break;
        case 6:
             month = "June";
             break;
        case 7:
             month = "July";
             break;
        case 8:
             month = "August";
             break;
        case 9:
             month = "September";
             break;
        case 10:
             month = "October";
             break;
        case 11:
             month = "November";
             break;
        case 12:
             month = "December";
             break;
    }
    const char* eday = "th";
    if ((dt.Day < 10) || (dt.Day > 20))
    switch (dt.Day % 10)
    {
           case 1:
                eday = "st";
                break;
           case 2:
                eday = "nd";
                break;
           case 3:
                eday = "rd";
                break;
           default:
                eday = "th";
                break;
    }
    const char* min0 = dt.Minute < 10 ? "0" : "";
    const char* sec0 = dt.Second < 10 ? "0" : "";
    ksprintf(dest, "%s %d%s, %d  %d:%s%d %s%d %s", month, dt.Day, eday, dt.Year, (dt.Hour % 12) == 0 ? 12 : dt.Hour % 12, min0, dt.Minute, sec0, dt.Second, dt.Hour < 12 ? "AM" : "PM");
}

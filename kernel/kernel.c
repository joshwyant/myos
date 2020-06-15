#include "kernel.h"

static void demo();

loader_info loaderInfo;
void kmain(loader_info *li)
{
    // Make a local copy of li
    loaderInfo = *li;
    li = &loaderInfo;
    // System initialization
    cli(); // Clear interrupt flag
	init_gdt();
    // Initialize interrupt system
    init_pic(); // Programmable Interrupt Controller - remap IRQs
    init_idt(); // Interrupt Descriptor Table
    sti(); // Allow interrupts

    // Initialize memory management
    init_paging(li); // Now we must init exceptions, to enable page faults
    init_heap(); // Initialize the heap

    // Initialize exceptions
    init_exceptions(); // Dependent on IDT; Start right after paging for page faults
    
    // Initialize symbol table
    init_symbols(li);
    
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

	// Temporary VESA mode
	init_vesa();
	
    //demo();
	show_splash();
	
	init_mouse();

    // Load the shell
    start_shell();

    // Load vesadrvr.o
    if (!load_driver("/system/bin/vesadrvr.o"))
    {
        kprintf("Error: Could not load vesadrvr.o: %s\n", elf_last_error());
        freeze();
    }

    init_clock();

    // Unmask timer IRQ
    irq_unmask(0);
}

void init_symbols(loader_info *li)
{
    kernel_dynamic = _DYNAMIC; // _DYNAMIC is the ELF symbol for the .dyn section
    int i;
    // Get the hash, string, and symbol tables
    for (i = 0; _DYNAMIC[i].d_tag != DT_NULL; i++)
    {
        void* ptr = li->loaded + _DYNAMIC[i].d_un.d_val;
        switch (_DYNAMIC[i].d_tag)
        {
            case DT_SYMTAB:
                kernel_symtab = ptr;
                break;
            case DT_STRTAB:
                kernel_strtab = ptr;
                break;
            case DT_HASH:
                kernel_hashtable = ptr;
                break;
        }
    }
    kernel_nbucket = kernel_hashtable[0];
    kernel_nchain = kernel_hashtable[1];
    kernel_bucket = &kernel_hashtable[2];
    kernel_chain = &kernel_bucket[kernel_nbucket];
}

Elf32_Sym *find_symbol(const char* name)
{
    // Use the ELF hash table to find the symbol.
    int i = kernel_bucket[elf_hash(name)%kernel_nbucket];
    while (i != SHN_UNDEF)
    {
        if (kstrcmp(kernel_strtab + kernel_symtab[i].st_name, name) == 0)
            return kernel_symtab + i;
        i = kernel_chain[i];
    }
    return 0;
}

// Calls the function with the given name. Pretty useless, but neat.
void invoke(const char* function)
{
    Elf32_Sym *s = find_symbol(function);
    if (s && (ELF32_ST_TYPE (s->st_info) == STT_FUNC))
        asm volatile("call *%0"::"g"(s->st_value));
    else
        kprintf("Function '%s' not found\n", function);
}

void hello()
{
    kprintf("Hello!");
}

void init_loader_info()
{
	// Process all the data given to us by the loader
	// TODO: Pass pages of mapped data in a more structured way.
}

void show_splash()
{
	clear_color(RGB(0, 0, 0));
	
    Bitmap b;
    if (read_bitmap(&b, "/system/bin/splash"))
    {
		RECT src = {0, 0, b.width, b.height};
		RECT dest = {0, 0, vesaMode.width, vesaMode.height};
		for (int color = 0; color <= 255; color += 17)
		{
			draw_image_ext(&b, &src, &dest, 255, RGB(color, color, color));
		}
		bitblt(&b, 0, 0);
    }
    else
    {
        print("Bitmap could not be read.\r\n");
		clear_color(RGB(128, 128, 128));
    }
	
	// TODO: Remove
	RECT r = {64, 160, vesaMode.width - 64, vesaMode.height - 160};
	rect(&r, 1, 0, RGB(255, 255, 255), 0, 64);
	rect(&r, 0, 8, 0, RGB(0, 0, 128), 192);
	
	draw_text("Hello, world!\nI'm Josh!!", 96, 192, RGB(255, 0, 0), 192, 16);
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

static void demo()
{	
    cls();

    /*// Print all the kernel's symbols!!!
    int i;
    for (i = 1; i < kernel_nchain; i++)
    {
        kprintf("%l %s\n", kernel_symtab[i].st_value, kernel_strtab + kernel_symtab[i].st_name);
    }
    */

    static char buffer[64];

    //kprintf("Type a symbol name:\n");

    kprintf("Execute a function:\n");

    while (1)
    {
        kbd_readln(buffer, 63);

        /*Elf32_Sym *s = find_symbol(buffer);
        if (s)
        {
            kprintf("Symbol %s found: %l\n", kernel_strtab + s->st_name, s->st_value);
        }
        else
        {
            kprintf("Symbol '%s' not found\n", buffer);
        }*/

        invoke(buffer);
    }
    

    /*invoke("hello");

    while (1) ;*/
}

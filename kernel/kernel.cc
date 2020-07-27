#include "kernel.h"

#ifdef __cplusplus
#include <memory>

using namespace kernel;

extern "C" {
#endif

static void demo();

loader_info loaderInfo;

// Need to set up memory, heap, etc. before
// we can call global constructors, esp. for
// exception support, which depends on library
// support, which depends on memory management.
void _pre_init(loader_info *li)
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
}

void kmain()
{
    // Initialize FPU
    init_fpu();
    
    // Initialize symbol table
    init_symbols(&loaderInfo);

    auto manager = DriverManager::init();
    
    // Initialize video so we can display errors
    // Dependent on paging, Dependent on IDT (Page faults) (Maps video memory)
    auto text_driver
        = manager->register_text_console_driver(std::make_shared<TextModeConsoleDriver>());
    manager->register_console_driver(text_driver);

    // Processes
    init_processes();

    // Interrupt related
    kbd_init(); // dependent on IDT and PIC
    init_timer(); // dependent on IDT and PIC

    // Filestystem
    auto disk_driver
        = manager->register_disk_driver(std::make_shared<PIODiskDriver>());

    auto fat_driver
        = manager->register_file_system_driver(std::make_shared<FATDriver>(disk_driver));
    
    // System TSS
    init_tss();

    // System call interface (int 0x30)
    init_syscalls();

    if (loaderInfo.vbe)
    {
        // Temporary VESA mode
        auto graphics_driver 
            = manager->register_graphics_driver(std::make_shared<kernel::VESAGraphicsDriver>(fat_driver));

        show_splash(graphics_driver, fat_driver);
        
        auto mouse_driver 
            = manager->register_mouse_driver(std::make_shared<PS2MouseDriver>(graphics_driver, fat_driver));
        mouse_driver->start();

        // auto console
        //     = manager->register_console_driver(std::make_shared<GraphicalConsoleDriver>(fat_driver, 25, 80));
    }

    // Load the shell
    start_shell(fat_driver);
	
    //demo();

    // Load vesadrvr.o
    if (!load_driver(fat_driver, "/system/bin/vesadrvr.o"))
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
        void* ptr = (void *)((char *)li->loaded + _DYNAMIC[i].d_un.d_val);
        switch (_DYNAMIC[i].d_tag)
        {
            case DT_SYMTAB:
                kernel_symtab = (Elf32_Sym*)ptr;
                break;
            case DT_STRTAB:
                kernel_strtab = (char*)ptr;
                break;
            case DT_HASH:
                kernel_hashtable = (unsigned*)ptr;
                break;
        }
    }
    kernel_nbucket = kernel_hashtable[0];
    kernel_nchain = kernel_hashtable[1];
    kernel_bucket = &kernel_hashtable[2];
    kernel_chain = &kernel_bucket[kernel_nbucket];
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

static void demo()
{	
    //cls();

    /*// Print all the kernel's symbols!!!
    int i;
    for (i = 1; i < kernel_nchain; i++)
    {
        kprintf("%l %s\n", kernel_symtab[i].st_value, kernel_strtab + kernel_symtab[i].st_name);
    }
    */

    static char buffer[64];

    kprintf("Type a symbol name:\n");

    // kprintf("Execute a function:\n");

    while (1)
    {
        kbd_readln(buffer, 63);

        Elf32_Sym *s = find_symbol(buffer);
        if (s)
        {
            kprintf("Symbol %s found: %l\n", kernel_strtab + s->st_name, s->st_value);
        }
        else
        {
            kprintf("Symbol '%s' not found\n", buffer);
        }

        //invoke(buffer);
    }
    

    /*invoke("hello");

    while (1) ;*/
}

#ifdef __cplusplus
}  // extern "C"

void show_splash(std::shared_ptr<kernel::GraphicsDriver> graphics_driver, std::shared_ptr<kernel::FileSystemDriver> fs_driver)
{
    auto ctx = graphics_driver->get_screen_context();
	clear_color(RGB(0, 0, 0));
	
    Bitmap b;
    if (read_bitmap(fs_driver, &b, "/system/bin/splash"))
    {
		RECT src = {0, 0, b.width, b.height};
		RECT dest = {0, 0, ctx->get_width(), ctx->get_height()};
		for (int color = 0; color <= 255; color += 17)
		{
			//draw_image_ext(&b, &src, &dest, 255, RGB(color, color, color));
		}
		bitblt(&b, 0, 0);
    }
    else
    {
        print("Bitmap could not be read.\r\n");
		clear_color(RGB(128, 128, 128));
    }
	
	// TODO: Remove
	RECT r = {64, 160, ctx->get_width() - 64, ctx->get_height() - 160};
	//rect(&r, 1, 0, RGB(255, 255, 255), 0, 64);
	//rect(&r, 0, 8, 0, RGB(0, 0, 128), 192);
	
	KVector<const char*> strs;
	strs.push_back("Hello");
	strs.push_back("World");
	strs.push_back("How");
	strs.push_back("Are");
	strs.push_back("You?");
	
	for (auto i = 0; i < strs.len(); i++)
	{
		//draw_text(strs[i], 0, i * 32, RGB(255, 255, 255), 255, 16, 32);
	}
	
	static const char *str = "Hello, world!\nI'm Josh!!";
	int x = 96, y = 192, xsize = 16, ysize = 32;
	//draw_text(str, x + 2, y + 2, RGB(0, 0, 0), 85, xsize, ysize);  // shadow
	//draw_text(str, x, y, RGB(255, 0, 0), 255, xsize, ysize);
}

void start_shell(std::shared_ptr<kernel::FileSystemDriver> fs_driver)
{
    // Load the shell
    if (!process_start(fs_driver, "/system/bin/shell"))
    {
        kprintf("Error: Could not load the shell: %s\n", elf_last_error());
        freeze();
    }
}

#endif

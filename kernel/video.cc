#include "kernel.h"

static kernel::ConsoleDriver *console;
static kernel::TextModeConsoleDriver *text_mode_console;

#ifdef __cplusplus
extern "C" {
#endif

void init_video()
{
    console
        = text_mode_console 
        = new kernel::TextModeConsoleDriver();
}
void init_graphical_console()
{
    console = new kernel::GraphicalConsoleDriver(*text_mode_console);
}
volatile char *get_console_videomem()
{
    return console->get_videomem();
}
int get_console_rows()
{
    return console->get_rows();
}
int get_console_cols()
{
    return console->get_cols();
}
void move_cursor(int pos)
{
    console->move_cursor(pos);
}
void print_char(char c)
{
    console->print_char(c);
}
void print(const char* str)
{
    console->print(str);
}
void printlen(const char* str, int len)
{
    console->printlen(str, len);
}
void show_cursor(int show)
{
    console->show_cursor(show);
}
void cls()
{
    console->cls();
}
void printhexb(char x)
{
    console->printhexb(x);
}
void printhexw(short x)
{
    console->printhexw(x);
}
void printhexd(int x)
{
    console->printhexd(x);
}
void printdec(int x)
{
    console->printdec(x);
}
void kprintdatetime(DateTime dt)
{
    console->kprintdatetime(dt);
}
void kprintf(const char* format, ...)
{
    console->kprintf(format); // Possible due to the implementation of kprintf
}
void endl()
{
    console->endl();
}
// void print_datetime()
//{
//    console->print_datetime();
//}

#ifdef __cplusplus
}  // extern "C"
#endif

kernel::GraphicalConsoleDriver::GraphicalConsoleDriver(int rows, int cols)
    : ConsoleDriver(nullptr, rows, cols)
{
    read_bitmap(&font, "/system/bin/font");
}

kernel::GraphicalConsoleDriver::GraphicalConsoleDriver(const TextModeConsoleDriver &original)
    : GraphicalConsoleDriver(original.get_rows(), original.get_cols())
{
    // Copy the text from the original video memory
    volatile short *orig_videomem = (volatile short *)(void*)original.get_videomem();
    for (auto i = 0; i < rows * cols; i++)
    {
        ((volatile short*)(void*)videomem)[i] = orig_videomem[i];
    }
}

void kernel::GraphicalConsoleDriver::show_cursor(int show)
{
    while (lock(&show_cursorlock)) process_yield();
    if (show?cursor_shown?0:1:cursor_shown?1:0)
    {
        cursor_shown = show;

        // TODO: Update display
    }
    show_cursorlock = 0; // Unlock show_cursor
}
void kernel::GraphicalConsoleDriver::update_cursor_index()
{
    // TODO
}

kernel::TextModeConsoleDriver::TextModeConsoleDriver()
    : ConsoleDriver((char*)(void*)0xB800, 80, 25)
{
    // Map video memory
    videomem = (volatile char*)kfindrange(4000);
    page_map((void*)videomem, (void*)0xB8000, PF_WRITE);
    // save CRT base IO port (map BDA first)
    void* bda = kfindrange(0x465);
    page_map(bda, 0, PF_WRITE);
    crtbaseio = *(volatile unsigned short*)(void*)((char*)bda+0x0463);
    page_unmap(bda);
    // get cursor position:
    //   get lo port from index register
    outb(crtbaseio, 0xF);
    cursorpos = inb(crtbaseio+1);
    //   get hi port from index register
    outb(crtbaseio, 0xE);
    cursorpos |= inb(crtbaseio+1)<<8;
    // check if cursor is hidden
    outb(crtbaseio, 0xA);
    cursor_shown = !(inb(crtbaseio+1)&0x20); // It seems the ! fixes it, but I need to look this up to be sure
}

void kernel::ConsoleDriver::move_cursor(int pos)
{
    while (lock(&cursorlock)) process_yield();
    cursorpos = pos;
    update_cursor_index();
    cursorlock = 0;
}

void kernel::ConsoleDriver::print_char(char c)
{
    __print_char(c, 7);
    update_cursor_index();
}

void kernel::ConsoleDriver::print(const char* str)
{
    int s = cursor_shown;
    if (s)
        show_cursor(0);
    while (*str != 0)
    {
        __print_char(*str,7);
        str++;
    }
    update_cursor_index();
    if (s)
        show_cursor(1);
}

void kernel::ConsoleDriver::printlen(const char* str, int len)
{
    int s = cursor_shown;
    if (s) show_cursor(0);
    while (len--) __print_char(*str++,7);
    update_cursor_index();
    if (s) show_cursor(1);
}

void kernel::TextModeConsoleDriver::show_cursor(int show)
{
    while (lock(&show_cursorlock)) process_yield();
    if (show?cursor_shown?0:1:cursor_shown?1:0)
    {
        cursor_shown = show;
        outb(crtbaseio,0xA);
        char c = show ? inb(crtbaseio+1)&0xDF : inb(crtbaseio+1)|0x20;
        //outb(crtbaseio,0xA); // Is this necessary?
        outb(crtbaseio+1, c);
    }
    show_cursorlock = 0; // Unlock show_cursor
}

void kernel::ConsoleDriver::cls()
{
    while (lock(&screenlock)) process_yield();
    volatile short* svideomem = (volatile short*)(videomem);
    int i;
    for (i = 0; i < rows * cols; i++, svideomem++)
    {
        *svideomem = 0x0720;
    }
    while (lock(&cursorlock)) process_yield(); // lock cursor io
    cursorpos = 0;
    update_cursor_index();
    cursorlock = 0;
    screenlock = 0;
}

void kernel::ConsoleDriver::printhexb(char c)
{
    char str[3] = {(char)('0'+((c&0xF0)>>4)),(char)('0'+(c&0xF)),0};
    if (str[0] > '9') str[0] += 7;
    if (str[1] > '9') str[1] += 7;
    print(str);
    while (lock(&cursorlock)) process_yield();
    update_cursor_index();
    cursorlock = 0;
}

void kernel::ConsoleDriver::printhexw(short w)
{
    printhexb(w>>8);
    printhexb(w);
}

void kernel::ConsoleDriver::printhexd(int d)
{
    printhexw(d>>16);
    printhexw(d);
}

void kernel::ConsoleDriver::printdec(int x)
{
    unsigned temp = (unsigned)(x < 0 ? -x : x);
    unsigned div;
    unsigned mod;
    char str[11];
    char str2[11];
    char *strptr = str;
    char *strptr2 = str2;
    if (x < 0) _print_char('-',7);
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
    print(str2);
}

void kernel::ConsoleDriver::kprintf(const char* format, ...)
{
    int s = cursor_shown;
    if (s)
        show_cursor(0);
    int arg = 0;
    while (*format)
    {
        volatile unsigned val;
        // Skip 3 items on the stack at %ebp: prev %epb, %eip, and $format.
        asm volatile ("movl (%%ebp,%1), %0":"=r"(val):"r"((arg+3)*4));
        if (*format == '%')
        {
            switch (*++format)
            {
                case 'd':
                    arg++;
                    printdec(val);
                    break;
                case 's':
                    arg++;
                    print((const char*)val);
                    break;
                case 'c':
                    arg++;
                    print_char((char)val);
                    break;
                case 'b':
                    arg++;
                    print("0x");
                    printhexb(val);
                    break;
                case 't':
                    arg++;
                    kprintdatetime(*(DateTime*)val);
                    break;
                case 'w':
                    arg++;
                    print("0x");
                    printhexw(val);
                    break;
                case 'l':
                    arg++;
                    print("0x");
                    printhexd(val);
                    break;
                case '%':
                    print_char('%');
                    break;
                default:
                    print_char('%');
                    print_char(*format);
                    break;
            }
        }
        else
        {
            print_char(*format);
        }
        format++;
    }
    if (s)
        show_cursor(1);
}

void kernel::ConsoleDriver::endl()
{
    print("\n");
}

void kernel::ConsoleDriver::kprintdatetime(DateTime dt)
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
    kprintf("%s %d%s, %d  %d:%s%d %s%d %s", month, dt.Day, eday, dt.Year, (dt.Hour % 12) == 0 ? 12 : dt.Hour % 12, min0, dt.Minute, sec0, dt.Second, dt.Hour < 12 ? "AM" : "PM");
}

/*void kernel::ConsoleDriver::print_datetime()
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
}*/

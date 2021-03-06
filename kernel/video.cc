#include "drivers.h"
#include "drawing.h"
#include "kernel.h"
#include "video.h"

using namespace kernel;

extern "C" {

int console_palette[] = {
    RGB(0, 0, 0),       // Black        (0)
    RGB(0, 0, 170),     // Blue         (1)
    RGB(0, 170, 0),     // Green        (2)
    RGB(0, 170, 170),   // Cyan         (3)
    RGB(170, 0, 0),     // Red          (4)
    RGB(170, 0, 170),   // Magenta      (5)
    RGB(170, 85, 0),    // Brown        (6)
    RGB(170, 170, 170), // Light Gray   (7)
    RGB(85, 85, 85),    // Dark Gray    (8)
    RGB(85, 85, 255),   // Light Blue   (9)
    RGB(85, 255, 85),   // Light Green  (10)
    RGB(85, 255, 255),  // Light Cyan   (11)
    RGB(255, 85, 85),   // Light Red    (12)
    RGB(255, 85, 255),  // Pink         (13)
    RGB(255, 255, 85),  // Yellow       (14)
    RGB(255, 255, 255)  // White        (15)
};

volatile char *get_console_videomem()
{
    return Kernel::current()
                ->drivers()
                ->console_driver()
                ->get_videomem();
}
int get_console_rows()
{
    return Kernel::current()
                ->drivers()
                ->console_driver()
                ->get_rows();
}
int get_console_cols()
{
    return Kernel::current()
                ->drivers()
                ->console_driver()
                ->get_cols();
}
void move_cursor(int pos)
{
    Kernel::current()
                ->drivers()
                ->console_driver()
                ->move_cursor(pos);
}
void print_char(char c)
{
    Kernel::current()
                ->drivers()
                ->console_driver()
                ->print_char(c);
}
void print(const char* str)
{
    Kernel::current()
                ->drivers()
                ->console_driver()
                ->print(str);
}
void printlen(const char* str, int len)
{
    Kernel::current()
                ->drivers()
                ->console_driver()
                ->printlen(str, len);
}
void show_cursor(int show)
{
    Kernel::current()
                ->drivers()
                ->console_driver()
                ->show_cursor(show);
}
void cls()
{
    Kernel::current()
                ->drivers()
                ->console_driver()
                ->cls();
}
void cls_color(int fore, int back)
{
    Kernel::current()
                ->drivers()
                ->console_driver()
                ->cls(fore, back);
}
void printhexb(char x)
{
    Kernel::current()
                ->drivers()
                ->console_driver()
                ->printhexb(x);
}
void printhexw(short x)
{
    Kernel::current()
                ->drivers()
                ->console_driver()
                ->printhexw(x);
}
void printhexd(int x)
{
    Kernel::current()
                ->drivers()
                ->console_driver()
                ->printhexd(x);
}
void printdec(int x)
{
    Kernel::current()
                ->drivers()
                ->console_driver()
                ->printdec(x);
}
void kprintdatetime(DateTime dt)
{
    Kernel::current()
                ->drivers()
                ->console_driver()
                ->kprintdatetime(dt);
}
void kprintf(const char* format, ...)
{
    Kernel::current()
                ->drivers()
                ->console_driver()
                ->kprintf(format); // Possible due to the implementation of kprintf
}
void endl()
{
    Kernel::current()
                ->drivers()
                ->console_driver()
                ->endl();
}
// void print_datetime()
// {
//    Kernel::current()
//                 ->drivers()
//                 ->console_driver()
//                 ->print_datetime();
// }

}  // extern "C"

kernel::GraphicalConsoleDriver::GraphicalConsoleDriver(std::shared_ptr<FileSystem> fs, int rows, int cols)
    : fs(fs),
      ConsoleDriver(rows, cols)
{
    read_bitmap(fs, &font, "/system/bin/font");
    display = new MemoryGraphicsContext(fs, nullptr, 24, cols * 8, rows * 16);
    update_cursor_index();
}

kernel::GraphicalConsoleDriver::~GraphicalConsoleDriver()
{
    delete display;
}

void kernel::GraphicalConsoleDriver::show_cursor(int show)
{
    {   ScopedLock lock(show_cursorlock);
        if (show?cursor_shown?0:1:cursor_shown?1:0)
        {
            cursor_shown = show;
        }
    } // show_cursorlock
    update_cursor_index();
}
void kernel::GraphicalConsoleDriver::redraw(int pos)
{
    int j = pos / cols;
    int i = pos % cols;
    RECT char_rect = {i * 8, j * 16, (i+1)*8, (j+1)*16};
    int back = console_palette[(unsigned char)videomem[pos * 2 + 1] >> 4];
    int fore = console_palette[(unsigned char)videomem[pos * 2 + 1] & 0xF];
    if (cursor_shown && pos == cursorpos)
    {
        swap_int(&back, &fore);
    }
    display->rect(&char_rect, 1, 0, back, 0, 255);
    char c[] = { videomem[pos * 2], 0 };
    if (c[0] >= 32 && c[0] <= 127)
    {
        display->draw_text(c, char_rect.x1, char_rect.y1, fore, 255, 8, 16);
    }
    Kernel::current()
        ->drivers()
        ->graphics_driver()
        ->get_screen_context()
        ->get_raw_context()
        ->draw_image_ext(
            display->as_bitmap(), 
            &char_rect, 
            &char_rect, 
            255, 
            RGB(255, 255, 255));
}

void kernel::GraphicalConsoleDriver::redraw()
{
    for (int i = 0; i < rows * cols; i++) redraw(i);
}

void kernel::GraphicalConsoleDriver::update_cursor_index()
{
    redraw(lastpos);
    lastpos = cursorpos;
    redraw(cursorpos);
}

kernel::TextModeConsoleDriver::TextModeConsoleDriver()
    : ConsoleDriver(reinterpret_cast<void*>(0xB8000))
{
    // save CRT base IO port (map BDA first)
    {   MappedMemory<char> bda(reinterpret_cast<void*>(0), PF_WRITE);
        crtbaseio = *(volatile unsigned short*)(void*)(bda.get()+0x0463);
    }
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

    cls();
}

void kernel::ConsoleDriver::move_cursor(int pos)
{
    ScopedLock lock(cursorlock);
    cursorpos = pos;
    update_cursor_index();
}

void kernel::ConsoleDriver::print_char(char c)
{
    __print_char(c, fore_color | (back_color << 4));
    update_cursor_index();
}

void kernel::ConsoleDriver::print(const char* str)
{
    int s = cursor_shown;
    if (s)
        show_cursor(0);
    while (*str != 0)
    {
        __print_char(*str, fore_color | (back_color << 4));
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
    while (len--) __print_char(*str++, fore_color | (back_color << 4));
    update_cursor_index();
    if (s) show_cursor(1);
}

void kernel::TextModeConsoleDriver::show_cursor(int show)
{
    {   ScopedLock lock(show_cursorlock);
        if (show?cursor_shown?0:1:cursor_shown?1:0)
        {
            cursor_shown = show;
            outb(crtbaseio,0xA);
            char c = show ? inb(crtbaseio+1)&0xDF : inb(crtbaseio+1)|0x20;
            //outb(crtbaseio,0xA); // Is this necessary?
            outb(crtbaseio+1, c);
        }
    } // show_cursorlock
    update_cursor_index();
}

void kernel::ConsoleDriver::cls()
{
    ScopedLock lock(screenlock);
    volatile short* svideomem = (volatile short*)(videomem);
    int i;
    unsigned short ch = (unsigned short)' ' | (fore_color << 8) | (back_color << 12);
    for (i = 0; i < rows * cols; i++, svideomem++)
    {
        *svideomem = ch;
    }
    ScopedLock lock1(cursorlock);
    cursorpos = 0;
    redraw();
    update_cursor_index();
}

void kernel::ConsoleDriver::printhexb(char c)
{
    char str[3] = {(char)('0'+((c&0xF0)>>4)),(char)('0'+(c&0xF)),0};
    if (str[0] > '9') str[0] += 7;
    if (str[1] > '9') str[1] += 7;
    print(str);
    ScopedLock lock(cursorlock);
    update_cursor_index();
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
    if (x < 0) _print_char('-', fore_color | (back_color << 4));
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

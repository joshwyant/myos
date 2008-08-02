#include "klib.h"
#include "video.h"

static inline void _print_char(char, unsigned char);
static inline void update_cursor_index();

static unsigned short crtbaseio;
static char cursor_shown;
static unsigned short cursorpos;

void init_video()
{
    // Map video memory
    videomem = kfindrange(4000);
    page_map((void*)videomem,(void*)0xB8000);
    // save CRT base IO port (map BDA first)
    void* bda = kfindrange(0x465);
    page_map(bda,0);
    crtbaseio = *((volatile unsigned short*)(bda+0x0463));
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
    cursor_shown = inb(crtbaseio+1)&0x20;
}

void move_cursor(int pos)
{
    cursorpos = pos;
    update_cursor_index();
}

static void _print_char(char c, unsigned char color)
{
    int i;
    volatile unsigned short* line1 = 
        (volatile unsigned short*)videomem;
    volatile unsigned short* line2 = 
        (volatile unsigned short*)(videomem+0xA0);
    if (c == 8) // backspace
    {
        if (cursorpos != 0) cursorpos--;
    }
    else if (c == 10) // lf
        cursorpos += 80;
    else if (c == 13) // cr
        cursorpos -= cursorpos%80;
    else if ((c >= 0x20) && (c < 0x7F))
    {
        *(volatile unsigned short*)(videomem+cursorpos*2) = 
            (unsigned char)c|(color<<8);
        cursorpos++;
    }
    if (cursorpos >= 2000)
    {
        cursorpos -= 80;
        for (i = 0; i < 1920; i++, line1++, line2++)
            *line1 = *line2;
        line1 = (volatile unsigned short*)(videomem+0xF00);
        for (i = 0; i < 80; i++, line1++)
            *line1 = 0x0720;
    }
}

void print_char(char c)
{
    _print_char(c, 7);
    update_cursor_index();
}

void print(const char* str)
{
    //show_cursor(0);
    while (*str != 0)
    {
        _print_char(*str,7);
        str++;
    }
    update_cursor_index();
    //show_cursor(1);
}

static void update_cursor_index()
{
    outb(crtbaseio,0xF);
    outb(crtbaseio+1,(unsigned char)(cursorpos));
    outb(crtbaseio,0xE);
    outb(crtbaseio+1,(unsigned char)(cursorpos>>8));
}

void show_cursor(int show)
{
    // TODO: Doesn't work?
    if (show?cursor_shown?0:1:cursor_shown?1:0)
    {
        cursor_shown = show;
        outb(crtbaseio,0xA);
        outb(crtbaseio+1, show ? inb(crtbaseio+1)|0x20 : inb(crtbaseio+1)&0xDF);
    }
}

void cls()
{
    volatile int* ivideomem = (volatile int*)(videomem);
    int i;
    for (i = 0; i < 1000; i++, ivideomem++)
    {
        *ivideomem = 0x07200720;
    }
    cursorpos = 0;
    update_cursor_index();
}

void printhexb(char c)
{
    char str[3] = {'0'+((c&0xF0)>>4),'0'+(c&0xF),0};
    if (str[0] > '9') str[0] += 7;
    if (str[1] > '9') str[1] += 7;
    print(str);
    update_cursor_index();
}

void printhexw(short w)
{
    printhexb(w>>8);
    printhexb(w);
}

void printhexd(int d)
{
    printhexw(d>>16);
    printhexw(d);
}

void printdec(int x)
{
    if (x == (int)0x80000000)
    {
        print("-2147483648");
        return;
    }
    int temp = x < 0 ? -x : x;
    int div;
    int mod;
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

void endl()
{
    print("\r\n");
}

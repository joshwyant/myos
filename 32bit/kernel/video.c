#include "video.h"

void init_video()
{
    // save CRT base IO port
    crtbaseio = *((volatile unsigned short*)(0x0463));
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

void _print_char(char c, unsigned char color)
{
    int i;
    volatile unsigned short* line1 = 
        (volatile unsigned short*)0xB8000;
    volatile unsigned short* line2 = 
        (volatile unsigned short*)0xB80A0;
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
        *(volatile unsigned short*)(0xB8000+cursorpos*2) = 
            (unsigned char)c|(color<<8);
        cursorpos++;
    }
    if (cursorpos >= 2000)
    {
        cursorpos -= 80;
        for (i = 0; i < 1920; i++, line1++, line2++)
            *line1 = *line2;
        line1 = (volatile unsigned short*)0xB8F00;
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
    //show_cursor(false);
    while (*str != 0)
    {
        _print_char(*str,7);
        str++;
    }
    update_cursor_index();
    //show_cursor(true);
}

void update_cursor_index()
{
    outb(crtbaseio,0xF);
    outb(crtbaseio+1,(unsigned char)(cursorpos));
    outb(crtbaseio,0xE);
    outb(crtbaseio+1,(unsigned char)(cursorpos>>8));
}

void show_cursor(bool show)
{
    if (show?cursor_shown?0:1:cursor_shown?1:0)
    {
        //cursor_shown = show;
        outb(crtbaseio,0xA);
        outb(crtbaseio+1, show ? inb(crtbaseio+1)|0x20 : inb(crtbaseio+1)&0xDF);
    }
}

void cls()
{
    volatile int* videomem = (volatile int*)0xB8000;
    int i;
    for (i = 0; i < 1000; i++, videomem++)
    {
        *videomem = 0x07200720;
    }
    cursorpos = 0;
    update_cursor_index();
}

void printhexb(char c)
{
    char str[3] = {'0'+((c&0xF0)>>4),'0'+(c&0xF),0};
    if (str[0] > '9') str[0] += 7;
    if (str[1] > '9') str[1] += 7;
    //print(str);
    _print_char(str[0],(c&0xF0)|7);
    _print_char(str[1],((c&0xF)<<4)|7);
    update_cursor_index();
}

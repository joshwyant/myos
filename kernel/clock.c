#include "clock.h"
#include "io.h"
#include "interrupt.h"

void init_clock()
{
    // TODO: Figure out how to do this (I want irq8 fired each second)
    register_isr(0x28,0,(void*)irq8);
    irq_unmask(8);
}

void handle_clock()
{
    eoi(8);
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

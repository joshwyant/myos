#ifndef __IO_H__
#define __IO_H__

#include "ktypes.h"

struct IDTDescr
{
   unsigned short offset_1; // offset bits 0..15
   unsigned short selector; // a code segment selector in GDT or LDT
   unsigned char zero;      // unused, set to 0
   unsigned char type_attr; // type and attributes, see below
   unsigned short offset_2; // offset bits 16..31
};

struct IDTr
{
    unsigned short         alignment __attribute__ ((packed));
    unsigned short         limit __attribute__ ((packed));
    unsigned int           base __attribute__ ((packed));

} __attribute__ ((aligned (4)));;

static inline void outb(unsigned short port, unsigned char val)
{
   asm volatile ("outb %0,%1"::"a"(val), "Nd" (port));
}

static inline void outw(unsigned short port, unsigned short val)
{
   asm volatile ("outw %0,%1"::"a"(val), "Nd" (port));
}

static inline void outl(unsigned short port, unsigned int val)
{
   asm volatile ("outl %0,%1"::"a"(val), "Nd" (port));
}

static inline unsigned char inb(unsigned short port)
{
   unsigned char ret;
   asm volatile ("inb %1,%0":"=a"(ret):"Nd"(port));
   return ret;
}

static inline unsigned short inw(unsigned short port)
{
   unsigned short ret;
   asm volatile ("inw %1,%0":"=a"(ret):"Nd"(port));
   return ret;
}

static inline unsigned int inl(unsigned short port)
{
   unsigned int ret;
   asm volatile ("inl %1,%0":"=a"(ret):"Nd"(port));
   return ret;
}

static inline void io_wait(void)
{
   asm volatile("outb %%al, $0x80" : : "a"(0));
}

static void lidt(void* base, unsigned int size /* limit+1 */)
{
    volatile unsigned int i[2];
    i[0] = (size-1) << 16;
    i[1] = (unsigned int) base;
    asm volatile ("lidt %0" : : "m" (((char*)i)[2]));
}

static void lgdt(void *base, unsigned int size) {
    unsigned int i[2];
 
    i[0] = (size-1) << 16;
    i[1] = (unsigned int) base;
    asm volatile ("lgdt (%0)": :"p" (((char *) i)+2));
}

static inline void cli() { asm volatile ("cli"); }

static inline void sti() { asm volatile ("sti"); }

static void PIC_remap(int offset1, int offset2)
{
    unsigned char a1, a2;
  
    a1 = inb(0x21);                        // save masks
    a2 = inb(0xa1);
 
    outb(0x20, 0x11);  // starts the initialization sequence
    io_wait();
    outb(0xa0, 0x11);
    io_wait();
    outb(0x21, offset1);                 // define the PIC vectors
    io_wait();
    outb(0xa1, offset2);
    io_wait();
    outb(0x21, 4);                       // continue initialization sequence
    io_wait();
    outb(0xa1, 2);
    io_wait();
    
    outb(0x21, 1); // ICW4_8086
    io_wait();
    outb(0xa1, 1);
    io_wait();
    
    outb(0x21, a1);   // restore saved masks.
    outb(0xa1, a2);

}

static inline void eoi(int irq)
{
    if (irq >= 8) outb(0xa0,0x20);
    outb(0x20,0x20);
}

#endif

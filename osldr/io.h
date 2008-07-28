#ifndef __IO_H__
#define __IO_H__

struct IDTDescr
{
   unsigned short offset_1; // offset bits 0..15
   unsigned short selector; // a code segment selector in GDT or LDT
   unsigned char zero;      // unused, set to 0
   unsigned char type_attr; // type and attributes, see below
   unsigned short offset_2; // offset bits 16..31
};

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

static inline void int86(unsigned char num)
{
   asm volatile("int %0"::"N"(num));
}

static inline void lidt(volatile void* base, unsigned int size /* limit+1 */)
{
    volatile unsigned int i[2];
    i[0] = (size-1) << 16;
    i[1] = (unsigned int) base;
    asm volatile ("lidt %0" : : "m" (((char*)i)[2]));
}

static inline void lgdt(volatile void *base, unsigned int size) {
    volatile unsigned int i[2];
    i[0] = (size-1) << 16;
    i[1] = (unsigned int) base;
    asm volatile ("lgdt %0" : : "m" (((char*)i)[2]));
}

static inline void cli() { asm volatile ("cli"); }

static inline void sti() { asm volatile ("sti"); }

static inline void irq_unmask(char irq)
{
    unsigned short mask = (0xFFFE<<irq)|(0xFFFE>>(16-irq));
    if (~(unsigned char)(mask&0xFF))
      outb(0x21, inb(0x21)&(unsigned char)(mask));
    else
      outb(0xa1, inb(0xa1)&(unsigned char)(mask>>8));
}

static inline void eoi(int irq)
{
    if (irq >= 8) outb(0xa0,0x20);
    outb(0x20,0x20);
}

static inline void invlpg(void* pg)
{
    // FIXED BUG!!!
    // was: asm volatile ("invlpg %0"::"m"(pg));
    asm volatile ("invlpg (%0)"::"r"((unsigned)pg));
}

static inline void hlt()
{
    asm volatile ("hlt");
}

static inline void hang()
{
    asm volatile ("0: jmp 0b");
}

static inline void freeze()
{
    cli();
    while (1) hlt();
}


#endif

#include "disk.h"
#include "io.h"
#include "process.h"

void kernel::PIODiskDriver::read_sectors(char *buffer, int sectors, int lba)
{
    // TODO: Lock disk buffer into memory >>here<<
    while (lock(&disklock)) process_yield();
    int base = controller ? 0x170 : 0x1F0;
    outb(base+1,0);
    outb(base+2, sectors);
    outb(base+3, lba&0xFF);
    outb(base+4, (lba >> 8)&0xFF);
    outb(base+5, (lba >> 16)&0xFF);
    outb(base+6, 0xE0|(drive ? 0x10 : 0)|((lba >> 24)&0xF));
    outb(base+7, 0x20);
    while (!(inb(base+7)&8)) ;
    int i;
    for (i = 0; i < 256; i++) ((unsigned short*)buffer)[i] = inw(base);
    disklock = 0;
}

void kernel::PIODiskDriver::write_sectors(const char *buffer, int sectors, int lba)
{
    // TODO: Lock disk buffer into memory >>here<<
    while (lock(&disklock)) process_yield();
    int base = controller ? 0x170 : 0x1F0;
    outb(base+1,0);
    outb(base+2, sectors);
    outb(base+3, lba&0xFF);
    outb(base+4, (lba >> 8)&0xFF);
    outb(base+5, (lba >> 16)&0xFF);
    outb(base+6, 0xE0|(drive ? 0x10 : 0)|((lba >> 24)&0xF));
    outb(base+7, 0x30);
    while (!(inb(base+7)&8)) ;
    int i;
    for (i = 0; i < 256; i++) outw(base, ((unsigned short*)buffer)[i]);
    disklock = 0;
}

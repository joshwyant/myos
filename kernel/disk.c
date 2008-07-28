#include "disk.h"
#include "klib.h"

/*
void ReadSectorsCHS(char* buffer, int sectors, int controller, int drive, int cylinder, int head, int sector, int bps)
{
    int base = controller ? 0x170 : 0x1F0;
    outb(base+2, sectors);
    outb(base+3, sector);
    outb(base+4, cylinder&0xFF);
    outb(base+5, (cylinder>>8)*0xFF);
    outb(base+6, 0xA0|head|(drive ? 0x10 : 0));
    outb(base+7, 0x20);
    while (!(inb(base+7)&8)) ;
    int i;
    for (i = 0; i < bps/2*sectors; i++) ((unsigned short*)buffer)[i] = inw(base);
}

void WriteSectorsCHS(char* buffer, int sectors, int controller, int drive, int cylinder, int head, int sector, int bps)
{
    int base = controller ? 0x170 : 0x1F0;
    outb(base+2, sectors);
    outb(base+3, sector);
    outb(base+4, cylinder&0xFF);
    outb(base+5, (cylinder>>8)*0xFF);
    outb(base+6, 0xA0|head|(drive ? 0x10 : 0));
    outb(base+7, 0x30);
    while (!(inb(base+7)&8)) ;
    int i;
    for (i = 0; i < bps/2*sectors; i++) outw(base, ((unsigned short*)buffer)[i]);
}
*/

static int disklock;

void ReadSectors(char* buffer, int sectors, int controller, int drive, int lba)
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

void WriteSectors(char* buffer, int sectors, int controller, int drive, int lba)
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

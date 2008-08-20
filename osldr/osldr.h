#ifndef __OSLDR_H__
#define __OSLDR_H__

#include "video.h"
#include "fat.h"
#include "disk.h"
#include "ldrlib.h"

// externals
extern void irq1(); // kerboard isr
extern void int0(); // divide error isr
extern void int8(); // double fault
extern void intd(); // gpf
extern void inte(); // pgf
extern char loader[];
extern char image_end[];

// main
void init32_main();
// initialization
void init_idt();
void init_exceptions();
void init_pic();
void init_mem();
void kbd_init();
// exceptions
void divide_error(unsigned edi, unsigned esi, unsigned ebp, unsigned esp, unsigned ebx, unsigned edx, unsigned ecx, unsigned eax, unsigned eip, unsigned cs, unsigned eflags);
void gpfault(unsigned edi, unsigned esi, unsigned ebp, unsigned esp, unsigned ebx, unsigned edx, unsigned ecx, unsigned eax, unsigned errorcode, unsigned eip, unsigned cs, unsigned eflags);
void pgfault(unsigned edi, unsigned esi, unsigned ebp, unsigned esp, unsigned ebx, unsigned edx, unsigned ecx, unsigned eax, unsigned errorcode, unsigned eip, unsigned cs, unsigned eflags);
void dobule_fault(unsigned edi, unsigned esi, unsigned ebp, unsigned esp, unsigned ebx, unsigned edx, unsigned ecx, unsigned eax, unsigned errorcode, unsigned eip, unsigned cs, unsigned eflags);
static void dump_stack(const char*, unsigned, unsigned, unsigned, unsigned, unsigned, unsigned, unsigned, unsigned, unsigned, unsigned);
static void bsod(const char*);

// keyboard data
static const char kbd_lowercase[] = { 0,0,'1','2','3','4','5','6','7','8','9','0','-','=','\b','\t','q','w','e','r','t','y','u','i','o','p','[',']','\r',0,'a','s','d','f','g','h','j','k','l',';','\'','`',0,'\\','z','x','c','v','b','n','m',',','.','/',0,'*',0,'\x20',0,0,0,0,0,0,0,0,0,0,0,0,0,'7','8','9','-','4','5','6','+','1','2','3','0','.',0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 };
static const char kbd_uppercase[] = { 0,0,'!','@','#','$','%','^','&','*','(',')','_','+','\b','\t','Q','W','E','R','T','Y','U','I','O','P','{','}','\r',0,'A','S','D','F','G','H','J','K','L',':','\"','~',0,'|', 'Z','X','C','V','B','N','M','<','>','?',0,'*',0,'\x20',0,0,0,0,0,0,0,0,0,0,0,0,0,'7','8','9','-','4','5','6','+','1','2','3','0','.',0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 };
static int kbd_escaped;
static int kbd_shift;

// other stuff
void* next_page;
void* heap_end;
void* pgdir;

// global descriptor table
static volatile unsigned int __attribute__ ((aligned(8))) gdt[] = {
    0x00000000,0x00000000, // 0x0000 Unused - null descriptor
    0x0000FFFF,0x00CF9A00, // 0x0008 DPL0 code - kernel
    0x0000FFFF,0x00CF9200, // 0x0010 DPL0 data - kernel
};

// interrupt descriptor table
static volatile int __attribute__ ((aligned(8))) idt [512];


#endif

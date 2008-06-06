#ifndef __kernel_h__
#define __kernel_h__

#include "video.h"
#include "fat.h"
#include "disk.h"
#include "klib.h"

// externals
extern void irq0(); // kerboard isr
extern void irq1(); // kerboard isr
extern void int0(); // divide error isr
extern void int8(); // double fault
extern void intd(); // gpf
extern void inte(); // pgf
extern char kernel[];
extern char image_end[];
extern void* kernel_end;

// main
void kmain();
// initialization
void kernel_init(); // nonstatic so compiler doesn't optimize it, and we can jump right over it in bochs
void init_paging();
void init_heap();
void init_idt();
void init_exceptions();
void init_pic();
void kbd_init();
void init_timer();
void init_processes();
void swap_start();
// exceptions
void divide_error();
void gpfault();
void pgfault(unsigned int errcode);
void double_fault();
static void dump_stack(const char*);
// Memory
static inline void* palloc(int,void*,void*);
static inline void  mark_pages(void*, int, int);
static inline void  mark_page(void*, int);
static int ksbrk(int);

// keyboard data
static const char kbd_lowercase[] = { 0,0,'1','2','3','4','5','6','7','8','9','0','-','=','\b','\t','q','w','e','r','t','y','u','i','o','p','[',']','\r',0,'a','s','d','f','g','h','j','k','l',';','\'','`',0,'\\','z','x','c','v','b','n','m',',','.','/',0,'*',0,'\x20',0,0,0,0,0,0,0,0,0,0,0,0,0,'7','8','9','-','4','5','6','+','1','2','3','0','.',0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 };
static const char kbd_uppercase[] = { 0,0,'!','@','#','$','%','^','&','*','(',')','_','+','\b','\t','Q','W','E','R','T','Y','U','I','O','P','{','}','\r',0,'A','S','D','F','G','H','J','K','L',':','\"','~',0,'|', 'Z','X','C','V','B','N','M','<','>','?',0,'*',0,'\x20',0,0,0,0,0,0,0,0,0,0,0,0,0,'7','8','9','-','4','5','6','+','1','2','3','0','.',0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 };
static int kbd_escaped;
static int kbd_shift;


// count of RAM
static unsigned int total_memory;

// Paging
static void* page_bitmap;
static void* page_directory;
static int page_bitmap_size;

// Memory Allocation
static struct FreeNode
{
    void* ptr;
    unsigned int size;
    struct FreeNode* prev;
    struct FreeNode* next;
} *first_free, *last_free;
struct MemHeader
{
    unsigned int size;
};
static void* heap_start;
static void* heap_end;
static void* heap_brk;

// interrupt descriptor table
static volatile unsigned int __attribute__ ((aligned(16))) gdt[] = {
    0x00000000,0x00000000, // Unused
    0x0000FFFF,0x00CF9A00, // Kernel code
    0x0000FFFF,0x00CF9200, // Kernel data
    0x00000000,0x00000000, // System TSS
};

static volatile int __attribute__ ((aligned(16))) idt [512];

#endif

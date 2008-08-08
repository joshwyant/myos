#ifndef __kernel_h__
#define __kernel_h__

#include "video.h"
#include "fat.h"
#include "disk.h"
#include "klib.h"

#ifndef XRES
#define XRES 1024
#define YRES 768
#endif

// externals
extern void irq0(); // kerboard isr
extern void irq1(); // kerboard isr
extern void int0(); // divide error isr
extern void int8(); // double fault
extern void intd(); // gpf
extern void inte(); // pgf
extern void int30(); // syscall
extern char kernel[];
extern char image_end[];
extern void* kernel_end;

// main
void kmain();
// initialization
void init_paging(); // nonstatic so compiler doesn't optimize it, and we can jump right over it in bochs
void init_heap();
void init_idt();
void init_exceptions();
void init_pic();
void kbd_init();
void init_timer();
void init_processes();
void init_tss();
void init_syscalls();
void start_shell();
// syscalls
void syscall(unsigned edi, unsigned esi, unsigned ebp, unsigned esp, unsigned ebx, unsigned edx, unsigned ecx, unsigned eax);
// exceptions
void divide_error(unsigned edi, unsigned esi, unsigned ebp, unsigned esp, unsigned ebx, unsigned edx, unsigned ecx, unsigned eax, unsigned eip, unsigned cs, unsigned eflags);
void gpfault(unsigned edi, unsigned esi, unsigned ebp, unsigned esp, unsigned ebx, unsigned edx, unsigned ecx, unsigned eax, unsigned errorcode, unsigned eip, unsigned cs, unsigned eflags);
void pgfault(unsigned edi, unsigned esi, unsigned ebp, unsigned esp, unsigned ebx, unsigned edx, unsigned ecx, unsigned eax, unsigned errorcode, unsigned eip, unsigned cs, unsigned eflags);
void dobule_fault(unsigned edi, unsigned esi, unsigned ebp, unsigned esp, unsigned ebx, unsigned edx, unsigned ecx, unsigned eax, unsigned errorcode, unsigned eip, unsigned cs, unsigned eflags);
static void dump_stack(const char*, unsigned, unsigned, unsigned, unsigned, unsigned, unsigned, unsigned, unsigned, unsigned, unsigned);
static void bsod(const char*);

// Memory
static inline void* palloc(int,void*,void*);
static inline void  mark_pages(void*, int, int);
static inline void  mark_page(void*, int);
static int ksbrk(int);

// Processes
typedef struct _ProcessNode
{
    Process                  *process;
    struct _ProcessNode      *prev;
    struct _ProcessNode      *next;
} ProcessNode;

typedef struct
{
    ProcessNode      *first;
    ProcessNode      *last;
} ProcessQueue;

ProcessQueue			processes;
unsigned			process_inc;

int		process_rotate();
void		process_node_unlink(ProcessNode* n);
void		process_node_link(ProcessNode* n);

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
static void* heap_start;
static void* heap_end;
static void* heap_brk;
static void* first_free;
static void* last_free;

// global descriptor table
static volatile unsigned int __attribute__ ((aligned(8))) gdt[] = {
    0x00000000,0x00000000, // 0x0000 Unused - null descriptor
    0x0000FFFF,0x00CF9A00, // 0x0008 DPL0 code - kernel
    0x0000FFFF,0x00CF9200, // 0x0010 DPL0 data - kernel
    0x0000FFFF,0x00CFBA00, // 0x0018 DPL1 code
    0x0000FFFF,0x00CFB200, // 0x0020 DPL1 data
    0x0000FFFF,0x00CFDA00, // 0x0028 DPL2 code
    0x0000FFFF,0x00CFD200, // 0x0030 DPL2 data
    0x0000FFFF,0x00CFFA00, // 0x0038 DPL3 code - user
    0x0000FFFF,0x00CFF200, // 0x0040 DPL3 data - user
    0x00000000,0x00000000, // 0x0048 System TSS (initialized later)
};

struct TSS
{
    unsigned short	backlink, __blh;
    unsigned int	esp0;
    unsigned short	ss0, __ss0h;
    unsigned int	esp1;
    unsigned short	ss1, __ss1h;
    unsigned int	esp2;
    unsigned short	ss2, __ss2h;
    unsigned int	cr3;
    unsigned int	eip;
    unsigned int	eflags;
    unsigned int	eax, ecx, edx, ebx;
    unsigned int	esp, ebp, esi, edi;
    unsigned short	es, __esh;
    unsigned short	cs, __csh;
    unsigned short	ss, __ssh;
    unsigned short	ds, __dsh;
    unsigned short	fs, __fsh;
    unsigned short	gs, __gsh;
    unsigned short	ldt, __ldth;
    unsigned short	trace, bitmap;
// Intel manual says, if using paging, avoid letting TSS cross a page
// boundary. The alignment is 128 here because it's the smallest number
// bigger than 104 (sizeof(TSS)) that the page size (4096 bytes) is divisible by.
} __attribute__ ((aligned(128))) system_tss;


// interrupt descriptor table
static volatile int __attribute__ ((aligned(8))) idt [512];


typedef struct {

	int	width;
	int	height;
	void*	bits;
	short   bpp;
} Bitmap;

typedef struct __attribute__ ((__packed__)) {
	unsigned int	bcSize;
	unsigned int	bcWidth;
	unsigned int	bcHeight;
	unsigned short	bcPlanes;
	unsigned short	bcBitCount;
} BITMAPCOREHEADER,*LPBITMAPCOREHEADER,*PBITMAPCOREHEADER;

typedef struct __attribute__ ((__packed__)) {
	unsigned short	bfType;
	unsigned int	bfSize;
	unsigned short	bfReserved1;
	unsigned short	bfReserved2;
	unsigned int	bfOffBits;
} BITMAPFILEHEADER,*LPBITMAPFILEHEADER,*PBITMAPFILEHEADER;

typedef struct {
    int Second;
    int Minute;
    int Hour;
    int Day;
    int Month;
    int Year;
} DateTime;

DateTime get_time();
// void print_datetime();
// int read_bitmap(Bitmap *b, char *filename);

// Takes a tag and tells if it's free
static inline int ktagisfree(unsigned tag)
{
    return (tag & 0x01000000) == 0;
}

// Uses the tag to tell if an address is at the beginning of the heap
static inline int ktagisbegin(unsigned tag)
{
    return (tag & 0x02000000) != 0;
}

// Tag is at the end of the heap
static inline int ktagisend(unsigned tag)
{
    return (tag & 0x04000000) != 0;
}

// The size of the block
static inline int ktagsize(unsigned tag)
{
    return (tag & 0x00FFFFFF) * 16;
}

// The size of the block, excluding the boundary tags
static inline int ktaginsize(unsigned tag)
{
    return (tag & 0x00FFFFFF) * 16 - 8;
}

// Returns the tag for the memory address
static inline unsigned ktag(void* ptr)
{
    return *((unsigned*)(ptr+(unsigned)(-4)));
}

// Sets the tags (tag must have the right length encoded)
static inline unsigned ksettag(void* ptr, unsigned tag)
{
    *((unsigned*)(ptr+(unsigned)(-4))) = tag;
    *((unsigned*)(ptr+ktaginsize(tag))) = tag;
}

// Makes a tag
static inline unsigned kmaketag(int blocksize, int used, int begin, int end)
{
    unsigned tag = (blocksize/16);
    if (used) tag |= 0x01000000;
    if (begin) tag |= 0x02000000;
    if (end) tag |= 0x04000000;
    return tag;
}

// The tag of the previous block
static inline unsigned ktagprev(void* ptr)
{
    return *((unsigned*)(ptr+(unsigned)(-8)));
}

// The tag of the next block
static inline unsigned ktagnext(void* ptr)
{
    return *((unsigned*)(ptr+ktagsize(ktag(ptr))));
}

// The pointer to the previous adjacent block
static inline void* kadjprev(void* ptr)
{
    return ptr+(unsigned)(-ktagsize(ktagprev(ptr)));
}

// The pointer to the next adjacent block
static inline void* kadjnext(void* ptr)
{
    return ptr+ktagsize(ktag(ptr));
}

// Returns the pointer previous to this free pointer 
static inline void* kprevfree(void* ptr)
{
    return ((void**)ptr)[0];
}

// returns the next free pointer
static inline void* knextfree(void* ptr)
{
    return ((void**)ptr)[1];
}

// mark the memory tag's used bit
static inline void kuseptr(void* ptr, int use)
{
    if (use)
        ksettag(ptr, ktag(ptr) | 0x01000000);
    else
        ksettag(ptr, ktag(ptr) & 0xFEFFFFFF);
}

static inline void kptrlink(void* ptr)
{
    ((void**)ptr)[0] = last_free;
    ((void**)ptr)[1] = 0;
    if (last_free)
        ((void**)last_free)[1] = ptr;
    last_free = ptr;
    if (!first_free)
        first_free = ptr;
}

static inline void kptrunlink(void* ptr)
{
    void* prev = ((void**)ptr)[0];
    void* next = ((void**)ptr)[1];

    if (prev)
        ((void**)prev)[1] = next;
    else
        first_free = next;

    if (next)
        ((void**)next)[0] = prev;
    else
        last_free = prev;
}

#endif

#ifndef __TASK_H__
#define __TASK_H__

extern void init_gdt();
extern void init_tss();

extern struct TSS
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
} __attribute__ ((aligned(128))) system_tss, *vm8086_tss;

// global descriptor table
extern volatile unsigned int __attribute__ ((aligned(8))) gdt[];

#endif  // __TASK_H__

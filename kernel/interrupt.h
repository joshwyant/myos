#ifndef __INTERRUPT_H__
#define __INTERRUPT_H__

#ifdef __cplusplus
extern "C" {
#endif

extern void init_pic();
extern void init_idt();

extern void register_isr(int num, int dpl, void* offset);
extern void register_trap(int num, int dpl, void* offset);
extern void register_task(int num, int dpl, unsigned short selector);

// interrupt descriptor table
extern volatile int __attribute__ ((aligned(8))) idt [];

#ifdef __cplusplus
}  // extern "C"
#endif

#endif  // __INTERRUPT_H__

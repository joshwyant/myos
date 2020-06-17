#ifndef __INTERRUPT_H__
#define __INTERRUPT_H__

#ifdef __cplusplus
extern "C" {
#endif

extern void init_pic();
extern void init_idt();

// interrupt descriptor table
extern volatile int __attribute__ ((aligned(8))) idt [];

#ifdef __cplusplus
}  // extern "C"
#endif

#endif  // __INTERRUPT_H__

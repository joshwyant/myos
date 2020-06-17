#ifndef __CLOCK_H__
#define __CLOCK_H__

#include "klib.h"

#ifdef __cplusplus
extern "C" {
#endif

extern void init_clock();
extern void irq8(); // system clock isr
DateTime get_time();


#ifdef __cplusplus
}  // extern "C"
#endif

#endif  // __CLOCK_H__

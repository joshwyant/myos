#ifndef __CLOCK_H__
#define __CLOCK_H__

#include "klib.h"

extern void init_clock();
extern void irq8(); // system clock isr
DateTime get_time();

#endif  // __CLOCK_H__

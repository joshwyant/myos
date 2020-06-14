#ifndef __TIMER_H__
#define __TIMER_H__

extern void init_timer();
extern void irq0(); // system timer isr

extern int pit_reload;
extern unsigned timer_seconds;
extern unsigned timer_fractions;

#endif  // __TIMER_H__

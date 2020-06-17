#ifndef __TIMER_H__
#define __TIMER_H__

#ifdef __cplusplus
extern "C" {
#endif

extern void init_timer();
extern void irq0(); // system timer isr

extern int pit_reload;
extern unsigned timer_seconds;
extern unsigned timer_fractions;

#ifdef __cplusplus
}  // extern "C"
#endif

#endif  // __TIMER_H__

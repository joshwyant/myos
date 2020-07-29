#ifndef __CLOCK_H__
#define __CLOCK_H__

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    int Second;
    int Minute;
    int Hour;
    int Day;
    int Month;
    int Year;
} DateTime;

extern void init_clock();
extern void irq8(); // system clock isr
DateTime get_time();

#ifdef __cplusplus
}  // extern "C"
#endif

#endif  // __CLOCK_H__

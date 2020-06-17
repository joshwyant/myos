#ifndef __KEYBOARD_H__
#define __KEYBOARD_H__

#ifdef __cplusplus
extern "C" {
#endif

extern void irq1(); // keyboard isr
extern void kbd_init();
char kbd_peekc();
char kbd_readc();
void kbd_read(char*, int);
void kbd_readln(char*, int);

// keyboard data
extern const char kbd_lowercase[];
extern const char kbd_uppercase[];
extern volatile int kbd_escaped;
extern volatile int kbd_shift;
extern volatile char kbd_buffer[];
extern volatile int kbd_count;

#ifdef __cplusplus
}  // extern "C"
#endif

#endif  // __KEYBOARD_H__

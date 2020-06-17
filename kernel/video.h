#ifndef __video_h__
#define __video_h__

#include "klib.h"

#ifdef __cplusplus
extern "C" {
#endif

void init_video();
void move_cursor(int);
void print_char(char);
void print(const char*);
void printlen(const char*, int);
void show_cursor(int);
void cls();
void printhexb(char x);
void printhexw(short x);
void printhexd(int x);
void printdec(int x);
void kprintdatetime(DateTime dt);
void kprintf(const char* format, ...);
void endl();
extern volatile char* videomem;
// void print_datetime();

#ifdef __cplusplus
}  // extern "C"
#endif

#endif

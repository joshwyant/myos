#ifndef __video_h__
#define __video_h__

void init_video();
void move_cursor(int);
void print_char(char);
void print(const char*);
void show_cursor(int);
void cls();
void printhexb(char x);
void printhexw(short x);
void printhexd(int x);
void printdec(int x);
void endl();
volatile char* videomem;

#endif

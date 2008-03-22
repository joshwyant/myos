#ifndef __video_h__
#define __video_h__

#include "io.h"
#include "ktypes.h"

unsigned short crtbaseio;
char cursor_shown;
unsigned short cursorpos;

void init_video();
inline void _print_char(char, unsigned char);
void print_char(char);
void print(const char*);
inline void update_cursor_index();
inline void show_cursor(bool);
void cls();
void printhexb(char x);

#endif

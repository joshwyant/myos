#ifndef __MOUSE_H__
#define __MOUSE_H__

#include "kernel.h"

typedef struct
{
	unsigned char flags;
	int delta_x;
	int delta_y;
} MOUSE_PACKET;
#define MPF_LEFT_BUTTON 	0x01
#define MPF_RIGHT_BUTTON 	0x02
#define MPF_MIDDLE_BUTTON 	0x04
#define MPF_ALWAYS_SET	 	0x08
#define MPF_X_NEGATIVE	 	0x10
#define MPF_Y_NEGATIVE	 	0x20
#define MPF_X_OVERFLOW 		0x40
#define MPF_Y_OVERFLOW 		0x80

inline static unsigned char mouse_read();
inline static void mouse_write(unsigned char a_write);
inline static int mouse_wait(unsigned char a_type);

extern void init_mouse();
extern unsigned char mouse_read();
extern void show_mouse_cursor(int bShow);
extern void irq12(); // mouse isr

extern MOUSE_PACKET mouse_packet;
extern int mouse_screen_x;
extern int mouse_screen_y;
extern int mouse_visible;
extern unsigned char mouse_erase_buffer[];
extern unsigned char mouse_cycle;

inline static unsigned char mouse_read()
{
  //Get's response from mouse
  mouse_wait(0);
  return inb(0x60);
}

inline static void mouse_write(unsigned char a_write)
{
  //Wait to be able to send a command
  mouse_wait(1);
  //Tell the mouse we are sending a command
  outb(0x64, 0xD4);
  //Wait for the final part
  mouse_wait(1);
  //Finally write
  outb(0x60, a_write);
}

inline static int mouse_wait(unsigned char a_type)
{
  io_wait();
  unsigned int _time_out=100000;
  if(a_type==0)
  {
    while(_time_out--) //Data
    {
      if((inb(0x64) & 1)==1)
      {
        return 1;
      }
    }
    return 0;
  }
  else
  {
    while(_time_out--) //Signal
    {
      if((inb(0x64) & 2)==0)
      {
        return 1;
      }
    }
    return 0;
  }
}

#endif  // __MOUSE_H__

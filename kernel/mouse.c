#include "kernel.h"

MOUSE_PACKET mouse_packet = {0, 0, 0};
int mouse_screen_x;
int mouse_screen_y;
int mouse_visible = 0;
unsigned char mouse_erase_buffer[768];
unsigned char mouse_cycle=0;

void init_mouse()
{
  unsigned char _status;  //unsigned char

  //Enable the auxiliary mouse device
  mouse_wait(1);
  outb(0x64, 0xA8);
 
  //Enable the interrupts
  mouse_wait(1);
  outb(0x64, 0x20);
  mouse_wait(0);
  _status=(inb(0x60) | 2);
  mouse_wait(1);
  outb(0x64, 0x60);
  mouse_wait(1);
  outb(0x60, _status);
 
  //Tell the mouse to use default settings
  mouse_write(0xF6);
  mouse_read();  //Acknowledge
 
  //Enable the mouse
  mouse_write(0xF4);
  mouse_read();  //Acknowledge
  
  mouse_screen_x = vesaMode.width / 2;
  mouse_screen_y = vesaMode.height / 2;
  show_mouse_cursor(1);

  //Setup the mouse handler
  register_isr(0x2c, 0, irq12);
  irq_unmask(12);
}

void draw_mouse_cursor(int x, int y)
{
	// TODO: Support cursor objects with bitmap and hotspot
	RECT r1 = {x, y - 7, x + 1, y + 8};
	RECT r2 = {x - 7, y, x + 8, y + 1};
	invert_rect(&r1);
	invert_rect(&r2);
}

//Mouse functions
void handle_mouse(void *a_r) //struct regs *a_r (not used but just there)
{
	unsigned char b = inb(0x60);
	switch(mouse_cycle)
	{
	case 0:
		if (b & MPF_ALWAYS_SET) // check alignment
		{
			mouse_packet.flags = b;
			mouse_cycle++;
		}
		break;
	case 1:
		mouse_packet.delta_x = b;
		if (mouse_packet.flags & MPF_X_NEGATIVE) mouse_packet.delta_x |= 0xFFFFFF00;
		if (mouse_packet.flags & MPF_X_OVERFLOW) mouse_packet.delta_x = mouse_packet.delta_x < 0 ? -256 : 255;
		mouse_cycle++;
		break;
	case 2:
		mouse_packet.delta_y = b;
		if (mouse_packet.flags & MPF_Y_NEGATIVE) mouse_packet.delta_y |= 0xFFFFFF00;
		if (mouse_packet.flags & MPF_Y_OVERFLOW) mouse_packet.delta_y = mouse_packet.delta_y < 0 ? -256 : 255;
		
		int bMouseVisible = mouse_visible;
		
		if (mouse_visible)
		{
			// Erase the mouse cursor from the screen (re-write the background).
			show_mouse_cursor(0);
		}

		mouse_screen_x += mouse_packet.delta_x;
		mouse_screen_y -= mouse_packet.delta_y;

		// Clip the mouse position
		if (mouse_screen_x < 0) mouse_screen_x = 0;
		if (mouse_screen_y < 0) mouse_screen_y = 0;
		if (mouse_screen_x >= vesaMode.width) mouse_screen_x = vesaMode.width - 1;
		if (mouse_screen_y >= vesaMode.height) mouse_screen_y = vesaMode.height - 1;
		
		if (bMouseVisible)
		{
			show_mouse_cursor(1);
		}

		mouse_cycle = 0;
		break;
	}
	eoi(12);
}

void show_mouse_cursor(int bShow)
{
	if ((bShow && mouse_visible) || !(bShow || mouse_visible)) return;
	
	// TODO: programmable with cursor object
	RECT mouse_rect = {mouse_screen_x - 8, mouse_screen_y - 8, mouse_screen_x + 8, mouse_screen_y + 8};
	
	if (bShow)
	{
		screen_to_buffer(&mouse_rect, mouse_erase_buffer);
		draw_mouse_cursor(mouse_screen_x, mouse_screen_y);
	}
	else
	{
		buffer_to_screen(mouse_erase_buffer, &mouse_rect);
	}
	mouse_visible = bShow;
}

#include <memory>
#include "kernel.h"

using namespace kernel;

void PS2MouseDriver::init()
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
  
  mouse_screen_x = graphics_driver->get_screen_context()->get_width() / 2;
  mouse_screen_y = graphics_driver->get_screen_context()->get_height() / 2;
  
  read_bitmap(fs_driver, &cursor, "/system/bin/cursor");

  mouse_erase_buffer = new unsigned char[cursor.width * cursor.height * 3];
}

void PS2MouseDriver::start()
{
  show_cursor(true);

  //Setup the mouse handler
  register_isr(0x2c, 0, (void*)irq12);
  irq_unmask(12);
}

void PS2MouseDriver::draw_mouse_cursor(int x, int y)
{
	//RECT r1 = {x, y - 7, x + 1, y + 8};
	//RECT r2 = {x - 7, y, x + 8, y + 1};
	//invert_rect(&r1);
	//invert_rect(&r2);
	
	draw_image(&cursor, x, y, 255);
}

void PS2MouseDriver::show_cursor(bool bShow)
{
	if ((bShow && mouse_visible) || !(bShow || mouse_visible)) return;
	
	// TODO: programmable with cursor object
	RECT mouse_rect = {mouse_screen_x, mouse_screen_y, mouse_screen_x + cursor.width, mouse_screen_y + cursor.height};
	
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

//Mouse functions
extern "C" void handle_mouse(void *a_r) //struct regs *a_r (not used but just there)
{
    ((kernel::PS2MouseDriver*)kernel::DriverManager::current()->mouse_driver().get())->mouse_handler();
}

void PS2MouseDriver::mouse_handler()
{
  	auto ctx = graphics_driver->get_screen_context();
	auto b = (MousePacketFlags)inb(0x60);
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
			show_cursor(false);
		}

		mouse_screen_x += mouse_packet.delta_x;
		mouse_screen_y -= mouse_packet.delta_y;

		// Clip the mouse position
		if (mouse_screen_x < 0) mouse_screen_x = 0;
		if (mouse_screen_y < 0) mouse_screen_y = 0;
		if (mouse_screen_x >= ctx->get_width()) mouse_screen_x = ctx->get_width() - 1;
		if (mouse_screen_y >= ctx->get_height()) mouse_screen_y = ctx->get_height() - 1;
		
		if (bMouseVisible)
		{
			show_cursor(true);
		}

		mouse_cycle = 0;
		break;
	}
	eoi(12);
}

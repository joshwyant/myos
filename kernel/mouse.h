#ifndef __MOUSE_H__
#define __MOUSE_H__

#include <stddef.h>
#include "fs.h"
#include "drawing.h"
#include "driver.h"
#include "string.h"

#ifdef __cplusplus
#include <memory>

extern "C"
{
    extern void irq12(); // mouse isr
    extern void handle_mouse(void *a_r);
}

namespace kernel
{    
enum MousePacketFlags
{
    MPF_LEFT_BUTTON 	= 0x01,
    MPF_RIGHT_BUTTON  =	0x02,
    MPF_MIDDLE_BUTTON =	0x04,
    MPF_ALWAYS_SET    = 0x08,
    MPF_X_NEGATIVE    = 0x10,
    MPF_Y_NEGATIVE    = 0x20,
    MPF_X_OVERFLOW    =	0x40,
    MPF_Y_OVERFLOW    =	0x80
};
    
class MouseDriver
  : public Driver
{
public:
    MouseDriver(KString device_name = "mouse")
        : Driver(device_name) {}
    virtual int screen_x() const = 0;
    virtual int screen_y() const = 0;
    virtual void show_cursor(bool bShow) = 0;
    virtual void mouse_handler() = 0;
    virtual void start() = 0;
}; // class MouseDriver

class PS2MouseDriver
    : public MouseDriver
{
public:
    PS2MouseDriver(
      std::shared_ptr<GraphicsDriver> graphics_driver,
      std::shared_ptr<FileSystem> fs,
      KString device_name = "mouse"
    ) : graphics_driver(graphics_driver),
        fs(fs),
        mouse_packet({0, 0, 0}),
        mouse_screen_x(0),
        mouse_screen_y(0),
        mouse_visible(0),
        mouse_erase_buffer(nullptr),
        mouse_cycle(0),
        MouseDriver(device_name)
    {
        init();
    }
    PS2MouseDriver(PS2MouseDriver&) = delete;
    PS2MouseDriver(PS2MouseDriver&&) = delete;
    PS2MouseDriver& operator=(PS2MouseDriver) = delete;
    int screen_x() const override { return mouse_screen_x; }
    int screen_y() const override { return mouse_screen_y; }
    void show_cursor(bool bShow) override;
    void mouse_handler() override;
    void start() override;
    virtual ~PS2MouseDriver()
    {
        delete[] mouse_erase_buffer;
    }
private:
    std::shared_ptr<GraphicsDriver> graphics_driver;
    std::shared_ptr<FileSystem> fs;

    int mouse_screen_x;
    int mouse_screen_y;
    int mouse_visible;
    unsigned char *mouse_erase_buffer;
    unsigned char mouse_cycle = 0;

    Bitmap cursor;

    struct MOUSE_PACKET
    {
      unsigned char flags;
      int delta_x;
      int delta_y;
    } mouse_packet = {0, 0, 0};

    friend void handle_mouse(void *a_r);


    void init();

    void draw_mouse_cursor(int x, int y);

    inline unsigned char mouse_read()
    {
      //Get's response from mouse
      mouse_wait(0);
      return inb(0x60);
    }

    inline void mouse_write(unsigned char a_write)
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

    inline bool mouse_wait(unsigned char a_type)
    {
      io_wait();
      unsigned int _time_out=100000;
      if(a_type==0)
      {
        while(_time_out--) //Data
        {
          if((inb(0x64) & 1)==1)
          {
            return true;
          }
        }
        return false;
      }
      else
      {
        while(_time_out--) //Signal
        {
          if((inb(0x64) & 2)==0)
          {
            return true;
          }
        }
        return false;
      }
    }
}; // class PS2MouseDriver
} // namespace kernel

#endif // __cplusplus
#endif  // __MOUSE_H__

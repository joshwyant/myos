#ifndef __KEYBOARD_H__
#define __KEYBOARD_H__

#include <stddef.h>
#include "interrupt.h"
#include "driver.h"

#ifdef __cplusplus
#include <memory>

extern "C" {
#endif

extern void irq1(); // keyboard isr

#ifdef __cplusplus
}  // extern "C"
namespace kernel
{
class Keymap
{
public:
    virtual char lowercase(int keycode) const = 0;
    virtual char uppercase(int keycode) const = 0;
}; // class Keymap

class AmericanKeymap
    : public Keymap
{
public:
    char lowercase(int keycode) const override { return _lowercase[keycode]; }
    char uppercase(int keycode) const override { return _uppercase[keycode]; }
private:
    static const char _lowercase[];
    static const char _uppercase[];
}; // class AmericanKeymap

class KeyboardDriver
    : public Driver
{
public:
    KeyboardDriver(KString device_name = "kbd")
        : Driver(device_name) {}
    virtual void keyboard_handler() = 0;
    virtual void start() = 0;
    virtual char peekc() = 0;
    virtual char readc() = 0;
    virtual void read(char*, int) = 0;
    virtual void readln(char*, int) = 0;
}; // class KeyboardDriver

class PS2KeyboardDriver
    : public KeyboardDriver
{
public:
    PS2KeyboardDriver(KString device_name = "kbd")
        : keymap(std::make_unique<AmericanKeymap>()),
          kbd_escaped(0),
          kbd_shift(0),
          kbd_count(0),
          KeyboardDriver(device_name)
    {
        // fill descriptor 0x21 (irq 1) for keyboard handler
        register_isr(0x21,0,(void*)irq1);
    }
    void keyboard_handler() override;
    void start() override;
    char peekc() override;
    char readc() override;
    void read(char*, int) override;
    void readln(char*, int) override;
    virtual ~PS2KeyboardDriver() = default;
private:
    std::unique_ptr<Keymap> keymap;
    volatile bool kbd_escaped;
    volatile bool kbd_shift;
    volatile char kbd_buffer[32];
    volatile int kbd_count;
}; // class PS2KeyboardDriver
} // namespace kernel
#endif // __cplusplus

#endif  // __KEYBOARD_H__

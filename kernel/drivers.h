#ifndef __KERNEL_DRIVERS_H__
#define __KERNEL_DRIVERS_H__

#include "video.h"
#include "disk.h"
#include "driver.h"
#include "fs.h"
#include "keyboard.h"
#include "map.h"
#include "mouse.h"
#include "string.h"
#include "timer.h"

#ifdef __cplusplus
#include <memory>

namespace kernel
{
class DriverManager
{
public:
    std::shared_ptr<ConsoleDriver>
        register_console_driver(std::shared_ptr<ConsoleDriver> driver) { return register_device(_console_driver = driver); }

    std::shared_ptr<ConsoleDriver>
        register_text_console_driver(std::shared_ptr<ConsoleDriver> driver) { return register_device(_text_console_driver = driver); }

    std::shared_ptr<DiskDriver>
        register_disk_driver(std::shared_ptr<DiskDriver> driver) { return register_device(_disk_driver = driver); }

    std::shared_ptr<FileSystemDriver>
        register_file_system_driver(std::shared_ptr<FileSystemDriver> driver) { return register_device(_fs_driver = driver); }

    std::shared_ptr<GraphicsDriver>
        register_graphics_driver(std::shared_ptr<GraphicsDriver> driver) { return register_device(_graphics_driver = driver); }

    std::shared_ptr<MouseDriver>
        register_mouse_driver(std::shared_ptr<MouseDriver> driver) { return register_device(_mouse_driver = driver); }

    std::shared_ptr<KeyboardDriver>
        register_keyboard_driver(std::shared_ptr<KeyboardDriver> driver) { return register_device(_keyboard_driver = driver); }

    std::shared_ptr<TimerDriver>
        register_timer_driver(std::shared_ptr<TimerDriver> driver) { return register_device(_timer_driver = driver); }

    std::shared_ptr<ConsoleDriver> text_console_driver() { return _text_console_driver; }
    std::shared_ptr<ConsoleDriver> console_driver() { return _console_driver; }
    std::shared_ptr<DiskDriver> disk_driver() { return _disk_driver; }
    std::shared_ptr<FileSystemDriver> file_system_driver() { return _fs_driver; }
    std::shared_ptr<GraphicsDriver> graphics_driver() { return _graphics_driver; }
    std::shared_ptr<MouseDriver> mouse_driver() { return _mouse_driver; }
    std::shared_ptr<KeyboardDriver> keyboard_driver() { return _keyboard_driver; }
    std::shared_ptr<TimerDriver> timer_driver() { return _timer_driver; }

    template <typename TDriver = Driver>
    std::shared_ptr<TDriver> get(const KString& key)
    {
        return const_cast<std::shared_ptr<TDriver>&>(static_cast<const DriverManager&>(*this).get(key));
    }

    template <typename TDriver = Driver>
    const std::shared_ptr<TDriver> get(const KString& key) const
    {
        return _device_map[key];
    }

private:
    template <typename TDriver>
    std::shared_ptr<TDriver>
        register_device(std::shared_ptr<TDriver> driver)
    {
        return std::dynamic_pointer_cast<TDriver>(_device_map.set(driver->name(), driver).value);
    }

    std::shared_ptr<ConsoleDriver> _text_console_driver;
    std::shared_ptr<ConsoleDriver> _console_driver;
    std::shared_ptr<DiskDriver> _disk_driver;
    std::shared_ptr<FileSystemDriver> _fs_driver;
    std::shared_ptr<GraphicsDriver> _graphics_driver;
    std::shared_ptr<MouseDriver> _mouse_driver;
    std::shared_ptr<KeyboardDriver> _keyboard_driver;
    std::shared_ptr<TimerDriver> _timer_driver;
    UnorderedMap<KString, std::shared_ptr<Driver> > _device_map;
};
} // namespace kernel
#endif // __cplusplus
#endif // __KERNEL_DRIVERS_H__

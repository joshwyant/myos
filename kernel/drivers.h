#ifndef __KERNEL_DRIVERS_H__
#define __KERNEL_DRIVERS_H__

#ifdef __cplusplus
#include <memory>
#include "kernel.h"
#include "video.h"
#include "disk.h"
#include "fs.h"
#include "keyboard.h"
#include "mouse.h"
namespace kernel
{
class DriverManager
{
public:
    static std::shared_ptr<DriverManager>& set_current(std::shared_ptr<DriverManager> manager) { return _current = manager; }
    static std::shared_ptr<DriverManager>& set_root(std::shared_ptr<DriverManager> manager) { return _root = manager; }
    static std::shared_ptr<DriverManager> current() { return _current; }
    static std::shared_ptr<DriverManager> root() { return _root; }
    static std::shared_ptr<DriverManager>& init()
    {
        return set_current(set_root(std::make_shared<DriverManager>()));
    }

    std::shared_ptr<ConsoleDriver>&
        register_console_driver(std::shared_ptr<ConsoleDriver> driver) { return _console_driver = driver; }

    std::shared_ptr<ConsoleDriver>&
        register_text_console_driver(std::shared_ptr<ConsoleDriver> driver) { return _text_console_driver = driver; }

    std::shared_ptr<DiskDriver>&
        register_disk_driver(std::shared_ptr<DiskDriver> driver) { return _disk_driver = driver; }

    std::shared_ptr<FileSystemDriver>&
        register_file_system_driver(std::shared_ptr<FileSystemDriver> driver) { return _fs_driver = driver; }

    std::shared_ptr<GraphicsDriver>&
        register_graphics_driver(std::shared_ptr<GraphicsDriver> driver) { return _graphics_driver = driver; }

    std::shared_ptr<MouseDriver>&
        register_mouse_driver(std::shared_ptr<MouseDriver> driver) { return _mouse_driver = driver; }

    std::shared_ptr<KeyboardDriver>&
        register_keyboard_driver(std::shared_ptr<KeyboardDriver> driver) { return _keyboard_driver = driver; }

    std::shared_ptr<ConsoleDriver> text_console_driver() { return _text_console_driver; }
    std::shared_ptr<ConsoleDriver> console_driver() { return _console_driver; }
    std::shared_ptr<DiskDriver> disk_driver() { return _disk_driver; }
    std::shared_ptr<FileSystemDriver> file_system_driver() { return _fs_driver; }
    std::shared_ptr<GraphicsDriver> graphics_driver() { return _graphics_driver; }
    std::shared_ptr<MouseDriver> mouse_driver() { return _mouse_driver; }
    std::shared_ptr<KeyboardDriver> keyboard_driver() { return _keyboard_driver; }

private:
    std::shared_ptr<ConsoleDriver> _text_console_driver;
    std::shared_ptr<ConsoleDriver> _console_driver;
    std::shared_ptr<DiskDriver> _disk_driver;
    std::shared_ptr<FileSystemDriver> _fs_driver;
    std::shared_ptr<GraphicsDriver> _graphics_driver;
    std::shared_ptr<MouseDriver> _mouse_driver;
    std::shared_ptr<KeyboardDriver> _keyboard_driver;
    static std::shared_ptr<DriverManager> _root;
    static std::shared_ptr<DriverManager> _current;
};
} // namespace kernel
#endif // __cplusplus
#endif // __KERNEL_DRIVERS_H__

#ifndef __KERNEL_H__
#define __KERNEL_H__

#include "../include/loader_info.h"
#include "drivers.h"
#include "elf.h"
#include "fs.h"
#include "video.h"

#ifdef __cplusplus
#include <memory>

extern "C" {
#endif

// main
extern loader_info loaderInfo;
extern void init_loader_info();

#ifdef __cplusplus
}  // extern "C"

// initialization
extern void show_splash(
    std::shared_ptr<kernel::GraphicsDriver> graphics_driver,
    std::shared_ptr<kernel::FileSystemDriver> fs_driver);
extern void start_shell(std::shared_ptr<kernel::FileSystemDriver> fs_driver);

namespace kernel
{
class KernelInterface
{
public:
    static std::shared_ptr<KernelInterface> current() { return _kernel; }
    static std::shared_ptr<KernelInterface>
        set_current(std::shared_ptr<KernelInterface> kernel)
        {
            return _kernel = kernel;
        }
    virtual std::shared_ptr<SymbolManager> symbols() const = 0;
    virtual std::shared_ptr<DriverManager> drivers() const = 0;
    virtual std::shared_ptr<DriverManager> root_drivers() const = 0;
private:
    static std::shared_ptr<KernelInterface> _kernel;
}; // class KernelInterface

class Kernel
    : public KernelInterface
{
public:
    Kernel();
    std::shared_ptr<SymbolManager> symbols() const override { return _symbols; }
    std::shared_ptr<DriverManager> drivers() const override { return _current_drivers; }
    std::shared_ptr<DriverManager> root_drivers() const override { return _root_drivers; }
    static auto init()
    {
        return Kernel::set_current(std::make_shared<Kernel>());  // Persist in memory
    }
private:
    std::shared_ptr<SymbolManager> _symbols;
    std::shared_ptr<DriverManager> _root_drivers;
    std::shared_ptr<DriverManager> _current_drivers;
}; // class Kernel
} // namespace kernel
#endif // __cplusplus

#endif  // __KERNEL_H__

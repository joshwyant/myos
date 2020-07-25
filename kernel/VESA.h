#ifndef __KERNEL_VESA_H__
#define __KERNEL_VESA_H__

#include "../include/VESA.h"
#include "video.h"
#include "kernel.h"

#ifdef __cplusplus
#include <memory>
#include "fs.h"

namespace kernel
{
class VESAGraphicsDriver 
    : public GraphicsDriver
{
public:
    VESAGraphicsDriver(std::shared_ptr<FileSystemDriver> fs_driver)
        : fs_driver(fs_driver), GraphicsDriver()
    {
        init();
    }
    virtual ~VESAGraphicsDriver()
    {
        delete buffered_screen_context;
        delete raw_screen_context;
    }
    BufferedGraphicsContext *get_screen_context() override
    {
        return buffered_screen_context;
    }
protected:
    vbe_mode_info vesaMode;
    BufferedMemoryGraphicsContext *buffered_screen_context;
    MemoryGraphicsContext *raw_screen_context;
    std::shared_ptr<FileSystemDriver> fs_driver;
private:
    void init();
}; // class VESAGraphicsDriver
} // namespace kernel
    
#endif  // __cplusplus
#endif  // __KERNEL_VESA_H__

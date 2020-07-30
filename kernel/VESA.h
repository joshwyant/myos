#ifndef __KERNEL_VESA_H__
#define __KERNEL_VESA_H__

#include "../include/VESA.h"
#include "drawing.h"
#include "fs.h"
#include "string.h"

#ifdef __cplusplus
#include <memory>

namespace kernel
{
class VESAGraphicsDriver 
    : public GraphicsDriver
{
public:
    VESAGraphicsDriver(std::shared_ptr<FileSystem> fs, KString device_name = "vesa")
        : fs(fs),
          GraphicsDriver(device_name)
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
    std::shared_ptr<FileSystem> fs;
    std::unique_ptr<MappedMemory<unsigned char> > _frame_buffer;
private:
    void init();
}; // class VESAGraphicsDriver
} // namespace kernel
    
#endif  // __cplusplus
#endif  // __KERNEL_VESA_H__

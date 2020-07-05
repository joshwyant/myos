#ifndef __KERNEL_VESA_H__
#define __KERNEL_VESA_H__

#include "../include/VESA.h"
#include "video.h"
#include "kernel.h"

#ifdef __cplusplus
extern "C" {
#endif

void init_vesa();

#ifdef __cplusplus
}  // extern "C"

namespace kernel
{
class VESAGraphicsDriver 
    : public GraphicsDriver
{
public:
    VESAGraphicsDriver();
    ~VESAGraphicsDriver()
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
}; // class VESAGraphicsDriver
} // namespace kernel
#endif  // __cplusplus
#endif  // __KERNEL_VESA_H__

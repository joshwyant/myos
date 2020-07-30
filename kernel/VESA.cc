#include <memory>
#include "VESA.h"
#include "kernel.h"
#include "memory.h"

void kernel::VESAGraphicsDriver::init()
{
    unsigned char *framebuffer_orig;
	// Map in and copy the info struct
    {   MappedMemory<vbe_mode_info> vesa_orig((void*)loaderInfo.vbe, PF_NONE);
        framebuffer_orig = vesa_orig.get()->framebuffer;
        vesaMode = *vesa_orig.get();
    } // vesa_orig
    page_free((void*)loaderInfo.vbe,1);
	
	// Map in the frame buffer
    int xres, yres, pixelWidth;
    xres = vesaMode.width;
    yres = vesaMode.height;
    pixelWidth = vesaMode.bpp/8;
    
    _frame_buffer = std::make_unique<MappedMemory<unsigned char> >(
            framebuffer_orig,
            PF_WRITE,
            xres*yres*pixelWidth);
    
    // Create the graphics context
    raw_screen_context = new MemoryGraphicsContext(fs, _frame_buffer.get()->get(), vesaMode.bpp, xres, yres);
    buffered_screen_context = new BufferedMemoryGraphicsContext(raw_screen_context);
}

#include <memory>
#include "kernel.h"
#include "VESA.h"

void kernel::VESAGraphicsDriver::init()
{
	// Map in and copy the info struct
    vbe_mode_info* vesa_orig = (vbe_mode_info*)kfindrange(4096);
    page_map((void*)vesa_orig, (void*)loaderInfo.vbe, PF_NONE);
    unsigned char *framebuffer_orig = vesa_orig->framebuffer;
	vesaMode = *vesa_orig;
    page_unmap(vesa_orig);
    page_free((void*)loaderInfo.vbe,1);
	
	// Map in the frame buffer
    int xres, yres, pixelWidth;
    xres = vesaMode.width;
    yres = vesaMode.height;
    pixelWidth = vesaMode.bpp/8;
    unsigned char *frameBuffer = (unsigned char *)kfindrange(xres*yres*3);
    for (int i = 0; i < xres*yres*pixelWidth+4095; i += 4096)
        page_map(frameBuffer+i,framebuffer_orig+i, PF_WRITE);
    
    // Create the graphics context
    raw_screen_context = new MemoryGraphicsContext(fs_driver, frameBuffer, vesaMode.bpp, xres, yres);
    buffered_screen_context = new BufferedMemoryGraphicsContext(raw_screen_context);
}

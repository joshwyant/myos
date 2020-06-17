#include "kernel.h"

vbe_mode_info vesaMode;
unsigned char *frameBuffer;

void init_vesa()
{
	// Map in and copy the info struct
    vbe_mode_info* vesa_orig = kfindrange(4096);
    page_map((void*)vesa_orig, (void*)loaderInfo.vbe, PF_NONE);
    unsigned char *framebuffer_orig = vesa_orig->framebuffer;
	vesaMode = *vesa_orig;
    page_unmap(vesa_orig);
    page_free((void*)loaderInfo.vbe,1);
	
	// Map in the frame buffer
    int xres, yres, bpp;
    xres = vesaMode.width;
    yres = vesaMode.height;
    bpp = vesaMode.bpp/8;
    frameBuffer = kfindrange(xres*yres*3);
    int i;
    for (i = 0; i < xres*yres*bpp+4095; i += 4096)
        page_map(frameBuffer+i,framebuffer_orig+i, PF_WRITE);
}

#ifndef __KERNEL_VESA_H__
#define __KERNEL_VESA_H__

#include "../include/VESA.h"

extern vbe_mode_info vesaMode;
extern unsigned char *frameBuffer;

void init_vesa();

#endif  // __KERNEL_VESA_H__

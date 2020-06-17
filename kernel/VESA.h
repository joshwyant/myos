#ifndef __KERNEL_VESA_H__
#define __KERNEL_VESA_H__

#include "../include/VESA.h"

#ifdef __cplusplus
extern "C" {
#endif

extern vbe_mode_info vesaMode;
extern unsigned char *frameBuffer;

void init_vesa();

#ifdef __cplusplus
}  // extern "C"
#endif

#endif  // __KERNEL_VESA_H__

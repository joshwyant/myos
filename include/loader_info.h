#ifndef __LOADER_INFO_H__
#define __LOADER_INFO_H__

#include "VESA.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    void	*memmap;	// Address of system memory map
    void	*freemem;	// Start of unused pages
    void	*loaded;	// Kernel virtual base address
    unsigned	memsize;	// Total bytes occupied by loaded kernel
	vbe_mode_info	*vbe;
} loader_info;

#ifdef __cplusplus
}  // extern "C"
#endif

#endif /* __LOADER_INFO_H__ */

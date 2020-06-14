#ifndef __VESA_H__
#define __VESA_H__

// Based on https://wiki.osdev.org/User:Omarrx024/VESA_Tutorial

struct vbe_mode_info_structure {
	unsigned short attributes;		// deprecated, only bit 7 should be of interest to you, and it indicates the mode supports a linear frame buffer.
	unsigned char window_a;			// deprecated
	unsigned char window_b;			// deprecated
	unsigned short granularity;		// deprecated; used while calculating bank numbers
	unsigned short window_size;
	unsigned short segment_a;
	unsigned short segment_b;
	unsigned long win_func_ptr;		// deprecated; used to switch banks from protected mode without returning to real mode
	unsigned short pitch;			// number of bytes per horizontal line
	unsigned short width;			// width in pixels
	unsigned short height;			// height in pixels
	unsigned char w_char;			// unused...
	unsigned char y_char;			// ...
	unsigned char planes;
	unsigned char bpp;			// bits per pixel in this mode
	unsigned char banks;			// deprecated; total number of banks in this mode
	unsigned char memory_model;
	unsigned char bank_size;		// deprecated; size of a bank, almost always 64 KB but may be 16 KB...
	unsigned char image_pages;
	unsigned char reserved0;
 
	unsigned char red_mask;
	unsigned char red_position;
	unsigned char green_mask;
	unsigned char green_position;
	unsigned char blue_mask;
	unsigned char blue_position;
	unsigned char reserved_mask;
	unsigned char reserved_position;
	unsigned char direct_color_attributes;
 
	unsigned char *framebuffer;		// physical address of the linear frame buffer; write here to draw to the screen
	unsigned long off_screen_mem_off;
	unsigned short off_screen_mem_size;	// size of memory in the framebuffer but not being displayed on the screen
	unsigned char reserved1[206];
} __attribute__ ((packed));
typedef struct vbe_mode_info_structure vbe_mode_info;

struct vbe_info_structure {
	char signature[4];	// must be "VESA" to indicate valid VBE support
	unsigned short version;			// VBE version; high byte is major version, low byte is minor version
	unsigned long oem;			// segment:offset pointer to OEM
	unsigned long capabilities;		// bitfield that describes card capabilities
	unsigned long video_modes;		// segment:offset pointer to list of supported video modes
	unsigned short video_memory;		// amount of video memory in 64KB blocks
	unsigned short software_rev;		// software revision
	unsigned long vendor;			// segment:offset to card vendor string
	unsigned long product_name;		// segment:offset to card model name
	unsigned long product_rev;		// segment:offset pointer to product revision
	char reserved[222];		// reserved for future expansion
	char oem_data[256];		// OEM BIOSes store their strings in this area
} __attribute__ ((packed));
typedef struct vbe_info_structure vbe_info;

#endif /* __VESA_H__ */

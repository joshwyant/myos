#ifndef __DRAWING_H__
#define __DRAWING_H__

#include "VESA.h"

#define RGB(r, g, b) ((r) | (g) << 8 | (b) << 16)
#define RGBA(r, g, b, a) ((r) | (g) << 8 | (b) << 16 | (a) << 24)
#define GET_R(rgb) ((rgb) & 0xFF)
#define GET_G(rgb) (((rgb) >> 8) & 0xFF)
#define GET_B(rgb) (((rgb) >> 16) & 0xFF)
#define GET_A(rgba) (((unsigned)(rgba) >> 24) & 0xFF)

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
	int x1;
	int y1;
	int x2;
	int y2;
} RECT;

typedef struct {

	int	width;
	int	height;
	void*	bits;
	short   bpp;
} Bitmap;

typedef struct __attribute__ ((__packed__)) {
	unsigned int	bcSize;
	unsigned int	bcWidth;
	unsigned int	bcHeight;
	unsigned short	bcPlanes;
	unsigned short	bcBitCount;
} BITMAPCOREHEADER,*LPBITMAPCOREHEADER,*PBITMAPCOREHEADER;

typedef struct __attribute__ ((__packed__)) {
	unsigned short	bfType;
	unsigned int	bfSize;
	unsigned short	bfReserved1;
	unsigned short	bfReserved2;
	unsigned int	bfOffBits;
} BITMAPFILEHEADER,*LPBITMAPFILEHEADER,*PBITMAPFILEHEADER;

inline static void swap_int(int *a, int *b);
inline static void rect_normalize(RECT *r);
inline static void rect_clip(RECT *r, RECT clip);
inline static void set_pixel(unsigned char *pixel, int c);
inline static void set_pixel_opacity(unsigned char *pixel, int c, int opacity);
inline static int get_pixel(unsigned char *pixel);
inline static void clip_to_screen(RECT *draw_r);
inline static void get_screen_metrics(int *stride, int *pixelWidth);
inline static void set_screen_pixel(int x, int y, int c);
inline static int get_screen_pixel(int x, int y);

void clear_color(int c);
void bitblt(Bitmap *bmp, int x, int y);
void draw_image(Bitmap *bmp, int x, int y, int opacity);
void draw_image_bgra(Bitmap *bmp, int x, int y, int opacity);
void draw_image_ext(Bitmap *bmp, RECT *src, RECT *dest, int opacity, int c);
void rect(RECT *r, char bSolid, int iborder, int c, int cborder, int opacity);
void invert_rect(RECT *r);
void screen_to_buffer(RECT *r, unsigned char *buffer);
void buffer_to_screen(unsigned char *buffer, RECT *r);
int read_bitmap(Bitmap *b, const char *filename);
void draw_text(const char *str, int x, int y, int c, int opacity, int xsize, int ysize);

inline static void swap_int(int *a, int *b)
{
	int tmp = *a;
	*a = *b;
	*b = tmp;
}

inline static void rect_normalize(RECT *r)
{
	if (r->x1 > r->x2)
	{
		swap_int(&r->x1, &r->x2);
	}
	if (r->y1 > r->y2)
	{
		swap_int(&r->y1, &r->y2);
	}
}

inline static void rect_clip(RECT *r, RECT clip)
{
	rect_normalize(r);
	if (r->x1 < clip.x1) r->x1 = clip.x1;
	if (r->y1 < clip.y1) r->y1 = clip.y1;
	if (r->x2 < clip.x1) r->x2 = clip.x1;
	if (r->y2 < clip.y1) r->y2 = clip.y1;
	if (r->x1 > clip.x2) r->x1 = clip.x2;
	if (r->y1 > clip.y2) r->y1 = clip.y2;
	if (r->x2 > clip.x2) r->x2 = clip.x2;
	if (r->y2 > clip.y2) r->y2 = clip.y2;
}

inline static unsigned char * get_screen_pixel_address(int x, int y, int stride, int pixelWidth)
{
	return frameBuffer + y * stride + x * pixelWidth;
}

inline static unsigned char * get_bitmap_pixel_address(Bitmap *bmp, int x, int y, int stride, int pixelWidth)
{
	return  (unsigned char *)bmp->bits
			+ bmp->width * bmp->height * pixelWidth 
			- stride * (y + 1)
			+ x * pixelWidth;
}

inline static void set_pixel(unsigned char *pixel, int c)
{	
	pixel[0] = (unsigned char)GET_B(c);
	pixel[1] = (unsigned char)GET_G(c);
	pixel[2] = (unsigned char)GET_R(c);
}

inline static void set_pixel_opacity(unsigned char *pixel, int c, int opacity)
{
	int b = pixel[0];
	int g = pixel[1];
	int r = pixel[2];
	b = b * 255 + ((int)GET_B(c) - b) * opacity;
	g = g * 255 + ((int)GET_G(c) - g) * opacity;
	r = r * 255 + ((int)GET_R(c) - r) * opacity;
	pixel[0] = (unsigned char)(b >> 8);
	pixel[1] = (unsigned char)(g >> 8);
	pixel[2] = (unsigned char)(r >> 8);
}

inline static int get_pixel(unsigned char *pixel)
{
	return RGB(pixel[2], pixel[1], pixel[0]);
}

inline static void get_screen_metrics(int *stride, int *pixelWidth)
{
	*pixelWidth = vesaMode.bpp >> 3;
	*stride = vesaMode.width**pixelWidth;
}

inline static void get_bitmap_metrics(Bitmap *bmp, int *stride, int *pixelWidth)
{
	*pixelWidth = bmp->bpp >> 3;
	*stride = bmp->width**pixelWidth;
}

inline static void clip_to_screen(RECT *draw_r)
{
	RECT screen_clip = {0, 0, vesaMode.width, vesaMode.height};
	rect_clip(draw_r, screen_clip);
}

inline static void set_screen_pixel(int x, int y, int c)
{
	int stride, pixelWidth;
	get_screen_metrics(&stride, &pixelWidth);
	return set_pixel(get_screen_pixel_address(x, y, stride, pixelWidth), c);
}

inline static int get_screen_pixel(int x, int y)
{
	int stride, pixelWidth;
	get_screen_metrics(&stride, &pixelWidth);
	return get_pixel(get_screen_pixel_address(x, y, stride, pixelWidth));
}

#ifdef __cplusplus
}  // extern "C"
#endif

#endif  // __DRAWING_H__

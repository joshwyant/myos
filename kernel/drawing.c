#include "kernel.h"

void clear_color(int c)
{
	int stride, pixelWidth;
	get_screen_metrics(&stride, &pixelWidth);
	
	unsigned char *end = frameBuffer + stride * vesaMode.height;
	for (unsigned char *pixel = frameBuffer; pixel != end; pixel += pixelWidth)
	{
		set_pixel(pixel, c);
	}
}

void bitblt(Bitmap *bmp, int x, int y)
{
	unsigned char *bitmap_pixel, *screen_pixel, *end_screen_pixel;
	int stride, pixelWidth, bmpStride, bmpPixelWidth;
	get_screen_metrics(&stride, &pixelWidth);
	get_bitmap_metrics(bmp, &bmpStride, &bmpPixelWidth);
	
	RECT full_r = {x, y, x + bmp->width, y + bmp->width};
	RECT draw_r = full_r;
	clip_to_screen(&draw_r);
	
	for (y = draw_r.y1; y < draw_r.y2; y++)
	{
		screen_pixel = get_screen_pixel_address(draw_r.x1, y, stride, pixelWidth);
		bitmap_pixel = get_bitmap_pixel_address(bmp, draw_r.x1 - full_r.x1, y - full_r.y1, bmpStride, bmpPixelWidth);
		end_screen_pixel = screen_pixel + stride;
		for (; screen_pixel < end_screen_pixel; screen_pixel += pixelWidth, bitmap_pixel += bmpPixelWidth)
		{
			screen_pixel[0] = bitmap_pixel[0];
			screen_pixel[1] = bitmap_pixel[1];
			screen_pixel[2] = bitmap_pixel[2];
		}
	}
}

void draw_image(Bitmap *bmp, int x, int y, int opacity)
{
	if (opacity == 0) return;
	if (opacity == 255)
	{
		bitblt(bmp, x, y);
		return;
	}
	unsigned char *bitmap_pixel, *screen_pixel, *end_screen_pixel;
	int stride, pixelWidth, bmpStride, bmpPixelWidth;
	get_screen_metrics(&stride, &pixelWidth);
	get_bitmap_metrics(bmp, &bmpStride, &bmpPixelWidth);
	
	RECT full_r = {x, y, x + bmp->width, y + bmp->width};
	RECT draw_r = full_r;
	clip_to_screen(&draw_r);
	
	for (y = draw_r.y1; y < draw_r.y2; y++)
	{
		screen_pixel = get_screen_pixel_address(draw_r.x1, y, stride, pixelWidth);
		bitmap_pixel = get_bitmap_pixel_address(bmp, draw_r.x1 - full_r.x1, y - full_r.y1, bmpStride, bmpPixelWidth);
		end_screen_pixel = screen_pixel + stride;
		for (; screen_pixel < end_screen_pixel; screen_pixel += pixelWidth, bitmap_pixel += bmpPixelWidth)
		{
			int b = screen_pixel[0];
			int g = screen_pixel[1];
			int r = screen_pixel[2];
			b = b * 255 + ((int)bitmap_pixel[0] - b) * opacity;
			g = g * 255 + ((int)bitmap_pixel[1] - g) * opacity;
			r = r * 255 + ((int)bitmap_pixel[2] - r) * opacity;
			screen_pixel[0] = (unsigned char)(b >> 8);
			screen_pixel[1] = (unsigned char)(g >> 8);
			screen_pixel[2] = (unsigned char)(r >> 8);
		}
	}
}

void draw_image_bgra(Bitmap *bmp, int x, int y, int opacity)
{
	if (bmp->bpp <= 24)
	{
		draw_image(bmp, x, y, opacity);
		return;
	}
	if (opacity == 0) return;
	unsigned char *bitmap_pixel, *screen_pixel, *end_screen_pixel;
	int stride, pixelWidth, bmpStride, bmpPixelWidth;
	get_screen_metrics(&stride, &pixelWidth);
	get_bitmap_metrics(bmp, &bmpStride, &bmpPixelWidth);
	
	RECT full_r = {x, y, x + bmp->width, y + bmp->width};
	RECT draw_r = full_r;
	clip_to_screen(&draw_r);
	
	for (y = draw_r.y1; y < draw_r.y2; y++)
	{
		screen_pixel = get_screen_pixel_address(draw_r.x1, y, stride, pixelWidth);
		bitmap_pixel = get_bitmap_pixel_address(bmp, draw_r.x1 - full_r.x1, y - full_r.y1, bmpStride, bmpPixelWidth);
		end_screen_pixel = screen_pixel + stride;
		for (; screen_pixel < end_screen_pixel; screen_pixel += pixelWidth, bitmap_pixel += bmpPixelWidth)
		{
			int b = screen_pixel[0];
			int g = screen_pixel[1];
			int r = screen_pixel[2];
			int op = (bitmap_pixel[3] * opacity) >> 8;
			b = b * 255 + ((int)bitmap_pixel[0] - b) * op;
			g = g * 255 + ((int)bitmap_pixel[1] - g) * op;
			r = r * 255 + ((int)bitmap_pixel[2] - r) * op;
			screen_pixel[0] = (unsigned char)(b >> 8);
			screen_pixel[1] = (unsigned char)(g >> 8);
			screen_pixel[2] = (unsigned char)(r >> 8);
		}
	}
}

void invert_rect(RECT *r)
{
	int stride, pixelWidth;
	get_screen_metrics(&stride, &pixelWidth);
	
	RECT draw_r = *r;
	clip_to_screen(&draw_r);
	
	for (int y = draw_r.y1; y < draw_r.y2; y++)
	{
		unsigned char *pixel = get_screen_pixel_address(draw_r.x1, y, stride, pixelWidth);
		for (int x = draw_r.x1; x < draw_r.x2; x++, pixel += pixelWidth)
		{
			pixel[0] = 255 - pixel[0];
			pixel[1] = 255 - pixel[1];
			pixel[2] = 255 - pixel[2];
		}
	}
}

// Unclipped screen coordinates. Buffer size is same as rectangle size. Assumes buffer is 24bpp.
void screen_to_buffer(RECT *r, unsigned char *buffer)
{
	int stride, pixelWidth;
	get_screen_metrics(&stride, &pixelWidth);
	
	int i = 0;
	RECT full_r = *r;
	RECT draw_r = *r;
	rect_normalize(&full_r);
	clip_to_screen(&draw_r);
	
	int cursor_width = full_r.x2 - full_r.x1;
	
	for (int y = draw_r.y1; y < draw_r.y2; y++)
	{
		unsigned char *pixel = get_screen_pixel_address(draw_r.x1, y, stride, pixelWidth);
		i = ((y - full_r.y1) * cursor_width + (draw_r.x1 - full_r.x1)) * pixelWidth;
		for (int x = draw_r.x1; x < draw_r.x2; x++, pixel += pixelWidth, i += 3)
		{
			buffer[i] = pixel[0];
			buffer[i+1] = pixel[1];
			buffer[i+2] = pixel[2];
		}
	}
}

// Unclipped screen coordinates. Buffer size is same as rectangle size.
void buffer_to_screen(unsigned char *buffer, RECT *r)
{
	int stride, pixelWidth;
	get_screen_metrics(&stride, &pixelWidth);
	
	int i = 0;
	RECT full_r = *r;
	RECT draw_r = *r;
	rect_normalize(&full_r);
	clip_to_screen(&draw_r);
	
	int cursor_width = full_r.x2 - full_r.x1;
	
	for (int y = draw_r.y1; y < draw_r.y2; y++)
	{
		unsigned char *pixel = get_screen_pixel_address(draw_r.x1, y, stride, pixelWidth);
		i = ((y - full_r.y1) * cursor_width + (draw_r.x1 - full_r.x1)) * pixelWidth;
		for (int x = draw_r.x1; x < draw_r.x2; x++, pixel += pixelWidth, i += 3)
		{
			pixel[0] = buffer[i];
			pixel[1] = buffer[i+1];
			pixel[2] = buffer[i+2];
		}
	}
}

void rect(RECT *r, char bSolid, int iborder, int c, int cborder, int opacity)
{
	int y;
	unsigned char *pixel;
	int stride, pixelWidth;
	get_screen_metrics(&stride, &pixelWidth);
	
	RECT draw_r = *r;
	RECT fill_r = *r;
	if (iborder >= 0) // Drawing area is enlarged
	{
		draw_r.x1 -= iborder;
		draw_r.y1 -= iborder;
		draw_r.x2 += iborder;
		draw_r.y2 += iborder;
	}
	else // Filling area is reduced
	{
		fill_r.x1 -= iborder;
		fill_r.y1 -= iborder;
		fill_r.x2 += iborder;
		fill_r.y2 += iborder;
	}
	clip_to_screen(&draw_r);
	clip_to_screen(&fill_r);
	
	// Top border
	if (iborder)
	for (y = draw_r.y1; y < fill_r.y1; y++)
	{
		int x = draw_r.x1;
		pixel = get_screen_pixel_address(x, y, stride, pixelWidth);
		for (; x < draw_r.x2; x++, pixel += pixelWidth)
		{
			set_pixel_opacity(pixel, cborder, opacity);
		}
	}
	
	// Middle part (y)
	for (; y < fill_r.y2; y++)
	{
		int x = draw_r.x1;
		pixel = get_screen_pixel_address(x, y, stride, pixelWidth);
		// Left border
		for (; x < fill_r.x1; x++, pixel += pixelWidth)
		{
			set_pixel_opacity(pixel, cborder, opacity);
		}
		// Middle part (x)
		if (bSolid)
		for (; x < fill_r.x2; x++, pixel += pixelWidth)
		{
			set_pixel_opacity(pixel, c, opacity);
		}
		// Right border
		for (x = fill_r.x2; x < draw_r.x2; x++, pixel += pixelWidth)
		{
			set_pixel_opacity(pixel, cborder, opacity);
		}
	}
	// Bottom border
	if (iborder)
	for (; y < draw_r.y2; y++)
	{
		int x = draw_r.x1;
		pixel = get_screen_pixel_address(x, y, stride, pixelWidth);
		for (; x < draw_r.x2; x++, pixel += pixelWidth)
		{
			set_pixel_opacity(pixel, cborder, opacity);
		}
	}
}

int read_bitmap(Bitmap *b, char *filename)
{
       FileStream fs;
       if (!file_open(filename, &fs)) return 0;
       BITMAPFILEHEADER bf;
       file_read(&fs, &bf, sizeof(BITMAPFILEHEADER));
       if (bf.bfType != 0x4D42)
       {
           file_close(&fs);
           return 0;
       }
       BITMAPCOREHEADER bc;
       file_read(&fs, &bc, sizeof(BITMAPCOREHEADER));
       void* buffer = kmalloc(bc.bcWidth*bc.bcHeight*(bc.bcBitCount/8));
       if (!buffer)
       {
           file_close(&fs);
           return 0;
       }
       file_seek(&fs, bf.bfOffBits);
       file_read(&fs, buffer, bc.bcWidth*bc.bcHeight*(bc.bcBitCount/8));
       file_close(&fs);
       b->width = bc.bcWidth;
       b->height = bc.bcHeight;
       b->bits = buffer;
       b->bpp = bc.bcBitCount;
       return 1;
}

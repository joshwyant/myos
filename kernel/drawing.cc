#include "kernel.h"

extern "C" {

void clear_color(int c)
{
	kernel::GraphicsDriver::get_current()
		->get_screen_context()
		->get_raw_context()
		->clear_color(c);
}
void bitblt(Bitmap *bmp, int x, int y)
{
	kernel::GraphicsDriver::get_current()
		->get_screen_context()
		->get_raw_context()
		->bitblt(bmp, x, y);
}
void draw_image(Bitmap *bmp, int x, int y, int opacity)
{
	kernel::GraphicsDriver::get_current()
		->get_screen_context()
		->get_raw_context()
		->draw_image(bmp, x, y, opacity);
}
void draw_image_bgra(Bitmap *bmp, int x, int y, int opacity)
{
	kernel::GraphicsDriver::get_current()
		->get_screen_context()
		->get_raw_context()
		->draw_image_bgra(bmp, x, y, opacity);
}
void draw_image_ext(Bitmap *bmp, RECT *src, RECT *dest, int opacity, int c)
{
	kernel::GraphicsDriver::get_current()
		->get_screen_context()
		->get_raw_context()
		->draw_image_ext(bmp, src, dest, opacity, c);
}
void rect(RECT *r, char bSolid, int iborder, int c, int cborder, int opacity)
{
	kernel::GraphicsDriver::get_current()
		->get_screen_context()
		->get_raw_context()
		->rect(r, bSolid, iborder, c, cborder, opacity);
}
void invert_rect(RECT *r)
{
	kernel::GraphicsDriver::get_current()
		->get_screen_context()
		->get_raw_context()
		->invert_rect(r);
}
void screen_to_buffer(RECT *r, unsigned char *buffer)
{
	kernel::GraphicsDriver::get_current()
		->get_screen_context()
		->get_raw_context()
		->screen_to_buffer(r, buffer);
}
void buffer_to_screen(unsigned char *buffer, RECT *r)
{
	kernel::GraphicsDriver::get_current()
		->get_screen_context()
		->get_raw_context()
		->buffer_to_screen(buffer, r);
}
void draw_text(const char *str, int x, int y, int c, int opacity, int xsize, int ysize)
{
	kernel::GraphicsDriver::get_current()
		->get_screen_context()
		->get_raw_context()
		->draw_text(str, x, y, c, opacity, xsize, ysize);
}

} // extern "C"

void kernel::MemoryGraphicsContext::clear_color(int c)
{
	int stride, pixelWidth;
	get_screen_metrics(&stride, &pixelWidth);
	
	unsigned char *line_begin = get_frame_buffer();
	unsigned char *line_end = line_begin + get_width() * pixelWidth;
	unsigned char *end = line_begin + stride * get_height();
	for (unsigned char *pixel = get_frame_buffer(); pixel < end; pixel += pixelWidth)
	{
		if (pixel == line_end)
		{
			line_begin += stride;
			pixel = line_begin;
			if (pixel >= end) break;
		}
		set_pixel(pixel, c);
	}
}

void kernel::MemoryGraphicsContext::draw_text(const char *str, int x, int y, int color, int opacity, int xsize, int ysize)
{
	static Bitmap font, *pFont = 0;
	if (!pFont)
	{
		pFont = &font;
		read_bitmap(pFont, "/system/bin/font");
	}
	
	RECT draw_r = {x, y, x + xsize, y + ysize};
	
	for (; *str; str++)
	{
		if (*str == '\r') continue;
		if (*str == '\n')
		{
			draw_r.x1 = x;
			draw_r.x2 = x + xsize;
			draw_r.y1 += ysize;
			draw_r.y2 += ysize;
			continue;
		}
		unsigned char c = *str;
		if (c > 127) c = 0;
		int char_col = c & 0xF;
		int char_row = c >> 4;
		RECT char_rect = {char_col * 8, char_row * 8, (char_col + 1) * 8, (char_row + 1) * 8};
		draw_image_ext(pFont, &char_rect, &draw_r, opacity, color);
		draw_r.x1 += xsize;
		draw_r.x2 += xsize;
	}
}

void kernel::MemoryGraphicsContext::bitblt(Bitmap *bmp, int x, int y)
{
	unsigned char *bitmap_pixel, *screen_pixel, *end_screen_pixel;
	int stride, pixelWidth, bmpStride, bmpPixelWidth;
	get_screen_metrics(&stride, &pixelWidth);
	get_bitmap_metrics(bmp, &bmpStride, &bmpPixelWidth);
	
	RECT full_r = {x, y, x + bmp->width, y + bmp->height};
	RECT draw_r = full_r;
	clip_to_screen(&draw_r);
	
	for (y = draw_r.y1; y < draw_r.y2; y++)
	{
		screen_pixel = get_screen_pixel_address(draw_r.x1, y, stride, pixelWidth);
		bitmap_pixel = get_bitmap_pixel_address(bmp, draw_r.x1 - full_r.x1, y - full_r.y1, bmpStride, bmpPixelWidth);
		end_screen_pixel = screen_pixel + (draw_r.x2 - draw_r.x1) * pixelWidth;
		for (; screen_pixel < end_screen_pixel; screen_pixel += pixelWidth, bitmap_pixel += bmpPixelWidth)
		{
			screen_pixel[0] = bitmap_pixel[0];
			screen_pixel[1] = bitmap_pixel[1];
			screen_pixel[2] = bitmap_pixel[2];
		}
	}
}

void kernel::MemoryGraphicsContext::draw_onto(GraphicsContext *other, int x, int y)
{
	unsigned char *bitmap_pixel, *screen_pixel, *end_screen_pixel;
	int stride, pixelWidth, bmpStride, bmpPixelWidth;
	other->get_screen_metrics(&stride, &pixelWidth);
	get_screen_metrics(&bmpStride, &bmpPixelWidth);
	
	RECT full_r = {x, y, x + width, y + height};
	RECT draw_r = full_r;
	other->clip_to_screen(&draw_r);
	
	for (y = draw_r.y1; y < draw_r.y2; y++)
	{
		screen_pixel = other->get_screen_pixel_address(draw_r.x1, y, stride, pixelWidth);
		bitmap_pixel = get_screen_pixel_address(draw_r.x1 - full_r.x1, y - full_r.y1, bmpStride, bmpPixelWidth);
		end_screen_pixel = screen_pixel + (draw_r.x2 - draw_r.x1) * pixelWidth;
		for (; screen_pixel < end_screen_pixel; screen_pixel += pixelWidth, bitmap_pixel += bmpPixelWidth)
		{
			screen_pixel[0] = bitmap_pixel[0];
			screen_pixel[1] = bitmap_pixel[1];
			screen_pixel[2] = bitmap_pixel[2];
		}
	}
}

void kernel::MemoryGraphicsContext::draw_image(Bitmap *bmp, int x, int y, int opacity)
{
	if (bmp->bpp > 24)
	{
		draw_image_bgra(bmp, x, y, opacity);
		return;
	}
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
	
	RECT full_r = {x, y, x + bmp->width, y + bmp->height};
	RECT draw_r = full_r;
	clip_to_screen(&draw_r);
	
	for (y = draw_r.y1; y < draw_r.y2; y++)
	{
		screen_pixel = get_screen_pixel_address(draw_r.x1, y, stride, pixelWidth);
		bitmap_pixel = get_bitmap_pixel_address(bmp, draw_r.x1 - full_r.x1, y - full_r.y1, bmpStride, bmpPixelWidth);
		end_screen_pixel = screen_pixel + (draw_r.x2 - draw_r.x1) * pixelWidth;
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

void kernel::MemoryGraphicsContext::draw_image_bgra(Bitmap *bmp, int x, int y, int opacity)
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
	
	RECT full_r = {x, y, x + bmp->width, y + bmp->height};
	RECT draw_r = full_r;
	clip_to_screen(&draw_r);
	
	for (y = draw_r.y1; y < draw_r.y2; y++)
	{
		screen_pixel = get_screen_pixel_address(draw_r.x1, y, stride, pixelWidth);
		bitmap_pixel = get_bitmap_pixel_address(bmp, draw_r.x1 - full_r.x1, y - full_r.y1, bmpStride, bmpPixelWidth);
		end_screen_pixel = screen_pixel + (draw_r.x2 - draw_r.x1) * pixelWidth;
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

void kernel::MemoryGraphicsContext::draw_image_ext(Bitmap *bmp, RECT *src, RECT *dest, int opacity, int c)
{
	if (opacity == 0) return;
	unsigned char *bitmap_pixel, *screen_pixel, *end_screen_pixel;
	int stride, pixelWidth, bmpStride, bmpPixelWidth;
	int src_w, src_h, dest_w, dest_h, bmp_x, bmp_y, x, y;
	get_screen_metrics(&stride, &pixelWidth);
	get_bitmap_metrics(bmp, &bmpStride, &bmpPixelWidth);
	
	RECT draw_r = *dest;
	clip_to_screen(&draw_r);
	src_w = src->x2 - src->x1;
	src_h = src->y2 - src->y1;
	dest_w = dest->x2 - dest->x1;
	dest_h = dest->y2 - dest->y1;
	
	int c_r = GET_R(c), c_g = GET_G(c), c_b = GET_B(c);
	
	for (y = draw_r.y1; y < draw_r.y2; y++)
	{
		bmp_y = (dest_h * src->y1 + (y - dest->y1) * src_h) / dest_h;
		//if (bmp_y < 0) bmp_y = 0; if (bmp_y >= bmp->height) bmp_y = bmp->height - 1;
		screen_pixel = get_screen_pixel_address(draw_r.x1, y, stride, pixelWidth);
		end_screen_pixel = screen_pixel + (draw_r.x2 - draw_r.x1) * pixelWidth;
		for (x = draw_r.x1; screen_pixel < end_screen_pixel; screen_pixel += pixelWidth, x++)
		{
			bmp_x = (dest_w * src->x1 + (x - dest->x1) * src_w) / dest_w;
			//if (bmp_x < 0) bmp_x = 0; if (bmp_x >= bmp->width) bmp_x = bmp->width - 1;
			bitmap_pixel = get_bitmap_pixel_address(bmp, bmp_x, bmp_y, bmpStride, bmpPixelWidth);
			int b = screen_pixel[0];
			int g = screen_pixel[1];
			int r = screen_pixel[2];
			int op = bmp->bpp > 24 ? (bitmap_pixel[3] * opacity) / 255 : opacity;
			b = b * 255 + (((int)bitmap_pixel[0] * c_b) / 255 - b) * op;
			g = g * 255 + (((int)bitmap_pixel[1] * c_g) / 255 - g) * op;
			r = r * 255 + (((int)bitmap_pixel[2] * c_r) / 255 - r) * op;
			screen_pixel[0] = (unsigned char)(b / 255);
			screen_pixel[1] = (unsigned char)(g / 255);
			screen_pixel[2] = (unsigned char)(r / 255);
		}
	}
}

void kernel::MemoryGraphicsContext::invert_rect(RECT *r)
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
void kernel::MemoryGraphicsContext::screen_to_buffer(RECT *r, unsigned char *buffer)
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
void kernel::MemoryGraphicsContext::buffer_to_screen(unsigned char *buffer, RECT *r)
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

void kernel::MemoryGraphicsContext::rect(RECT *r, char bSolid, int iborder, int c, int cborder, int opacity)
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
	{
		for (y = draw_r.y1; y < fill_r.y1; y++)
		{
			int x = draw_r.x1;
			pixel = get_screen_pixel_address(x, y, stride, pixelWidth);
			for (; x < draw_r.x2; x++, pixel += pixelWidth)
			{
				set_pixel_opacity(pixel, cborder, opacity);
			}
		}
	}
	else
	{
		y = fill_r.y1;
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
		{
			for (; x < fill_r.x2; x++, pixel += pixelWidth)
			{
				set_pixel_opacity(pixel, c, opacity);
			}
		}
		else
		{
			x += fill_r.x2 - fill_r.x1;
			pixel += (fill_r.x2 - fill_r.x1) * pixelWidth;
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

void kernel::BufferedGraphicsContext::swap_buffers()
{
	unsigned char *buffer_pixel, *screen_pixel, *end_screen_pixel;
	int stride, pixelWidth;
	get_screen_metrics(&stride, &pixelWidth);
	
	for (int y = 0; y < get_height(); y++)
	{
		screen_pixel = get_raw_context()->get_screen_pixel_address(0, y, stride, pixelWidth);
		buffer_pixel = get_screen_pixel_address(0, y, stride, pixelWidth);
		end_screen_pixel = screen_pixel + get_width() * pixelWidth;
		for (; screen_pixel < end_screen_pixel; screen_pixel += pixelWidth, buffer_pixel += pixelWidth)
		{
			screen_pixel[0] = buffer_pixel[0];
			screen_pixel[1] = buffer_pixel[1];
			screen_pixel[2] = buffer_pixel[2];
		}
	}
}

// BufferedGraphicsContext members
kernel::GraphicsContext *kernel::BufferedMemoryGraphicsContext::get_raw_context()
{
	return raw_context;
}
void kernel::BufferedMemoryGraphicsContext::swap_buffers()
{
	// TODO
}

int read_bitmap(Bitmap *b, const char *filename)
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

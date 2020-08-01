#ifndef __DRAWING_H__
#define __DRAWING_H__

#include <stddef.h>
#include "disk.h"
#include "fs.h"

#define RGB(r, g, b) ((r) | (g) << 8 | (b) << 16)
#define RGBA(r, g, b, a) ((r) | (g) << 8 | (b) << 16 | (a) << 24)
#define GET_R(rgb) ((rgb) & 0xFF)
#define GET_G(rgb) (((rgb) >> 8) & 0xFF)
#define GET_B(rgb) (((rgb) >> 16) & 0xFF)
#define GET_A(rgba) (((unsigned)(rgba) >> 24) & 0xFF)

#ifdef __cplusplus
#include <memory>

extern "C" {
#endif // __cplusplus

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
	int direction;
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

inline static unsigned char * get_bitmap_pixel_address(Bitmap *bmp, int x, int y, int stride, int pixelWidth)
{
	return  bmp->direction
				? (unsigned char *)bmp->bits
					+ bmp->width * bmp->height * pixelWidth 
					- stride * (y + 1)
					+ x * pixelWidth
				: (unsigned char *)bmp->bits
					+ stride * y
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

inline static void get_bitmap_metrics(Bitmap *bmp, int *stride, int *pixelWidth)
{
	*pixelWidth = bmp->bpp >> 3;
	*stride = bmp->width**pixelWidth;
}

#ifdef __cplusplus
}  // extern "C"

bool read_bitmap(std::shared_ptr<kernel::FileSystem> fs, Bitmap *b, const char *filename);

namespace kernel
{
class BufferedGraphicsContext;

class GraphicsDriver
    : public Driver
{
public:
    GraphicsDriver(String device_name)
        : Driver(device_name) {}
    virtual BufferedGraphicsContext *get_screen_context() = 0;
    virtual ~GraphicsDriver() {}
}; // class GraphicsContext

class GraphicsContext
{
public:
	GraphicsContext()
		: bmp(nullptr) {}
	virtual ~GraphicsContext()
	{
		if (bmp)
		{
			delete bmp;
			bmp = nullptr;
		}
	}
	Bitmap *as_bitmap()
	{
		if (!bmp)
		{
			bmp = new Bitmap();
			bmp->bits = get_frame_buffer();
			bmp->bpp = get_bpp();
			bmp->height = get_height();
			bmp->width = get_width();
			bmp->direction = 0;
		}
		return bmp;
	}
    virtual void clear_color(int c) = 0;
	virtual void draw_onto(GraphicsContext *other, int x, int y) = 0;
    virtual void bitblt(Bitmap *bmp, int x, int y) = 0;
    virtual void draw_image(Bitmap *bmp, int x, int y, int opacity) = 0;
    virtual void draw_image_bgra(Bitmap *bmp, int x, int y, int opacity) = 0;
    virtual void draw_image_ext(Bitmap *bmp, RECT *src, RECT *dest, int opacity, int c) = 0;
    virtual void rect(RECT *r, char bSolid, int iborder, int c, int cborder, int opacity) = 0;
    virtual void invert_rect(RECT *r) = 0;
    virtual void screen_to_buffer(RECT *r, unsigned char *buffer) = 0;
    virtual void buffer_to_screen(unsigned char *buffer, RECT *r) = 0;
    virtual void draw_text(const char *str, int x, int y, int c, int opacity, int xsize, int ysize) = 0;
    virtual int get_width() = 0;
    virtual int get_height() = 0;
    virtual unsigned char *get_frame_buffer() = 0;
    virtual int get_bpp() = 0;
    virtual int get_stride() = 0;

	unsigned char * get_screen_pixel_address(int x, int y, int stride, int pixelWidth)
	{
		return (unsigned char *)get_frame_buffer() + y * stride + x * pixelWidth;
	}

	void get_screen_metrics(int *stride, int *pixelWidth)
	{
		*pixelWidth = get_bpp() >> 3;
		*stride = get_stride();
	}

	void clip_to_screen(RECT *draw_r)
	{
		RECT screen_clip = {0, 0, get_width(), get_height()};
		rect_clip(draw_r, screen_clip);
	}

	void set_screen_pixel(int x, int y, int c)
	{
		int stride, pixelWidth;
		get_screen_metrics(&stride, &pixelWidth);
		return set_pixel(get_screen_pixel_address(x, y, stride, pixelWidth), c);
	}

	int get_screen_pixel(int x, int y)
	{
		int stride, pixelWidth;
		get_screen_metrics(&stride, &pixelWidth);
		return get_pixel(get_screen_pixel_address(x, y, stride, pixelWidth));
	}
protected:
	Bitmap *bmp;
}; // class GraphicsContext

class BufferedGraphicsContext
    : public GraphicsContext
{
public:
	BufferedGraphicsContext()
		: GraphicsContext() {}
    virtual GraphicsContext *get_raw_context() = 0;
    virtual void swap_buffers() = 0;
	virtual ~BufferedGraphicsContext() {}
}; // class BufferedGraphicsContext

class MemoryGraphicsContext
	: public GraphicsContext
{
public:
	MemoryGraphicsContext(std::shared_ptr<kernel::FileSystem> fs, unsigned char *buffer, int bpp, int width, int height, int stride = 0)
		: fs(fs),
		  stride(stride ? stride : width * (bpp >> 3)),
		  own_buffer(buffer ? false : true),
		  buffer(buffer ? buffer : new unsigned char[height * this->stride]),
		  bpp(bpp),
		  width(width),
		  height(height),
		  GraphicsContext() {}
	virtual ~MemoryGraphicsContext()
	{
		if (own_buffer && buffer)
		{
			delete[] buffer;
		}
	}
    void clear_color(int c) override;
	void draw_onto(GraphicsContext *other, int x, int y) override;
    void bitblt(Bitmap *bmp, int x, int y) override;
    void draw_image(Bitmap *bmp, int x, int y, int opacity) override;
    void draw_image_bgra(Bitmap *bmp, int x, int y, int opacity) override;
    void draw_image_ext(Bitmap *bmp, RECT *src, RECT *dest, int opacity, int c) override;
    void rect(RECT *r, char bSolid, int iborder, int c, int cborder, int opacity) override;
    void invert_rect(RECT *r) override;
    void screen_to_buffer(RECT *r, unsigned char *buffer) override;
    void buffer_to_screen(unsigned char *buffer, RECT *r) override;
    void draw_text(const char *str, int x, int y, int c, int opacity, int xsize, int ysize) override;
    int get_width() override { return width; }
    int get_height() override { return height; }
    unsigned char *get_frame_buffer() override { return buffer; }
    int get_bpp() override { return bpp; }
    int get_stride() override { return stride; }
	MemoryGraphicsContext subcontext(RECT r)
	{
		RECT clipped_rect = r;
		clip_to_screen(&clipped_rect);
		return MemoryGraphicsContext(fs, buffer + (r.x1 * (bpp >> 3)), r.x2 - r.x1, r.y2 - r.y1, stride);
	}
protected:
	friend class BufferedMemoryGraphicsContext;
	int width, height, stride, bpp;
	unsigned char *buffer;
	bool own_buffer;
	std::shared_ptr<kernel::FileSystem> fs;
}; // class MemoryGraphicsContext

class BufferedMemoryGraphicsContext
	: public BufferedGraphicsContext
{
public:
	BufferedMemoryGraphicsContext(MemoryGraphicsContext *raw_context)
		: BufferedGraphicsContext()
	{
		own_raw_context = false;
		this->raw_context = raw_context;
		this->buffer_context 
			= new MemoryGraphicsContext(
				raw_context->fs,
				nullptr,
				raw_context->get_bpp(),
				raw_context->get_width(),
				raw_context->get_height(),
				raw_context->get_stride());
	}
	BufferedMemoryGraphicsContext(std::shared_ptr<kernel::FileSystem> fs, int bpp, int width, int height, int stride = 0)
	{
		own_raw_context = true;
		this->raw_context = new MemoryGraphicsContext(fs, nullptr, bpp, width, height, stride);
		this->buffer_context = new MemoryGraphicsContext(fs, nullptr, bpp, width, height, stride);
	}
	virtual ~BufferedMemoryGraphicsContext()
	{
		if (own_raw_context && raw_context)
		{
			delete raw_context;
		}
		delete buffer_context;
	}
    void clear_color(int c) override
	{
		buffer_context->clear_color(c);
	}
    void bitblt(Bitmap *bmp, int x, int y) override
	{
		buffer_context->bitblt(bmp, x, y);
	}
	void draw_onto(GraphicsContext *other, int x, int y) override
	{
		buffer_context->draw_onto(other, x, y);
	}
    void draw_image(Bitmap *bmp, int x, int y, int opacity) override
	{
		buffer_context->draw_image(bmp, x, y, opacity);
	}
    void draw_image_bgra(Bitmap *bmp, int x, int y, int opacity) override
	{
		buffer_context->draw_image_bgra(bmp, x, y, opacity);
	}
    void draw_image_ext(Bitmap *bmp, RECT *src, RECT *dest, int opacity, int c) override
	{
		buffer_context->draw_image_ext(bmp, src, dest, opacity, c);
	}
    void rect(RECT *r, char bSolid, int iborder, int c, int cborder, int opacity) override
	{
		buffer_context->rect(r, bSolid, iborder, c, cborder, opacity);
	}
    void invert_rect(RECT *r) override
	{
		buffer_context->invert_rect(r);
	}
    void screen_to_buffer(RECT *r, unsigned char *buffer) override
	{
		buffer_context->screen_to_buffer(r, buffer);
	}
    void buffer_to_screen(unsigned char *buffer, RECT *r) override
	{
		buffer_context->buffer_to_screen(buffer, r);
	}
    void draw_text(const char *str, int x, int y, int c, int opacity, int xsize, int ysize) override
	{
		buffer_context->draw_text(str, x, y, c, opacity, xsize, ysize);
	}
    int get_width() override
	{
		return buffer_context->get_width();
	}
    int get_height() override
	{
		return buffer_context->get_height();
	}
    unsigned char *get_frame_buffer() override
	{
		return buffer_context->get_frame_buffer();
	}
    int get_bpp() override
	{
		return buffer_context->get_bpp();
	}
    int get_stride() override
	{
		return buffer_context->get_stride();
	}

	// BufferedGraphicsContext members
    GraphicsContext *get_raw_context() override;
    void swap_buffers() override;
protected:
	MemoryGraphicsContext *raw_context;
	MemoryGraphicsContext *buffer_context;
	bool own_raw_context;
	std::shared_ptr<kernel::FileSystem> fs;
}; // class BufferedMemoryGraphicsContext

}	 	// namespace kernel
#endif  // __cplusplus
#endif  // __DRAWING_H__

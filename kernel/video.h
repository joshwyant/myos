#ifndef __video_h__
#define __video_h__

#include "clock.h"
#include "drawing.h"
#include "driver.h"
#include "memory.h"
#include "sync.h"

#ifdef __cplusplus
#include <memory>

extern "C" {
#endif

#define C_BLACK         0
#define C_BLUE          1
#define C_GREEN         2
#define C_CYAN          3
#define C_RED           4
#define C_MAGENTA       5
#define C_BROWN         6
#define C_LIGHTGRAY     7
#define C_DARKGRAY      8
#define C_LIGHTBLUE     9
#define C_LIGHTGREEN    10
#define C_LIGHTCYAN     11
#define C_LIGHTRED      12
#define C_PINK          13
#define C_YELLOW        14
#define C_WHITE         15

extern int console_palette[];

volatile char *get_console_videomem();
int get_console_rows();
int get_console_cols();
void move_cursor(int);
void print_char(char);
void print(const char*);
void printlen(const char*, int);
void show_cursor(int);
void cls();
void cls_color(int fore, int back);
void printhexb(char x);
void printhexw(short x);
void printhexd(int x);
void printdec(int x);
void kprintdatetime(DateTime dt);
void kprintf(const char* format, ...);
void endl();
// void print_datetime();

#ifdef __cplusplus
}  // extern "C"

namespace kernel
{
class ConsoleDriver
    : public Driver
{
public:
    ConsoleDriver(
        void *videomem = nullptr,
        int rows = 25, int cols = 80)
        :  videomem_provided(videomem ? true : false),
           videomem(videomem
                        ? (volatile char*)kfindrange(rows * cols * 2) 
                        : new char[rows * cols * 2]),
           rows(rows), cols(cols),
           fore_color(C_LIGHTGRAY),
           back_color(C_BLACK),
           cursor_shown(0),
           cursorpos(0),
           lastpos(0),
           screenlock(0),
           cursorlock(0),
           show_cursorlock(0),
           Driver("console")
    {
        if (videomem_provided)
        {
            page_map((void*)this->videomem, (void*)videomem, PF_WRITE);
        }
    }
    ConsoleDriver(int rows, int cols)
        : ConsoleDriver(nullptr, rows, cols) {}
    virtual ~ConsoleDriver() {
        if (videomem && videomem_provided)
        {
            delete[] videomem;
            videomem = nullptr;
        }
    }
    virtual void show_cursor(int) = 0;
    virtual void redraw() {}
    virtual void redraw(int) {}
    void move_cursor(int);
    void print_char(char);
    void print(const char*);
    void printlen(const char*, int);
    void cls();
    void cls(int fore, int back) {
        set_fore_color(fore);
        set_back_color(back);
        cls();
    }
    void printhexb(char x);
    void printhexw(short x);
    void printhexd(int x);
    void printdec(int x);
    void kprintdatetime(DateTime dt);
    void kprintf(const char* format, ...);
    void endl();
    int get_rows() const { return rows; }
    int get_cols() const { return cols; }
    volatile char *get_videomem() const { return videomem; }
    void set_fore_color(int color) { fore_color = color; }
    void set_back_color(int color) { back_color = color; }
    unsigned short get_cursor_pos() const { return cursorpos; }
protected:
    virtual void update_cursor_index() = 0;
    volatile char *videomem;
    int rows;
    int cols;
    char cursor_shown;
    unsigned short cursorpos;
    unsigned short lastpos;
    bool videomem_provided;
    int fore_color;
    int back_color;

    // spinlocks
    int screenlock;
    int cursorlock;
    int show_cursorlock;
private:
    void __print_char(char c, unsigned char color)
    {
        if (c == '\r') return;
        if (c == '\n')
        _print_char('\r', color);
        _print_char(c, color);
    }

    void _print_char(char c, unsigned char color)
    {
        ScopedLock lock(screenlock);
        int i;
        volatile unsigned short* line1 = 
            (volatile unsigned short*)videomem;
        volatile unsigned short* line2 = 
            (volatile unsigned short*)(videomem+2*cols);
        if (c == 8) // backspace
        {
            if (cursorpos != 0) cursorpos--;
        }
        else if (c == 10) // lf
            cursorpos += cols;
        else if (c == 13) // cr
            cursorpos -= cursorpos%cols;
        else if ((c >= 0x20) && (c < 0x7F))
        {
            *(volatile unsigned short*)(videomem+cursorpos*2) = 
                (unsigned char)c|(color<<8);
            redraw(cursorpos);
            cursorpos++;
        }
        if (cursorpos >= rows * cols)
        {
            cursorpos -= cols;
            for (i = 0; i < (rows - 1) * cols; i++, line1++, line2++)
                *line1 = *line2;
            line1 = (volatile unsigned short*)(videomem+ 2 * (rows - 1) * cols);
            for (i = 0; i < cols; i++, line1++)
                *line1 = (unsigned short)' ' | (color << 8);
            redraw();
        }
    }
}; // class ConsoleDriver

class TextModeConsoleDriver : public ConsoleDriver
{
public:
    TextModeConsoleDriver();
    void show_cursor(int) override;
protected:
    void update_cursor_index() override
    {
        outb(crtbaseio,0xF);
        outb(crtbaseio+1,(unsigned char)(cursorpos));
        outb(crtbaseio,0xE);
        outb(crtbaseio+1,(unsigned char)(cursorpos>>8));
    }
    unsigned short crtbaseio;
}; // class TextModeConsoleDriver

class GraphicalConsoleDriver : public ConsoleDriver
{
public:
    GraphicalConsoleDriver(std::shared_ptr<FileSystem> fs, int rows, int cols);
    ~GraphicalConsoleDriver() override;
    void show_cursor(int) override;
    void redraw() override;
    void redraw(int) override;
protected:
    void update_cursor_index() override;
    Bitmap font;
    std::shared_ptr<FileSystem> fs;
    MemoryGraphicsContext *display;
}; // class TextModeConsoleDriver
}  // namespace kernel
#endif // __cplusplus
#endif // __video_h__

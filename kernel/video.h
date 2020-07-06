#ifndef __video_h__
#define __video_h__

#include "kernel.h"
#include "drawing.h"
#include "klib.h"

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

#ifdef __cplusplus
extern "C" {
#endif

extern int console_palette[];

void init_video();
void init_graphical_console();
volatile char *get_console_videomem();
int get_console_rows();
int get_console_cols();
void move_cursor(int);
void print_char(char);
void print(const char*);
void printlen(const char*, int);
void show_cursor(int);
void cls();
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
{
public:
    ConsoleDriver(volatile char videomem[], int rows, int cols) {
        if (videomem)
        {
            videomem_provided = true;
        }
        this->videomem = videomem ? videomem : new char [rows * cols * 2];
        this->rows = rows;
        this->cols = cols;
    };
    ~ConsoleDriver() {
        if (videomem && videomem_provided)
        {
            delete[] videomem;
            videomem = nullptr;
        }
    }
    virtual void show_cursor(int) = 0;
    void move_cursor(int);
    void print_char(char);
    void print(const char*);
    void printlen(const char*, int);
    void cls();
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
protected:
    virtual void update_cursor_index() = 0;
    volatile char *videomem;
    int rows;
    int cols;
    char cursor_shown;
    unsigned short cursorpos;
    bool videomem_provided;

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
        while (lock(&screenlock)) process_yield(); // Another thread is using this function
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
            cursorpos++;
        }
        if (cursorpos >= rows * cols)
        {
            cursorpos -= cols;
            for (i = 0; i < (rows - 1) * cols; i++, line1++, line2++)
                *line1 = *line2;
            line1 = (volatile unsigned short*)(videomem+ 2 * (rows - 1) * cols);
            for (i = 0; i < cols; i++, line1++)
                *line1 = 0x0720;
        }
        screenlock = 0; // Unlock this function
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
    GraphicalConsoleDriver(int rows, int cols);
    GraphicalConsoleDriver(const TextModeConsoleDriver &original);
    //~GraphicalConsoleDriver() override;
    void show_cursor(int) override;
protected:
    void update_cursor_index() override;
    Bitmap font;
    MemoryGraphicsContext *display;
}; // class TextModeConsoleDriver

class GraphicsDriver
{
public:
    virtual BufferedGraphicsContext *get_screen_context() = 0;
    static GraphicsDriver *get_current() { return current; }
    static void set_current(GraphicsDriver *driver) { current = driver; }
protected:
    static GraphicsDriver *current;
}; // class GraphicsContext
}  // namespace kernel
#endif // __cplusplus
#endif // __video_h__

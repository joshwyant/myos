#include "kernel.h"

// keyboard data
const char kbd_lowercase[] = { 0,0,'1','2','3','4','5','6','7','8','9','0','-','=','\b','\t','q','w','e','r','t','y','u','i','o','p','[',']','\r',0,'a','s','d','f','g','h','j','k','l',';','\'','`',0,'\\','z','x','c','v','b','n','m',',','.','/',0,'*',0,'\x20',0,0,0,0,0,0,0,0,0,0,0,0,0,'7','8','9','-','4','5','6','+','1','2','3','0','.',0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 };
const char kbd_uppercase[] = { 0,0,'!','@','#','$','%','^','&','*','(',')','_','+','\b','\t','Q','W','E','R','T','Y','U','I','O','P','{','}','\r',0,'A','S','D','F','G','H','J','K','L',':','\"','~',0,'|', 'Z','X','C','V','B','N','M','<','>','?',0,'*',0,'\x20',0,0,0,0,0,0,0,0,0,0,0,0,0,'7','8','9','-','4','5','6','+','1','2','3','0','.',0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 };
volatile int kbd_escaped;
volatile int kbd_shift;
volatile char kbd_buffer[32];
volatile int kbd_count = 0;

// called by wrapper irq1()
void handle_keyboard()
{
    char scancode = inb(0x60);
    int escaped = kbd_escaped;
    kbd_escaped = 0;
    if (escaped)
    {
    }
    else switch ((unsigned char)scancode)
    {
    case 0x0E:
        // print("\b\x20\b");
        if (kbd_count == 32) kbd_count = 0;
        kbd_buffer[kbd_count++] = '\b';
        break;
    case 0x1C:
        // endl();
        if (kbd_count == 32) kbd_count = 0;
        kbd_buffer[kbd_count++] = '\r';
        break;
    case 0x2A:
    case 0x36:
        kbd_shift = 1;
        break;
    case 0xAA:
    case 0xB6:
        kbd_shift = 0;
        break;
    case 0xE0:
        kbd_escaped = 1;
        break;
    default:
        if (!(scancode&0x80))
        {
            char c = kbd_shift? kbd_uppercase[scancode] : kbd_lowercase[scancode];
            if (c)
            {
                if (kbd_count == 32) kbd_count = 0;
                kbd_buffer[kbd_count++] = c;
            }
            //print_char(c);
        }
        break;
    }
    eoi(1);
}

// Keyboard initialization
void kbd_init()
{
    // fill descriptor 0x21 (irq 1) for keyboard handler
    register_isr(0x21,0,(void*)irq1);
    // unmask IRQ
    irq_unmask(1);
    // Keyboard variables
    kbd_escaped = 0;
    kbd_shift = 0;
}

// These functions are temporary test functions, 
// designed for kmain demos (not multithreading). They will
// have to be made thread-safe if it is to be extended.

char kbd_peekc()
{
    if (kbd_count)
        return kbd_buffer[kbd_count-1];
    else
        return 0;
}

char kbd_readc()
{
    while (!kbd_count) hlt(); // power saver
        return kbd_buffer[kbd_count---1];
}

void kbd_read(char* str, int max)
{
    char c = kbd_readc();
    int i = 0;
    while ((c == '\x20') || (c == '\r'))
    {
        print_char(c);
        c = kbd_readc();
    }
    while ((c != '\r') && (c != '\x20'))
    {
        if (c == '\b')
        {
            if (i > 0)
            {
                if (max++ >= 0)
                    str--;
                i--;
                print("\b \b");
            }
        }
        else
        {
            if (max-- > 0)
                *str++ = c;
            i++;
            print_char(c);
        }
        c = kbd_readc();
    }
    if (c == '\r')
        endl();
    else
        print_char('\x20');
    *str = 0;
}

void kbd_readln(char* str, int max)
{
    char c = kbd_readc();
    int i = 0;
    while (c != '\r')
    {
        if (c == '\b')
        {
            if (i > 0)
            {
                if (max++ >= 0)
                    str--;
                i--;
                print("\b \b");
            }
        }
        else
        {
            if (max-- > 0)
                *str++ = c;
            i++;
            print_char(c);
        }
        c = kbd_readc();
    }
    endl();
    *str = 0;
}

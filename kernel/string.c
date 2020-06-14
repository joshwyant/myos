#include "kernel.h"

const char* ksprintdec(char* str, int x)
{
    unsigned temp = (unsigned)(x < 0 ? -x : x);
    unsigned div;
    unsigned mod;
    char str2[11];
    char *strptr = str;
    char *strptr2 = str2;
    if (x < 0)
    {
        str[0] = '-';
        strptr++;
    }
    do
    {
        div = temp/10;
        mod = temp%10;
        temp = div;
        *strptr = '0'+mod;
        strptr++;
    } while (temp != 0);
    *strptr = 0;
    do
    {
        strptr--;
        *strptr2 = *strptr;
        strptr2++;
    } while (strptr != str);
    *strptr2 = 0;
    return str;
}

const char* ksprintf(char* dest, const char* format, ...)
{
    const char* destorig = dest;
    int arg = 0;
    while (*format)
    {
        volatile unsigned val;
        // Skip 4 items on the stack at %ebp: prev %epb, %eip, $dest, and $format.
        asm volatile ("movl (%%ebp,%1), %0":"=r"(val):"r"((arg+4)*4));
        if (*format == '%')
        {
            switch (*++format)
            {
                case 'd':
                    arg++;
                    ksprintdec(dest, val);
                    while (*dest) dest++;
                    break;
                case 's':
                    arg++;
                    kstrcpy(dest, (const char*)val);
                    while (*dest) dest++;
                    break;
                case 'c':
                    arg++;
                    *dest++ = (char)val;
                    break;
                case 'b':
                    arg++;
                    kstrcpy(dest, "0x");
                    dest += 2;
                    ksprinthexb(dest, val);
                    dest += 2;
                    break;
                case 'w':
                    arg++;
                    kstrcpy(dest, "0x");
                    dest += 2;
                    ksprinthexw(dest, val);
                    dest += 4;
                    break;
                case 'l':
                    arg++;
                    kstrcpy(dest, "0x");
                    dest += 2;
                    ksprinthexd(dest, val);
                    dest += 8;
                    break;
                case '%':
                    *dest++ = '%';
                    break;
                default:
                    *dest++ = '%';
                    *dest++ = *format;
                    break;
            }
        }
        else
        {
            *dest++ = *format;
        }
        format++;
    }
    *dest = 0;
    return destorig;
}

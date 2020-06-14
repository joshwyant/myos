#ifndef __KERNEL_STRING_H__
#define __KERNEL_STRING_H__

extern const char* ksprintdec(char* str, int x);
const char* ksprintf(char* dest, const char* format, ...);

inline static const char* kstrcpy(char* dest, const char* src)
{
    const char* destorig = dest;
    do *dest++ = *src; while (*src++);
    return destorig;
}
inline static int kstrlen(const char* str)
{
    int c = 0;
    while (*str++) c++;
    return c;
}
inline static const char* ksprinthexb(char* str, char c)
{
    str[0] = '0'+((c&0xF0)>>4);
    str[1] = '0'+(c&0xF);
    str[2] = 0;
    if (str[0] > '9') str[0] += 7;
    if (str[1] > '9') str[1] += 7;
    return str;
}

inline static const char* ksprinthexw(char* str, short w)
{
    ksprinthexb(str, w>>8);
    ksprinthexb(str+2, w);
    return str;
}

inline static const char* ksprinthexd(char* str, int d)
{
    ksprinthexw(str, d>>16);
    ksprinthexw(str+4, d);
    return str;
}

inline static const char* kstrcat(char* dest, const char* src)
{
    return kstrcpy(dest+kstrlen(dest), src);
}

inline static int kstrcmp(const char* a, const char* b)
{
    while ((*a && *b) && (*a == *b)) a++, b++;
    return *a - *b;
}

static inline char ktoupper(char c)
{
    if ((c < 'a') || (c > 'z')) return c;
    return c-'a'+'A';
}

static inline char ktolower(char c)
{
    if ((c < 'A') || (c > 'Z')) return c;
    return c-'A'+'a';
}

#endif  // __KERNEL_STRING_H__

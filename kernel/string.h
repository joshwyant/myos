#ifndef __KERNEL_STRING_H__
#define __KERNEL_STRING_H__

#ifdef __cplusplus
#include <stddef.h>

extern "C" {
#endif

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

#ifdef __cplusplus
}  // extern "C"

namespace kernel
{
template<typename char_t>
class _KString
{
public:
    _KString() {}
    _KString(const char_t *str)
    {
        size_t max_short_len = sizeof(storage.s.buffer) / sizeof(char_t) - 1;
        int i;
        for (i = 0; str[i] && i < max_short_len; i++)
        {
            storage.s.buffer[i] = str[i];
            storage.s.length++;
        }
        if (str[i])
        {
            storage.l.is_long = true;
            storage.l.length = 0;
            for (i = 0; str[i]; i++)
            {
                storage.l.length++;
            }
            storage.l.buffer = new char_t[storage.l.length + 1];
            for (i = 0; i < storage.l.length; i++)
            {
                storage.l.buffer[i] = str[i];
            }
        }
    }
    ~_KString()
    {
        if (storage.l.is_long && storage.l.buffer)
        {
            delete storage.l.buffer;
        }
        storage.l.buffer = nullptr;
        storage.l.length = 0;
        storage.l.is_long = false;
    }
    const char_t *c_str()
    {
        return storage.l.is_long ? storage.l.buffer : storage.s.buffer;
    }
    char_t operator[](size_t i) { return storage.l.is_long ? storage.l.buffer[i] : storage.s.buffer[i]; }
protected:

    struct short_string
    {
        char_t buffer[(sizeof(size_t) * 3) / sizeof(char_t) - sizeof(char_t)];
        size_t length : sizeof(char_t) > 8 ? 15 : 7;
        bool is_long : 1;
    };
    struct long_string
    {
        char_t *buffer;
        size_t length : sizeof(size_t) - 1;
        bool is_long : 1;
    };
    union
    {
        struct short_string s;
        struct long_string l;
    } storage;
};
}  // namespace kernel
#endif

#endif  // __KERNEL_STRING_H__

#ifndef __KERNEL_STRING_H__
#define __KERNEL_STRING_H__

#ifdef __cplusplus

#include <stddef.h>
#include <utility>
#include "error.h"
#include "memory.h"

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
// Forward declarations
template<typename CharT>
class KBasicString;

typedef KBasicString<char> KString;
typedef KBasicString<wchar_t> KWString;

template <typename CharT>
class KBasicStringView;

typedef KBasicStringView<char> KStringView;
typedef KBasicStringView<wchar_t> KWStringView;

template<typename CharT>
class KBasicString
{
public:
    KBasicString()
    {
        storage.s.is_long = false;
        storage.s.buffer[0] = 0;
        storage.s.length = 0;
    }
    KBasicString(const KBasicString& other)
        : storage(other.storage)
    {
        if (storage.l.is_long)
        {
            storage.l.buffer = (CharT *)kdupheap(storage.l.buffer);
            if (!storage.l.buffer) [[unlikely]]
            {
                throw OutOfMemoryError();
            }
        }
    }
    KBasicString(KBasicString&& other) noexcept
        : KBasicString()
	{
		swap(*this, other);
	}
    KBasicString& operator=(KBasicString other)
	{
		swap(*this, other);
		return *this;
	}
    friend void swap(KBasicString& a, KBasicString& b)
	{
		using std::swap;
		swap(a.storage, b.storage);
	}
    KBasicString(const KBasicStringView<CharT> str)
    {
        size_t length = 0;
        CharT *buffer;
        int i;
        if (str.len() < short_capacity())
        {
            storage.s.is_long = false;
            storage.s.length = str.len();
            buffer = storage.s.buffer;
        }
        else
        {
            storage.l.is_long = true;
            storage.l.length = str.len();
            storage.l.hash = 0;
            storage.l.buffer = (CharT *)kmalloc(str.len() + 1);
            buffer = storage.l.buffer;
            if (!buffer) [[unlikely]] throw OutOfMemoryError();
        }
        const CharT *other_buffer = str.c_str();
        for (i = 0; i < str.len(); i++)
        {
            buffer[i] = other_buffer[i];
        }
        buffer[str.len()] = 0;
    }
    // Includes null terminator
    static size_t short_capacity() { return sizeof(storage.s.buffer) / sizeof(CharT); }
    static KBasicString preallocated(CharT *buffer, int length)
    {
        if (length < short_capacity())
        {
            KBasicString short_str(buffer);
            kfree(buffer);
            return short_str;
        }
        KBasicString s;
        s.storage.l.is_long = true;
        s.storage.l.length = length;
        s.storage.l.hash = 0;
        s.storage.l.buffer = buffer;
        return s;
    }
    KBasicString(const CharT *str) : KBasicString(KBasicStringView<CharT>(str)) {}
    virtual ~KBasicString()
    {
        if (storage.l.is_long && storage.l.buffer)
        {
            kfree(storage.l.buffer);
        }
        storage.l.buffer = nullptr;
        storage.l.length = 0;
        storage.l.hash = 0;
        storage.l.is_long = false;
    }
    CharT *c_str()
    {
        return const_cast<CharT*>(static_cast<const KBasicString&>(*this).c_str());
    }
    const CharT *c_str() const
    {
        return storage.l.is_long ? storage.l.buffer : storage.s.buffer;
    }
    const size_t len() const
    {
        return storage.l.is_long ? storage.l.length : storage.s.length;
    }
    CharT& operator[](int i)
    {
        return const_cast<CharT&>(static_cast<const KBasicString&>(*this)[i]);
    }
    const CharT& operator[](int i) const
    {
        if (i < 0 || i > len()) [[unlikely]]
        {
            throw OutOfBoundsError();
        }
        return storage.l.is_long ? storage.l.buffer[i] : storage.s.buffer[i];
    }
    bool operator==(const KBasicString& other) const
    {
        if (this == &other) [[unlikely]] return true;
        if (hash() != other.hash()) [[unlikely]] return false;
        auto aPtr = c_str();
        auto bPtr = other.c_str();
        while (*aPtr && *bPtr)
        {
            ++aPtr;
            ++bPtr;
        }
        return *aPtr == *bPtr;
    }
    int hash() const
    {
        int h = storage.l.is_long ? storage.l.hash : 0;
        if (h == 0)
        {
            auto buffer = c_str();
            for (int i = 0; i < len(); i++) {
                h = 31 * h + buffer[i];
            }
            if (storage.l.is_long)
            {
                storage.l.hash = h; // hash is declared mutable, so it's
                                    // okay to set in const function
            }
        }
        return h;
    }
    KBasicString operator+(const KBasicString& other) const
    {
        KBasicString str(*this);
        return str.concat(other);
    }
    KBasicString& concat(const KBasicStringView<CharT>& other)
    {
        size_t short_capacity = sizeof(storage.s.buffer) / sizeof(CharT); // including null terminator
        size_t oldlen = len();
        size_t newlen = oldlen + other.len();
        CharT *buffer;
        if (newlen < short_capacity)
        {
            storage.s.length = newlen;
            buffer = storage.s.buffer;
        }
        else [[likely]]
        {
            if (!storage.l.is_long)
            {
                buffer = (CharT *)kmalloc(newlen + 1);
                if (!buffer) [[unlikely]] throw OutOfMemoryError();
                if (oldlen) [[likely]] kmemcpy(buffer, storage.s.buffer, sizeof(CharT) * oldlen);
                storage.l.buffer = buffer;
                storage.l.is_long = true;
            }
            else
            {
                buffer = (CharT *)krealloc(storage.l.buffer, newlen + 1);
                if (!buffer) [[unlikely]] throw OutOfMemoryError();
                storage.l.buffer = buffer;
            }
            storage.l.length = newlen;
        }
        const CharT *other_buffer = other.c_str();
        for (int i = 0; i < other.len(); i++)
        {
            buffer[oldlen + i] = other_buffer[i];
        }
        buffer[newlen] = 0;
        return *this;
    }
    KBasicString& concat(const CharT *other)
    {
        return concat(KBasicStringView<CharT>(other));
    }
protected:
    struct short_string
    {
        CharT buffer[(sizeof(size_t) * 3) / sizeof(CharT) - 1];
        size_t length : sizeof(CharT) * 8 - 1;
        bool is_long : 1;
    };
    struct long_string
    {
        CharT *buffer;
        mutable int hash;
        size_t length : sizeof(size_t) * 8 - 1;
        bool is_long : 1;
    };
    union
    {
        struct short_string s;
        struct long_string l;
    } storage;
};

template <typename CharT>
class KBasicStringView
{
public:
    KBasicStringView()
        : KBasicStringView(0, "", nullptr) {}
    KBasicStringView(const KBasicStringView& other)
        : KBasicStringView(other.length, other.cstr, other.str) {}
    KBasicStringView(KBasicStringView&& other) noexcept
        : KBasicStringView()
	{
		swap(*this, other);
	}
    KBasicStringView(const KBasicString<CharT> &str)
        : KBasicStringView(0, nullptr, &str) {}
    KBasicStringView(const CharT *str)
        : KBasicStringView(0, str, nullptr)
    {
        while (*str++)
        {
            length++;
        }
    }
    KBasicStringView& operator=(KBasicStringView other)
	{
		swap(*this, other);
		return *this;
	}
    friend void swap(KBasicStringView& a, KBasicStringView &b)
	{
		using std::swap;
		swap(a.cstr, b.cstr);
		swap(a.str, b.str);
		swap(a.length, b.length);
	}
    const CharT *c_str() const
    {
        return cstr ? cstr : str->c_str();
    }
    const size_t len() const
    {
        return str ? str->len() : length;
    }
    const CharT& operator[](int i) const
    {
        if (i < 0 || i > len()) [[unlikely]]
        {
            throw OutOfBoundsError();
        }
        return c_str()[i];
    }
protected:
    const CharT *cstr;
    const KBasicString<CharT> *str;
    size_t length;
private:
    KBasicStringView(size_t length, const CharT *cstr, const KBasicString<CharT> *str)
        : length(length), cstr(cstr), str(str) {}
};  // KBasicStringView
}  // namespace kernel
#endif

#endif  // __KERNEL_STRING_H__

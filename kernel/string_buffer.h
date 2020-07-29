#ifndef __KERNEL_STRING_H__
#define __KERNEL_STRING_H__

#include <stddef.h>
#include "error.h"
#include "memory.h"
#include "pool.h"
#include "string.h"
#include "vector.h"

#ifdef __cplusplus
#include <utility>

namespace kernel
{
template <typename CharT>
class KBasicStringBuffer;

typedef KBasicStringBuffer<char> KStringBuffer;
typedef KBasicStringBuffer<wchar_t> KWStringBuffer;

template <typename CharT>
class KBasicStringBuffer
{
public:
    KBasicStringBuffer()
        : length(0) {}
    KBasicStringBuffer(KBasicStringBuffer&) = delete;
    KBasicStringBuffer& operator=(KBasicStringBuffer) = delete;
    KBasicStringBuffer(KBasicStringBuffer&& other) noexcept
        : KBasicStringBuffer()
    {
        swap(*this, other);
    }
    KBasicStringBuffer& operator=(KBasicStringBuffer&& other) // Move-assignment
    {
        swap(*this, other);
        return *this;
    }
    virtual ~KBasicStringBuffer()
    {
        for (auto& pool_item : pool_items)
        {
            pool.deallocate(pool_item);
        }
    }
    friend void swap(KBasicStringBuffer& a, KBasicStringBuffer& b)
    {
        using std::swap;
        swap(a.buffer, b.buffer);
        swap(a.pool, b.pool);
        swap(a.pool_items, b.pool_items);
        swap(a.length, b.length);
    }
    void append(KBasicStringView<CharT> str)
    {
        buffer.push_back(std::move(str));
    }
    void append(KBasicString<CharT>&& str)
    {
        buffer.push_back(*pool_items.push_back(pool.allocate(std::move(str))));
    }
    void append(CharT value)
    {
        CharT chars[] = { value, 0 };
        KBasicString<CharT> char_str(chars);
        append(std::move(char_str));
    }
    KString to_string() const
    {
        CharT *destBuffer = (CharT*)kmalloc((length + 1) * sizeof(CharT));
        int pos = 0;
        for (auto& str : buffer)
        {
            for (auto i = 0; i < str.len(); i++)
            {
                destBuffer[pos++] = str[i];
            }
        }
        destBuffer[pos] = 0;
        if (length < KBasicString<CharT>::short_capacity())
        {
            return KBasicString<CharT>(destBuffer);
        }
        return KBasicString<CharT>::preallocated(destBuffer, length);
    }
    size_t len() const { return length; }
protected:
    KVector<KBasicStringView<CharT> > buffer;
    MemoryPool<KBasicString<CharT> > pool;
    KVector<KBasicString<CharT>*> pool_items;
    int length;
};  // KBasicStringBuffer
}  // namespace kernel
#endif

#endif  // __KERNEL_STRING_H__

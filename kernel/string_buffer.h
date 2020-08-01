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
class BasicStringBuffer;

typedef BasicStringBuffer<char> StringBuffer;
typedef BasicStringBuffer<wchar_t> WStringBuffer;

template <typename CharT>
class BasicStringBuffer
{
public:
    BasicStringBuffer()
        : length(0) {}
    BasicStringBuffer(BasicStringBuffer&) = delete;
    BasicStringBuffer& operator=(BasicStringBuffer) = delete;
    BasicStringBuffer(BasicStringBuffer&& other) noexcept
        : BasicStringBuffer()
    {
        swap(*this, other);
    }
    BasicStringBuffer& operator=(BasicStringBuffer&& other) // Move-assignment
    {
        swap(*this, other);
        return *this;
    }
    virtual ~BasicStringBuffer()
    {
        for (auto& pool_item : pool_items)
        {
            pool.deallocate(pool_item);
        }
    }
    friend void swap(BasicStringBuffer& a, BasicStringBuffer& b)
    {
        using std::swap;
        swap(a.buffer, b.buffer);
        swap(a.pool, b.pool);
        swap(a.pool_items, b.pool_items);
        swap(a.length, b.length);
    }
    void append(BasicStringView<CharT> str)
    {
        buffer.push_back(std::move(str));
    }
    void append(BasicString<CharT>&& str)
    {
        buffer.push_back(*pool_items.push_back(pool.allocate(std::move(str))));
    }
    void append(CharT value)
    {
        CharT chars[] = { value, 0 };
        BasicString<CharT> char_str(chars);
        append(std::move(char_str));
    }
    String to_string() const
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
        if (length < BasicString<CharT>::short_capacity())
        {
            return BasicString<CharT>(destBuffer);
        }
        return BasicString<CharT>::preallocated(destBuffer, length);
    }
    size_t len() const { return length; }
protected:
    Vector<BasicStringView<CharT> > buffer;
    MemoryPool<BasicString<CharT> > pool;
    Vector<BasicString<CharT>*> pool_items;
    int length;
};  // BasicStringBuffer
}  // namespace kernel
#endif

#endif  // __KERNEL_STRING_H__

#ifndef __KERNEL_BUFFER_H__
#define __KERNEL_BUFFER_H__

#include <stddef.h>
#include "error.h"

#ifdef __cplusplus
#include <utility>
#include <algorithm>

namespace kernel
{
class Buffer
{
public:
    explicit Buffer(size_t capacity = 1024)
        : mBuffer(capacity ? new char[capacity] : nullptr),
          mCapacity(capacity),
          mStart(0),
          mEnd(0),
          mCount(0)
    {
        if (capacity && !mBuffer) throw OutOfMemoryError();
    }
    Buffer(Buffer&) = delete;
    Buffer& operator=(Buffer) = delete;
    Buffer(Buffer&& other) noexcept
        : Buffer()
    {
        swap(*this, other);
    }
    Buffer& operator=(Buffer&& other)
    {
        swap(*this, other);
        return *this;
    }
    ~Buffer()
    {
        delete[] mBuffer;
        mBuffer = nullptr;
        mStart = 0;
        mEnd = 0;
        mCount = 0;
        mCapacity = 0;
    }
    friend void swap(Buffer& a, Buffer& b)
    {
        using std::swap;
        swap(a.mBuffer, b.mBuffer);
        swap(a.mCapacity, b.mCapacity);
        swap(a.mStart, b.mStart);
        swap(a.mEnd, b.mEnd);
        swap(a.mCount, b.mCount);
    }
    size_t capacity() const { return mCapacity; }
    size_t available_capacity() const { return mCapacity - mCount; }
    bool has_capacity() const { return available_capacity() > 0; }
    bool has_bytes() const { return len() > 0; }
    size_t len() const { return mCount; }
    int write(const char *src, int pos, size_t bytes);
    int read(char *dest, int pos, size_t bytes);
protected:
    char *mBuffer;
    size_t mCapacity;
    size_t mStart;
    size_t mEnd;
    size_t mCount;
}; // class Buffer

} // namespace kernel

#endif  // __cplusplus

#endif  // __KERNEL_BUFFER_H__

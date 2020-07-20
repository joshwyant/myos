#ifndef __KERNEL_BUFFER_H__
#define __KERNEL_BUFFER_H__

#ifdef __cplusplus

#include <utility>
#include <stddef.h>
#include <algorithm>
#include "error.h"
#include "memory.h"

namespace kernel
{
class Buffer
{
public:
    Buffer(size_t capacity = 1024)
        : mBuffer(capacity ? (char *)kmalloc(capacity) : nullptr),
          mCapacity(capacity),
          mStart(0),
          mEnd(0),
          mCount(0)
    {
        if (capacity && !mBuffer) throw OutOfMemoryError();
    }
    Buffer(Buffer&) = delete;
    Buffer(Buffer&&) noexcept = delete;
    Buffer& operator=(Buffer) = delete;
    ~Buffer()
    {
        kfree(mBuffer);
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
    size_t capacity() { return mCapacity; }
    size_t available_capacity() { return mCapacity - mCount; }
    bool has_capacity() { return available_capacity() > 0; }
    bool has_bytes() { return len() > 0; }
    size_t len() { return mCount; }
    int write(const char *src, int pos, size_t bytes)
    {
        int bytes_written = 0;
        int chunk = std::min(available_capacity(), capacity() - mEnd, bytes);
        kmemcpy(mBuffer + mEnd, src + pos, chunk);
        bytes_written += chunk;
        bytes -= chunk;
        mCount += chunk;
        pos += chunk;
        mEnd += chunk;
        if (bytes > 0 && pos == capacity())
        {
            mEnd = 0;
            chunk = std::min(available_capacity(), bytes);
            kmemcpy(mBuffer + mEnd, src + pos, chunk);
            bytes_written += chunk;
            mCount += chunk;
            mEnd += chunk;
            // bytes -= chunk;
            // pos += chunk;
        }
        return bytes_written;
    }
    int read(char *dest, int pos, size_t bytes)
    {
        int bytes_written = 0;
        int chunk = std::min(len(), capacity() - mStart, bytes);
        kmemcpy(dest + pos, mBuffer + mStart, chunk);
        bytes_written += chunk;
        bytes -= chunk;
        mCount -= chunk;
        pos += chunk;
        mStart += chunk;
        if (bytes > 0 && pos == capacity())
        {
            mStart = 0;
            chunk = std::min(len(), bytes);
            kmemcpy(mBuffer + mStart, dest + pos, chunk);
            bytes_written += chunk;
            mCount -= chunk;
            mStart += chunk;
            // bytes -= chunk;
            // pos += chunk;
        }
        return bytes_written;
    }
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

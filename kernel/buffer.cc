#include "buffer.h"
#include "memory.h"

namespace kernel
{

int Buffer::write(const char *src, int pos, size_t bytes)
{
    int bytes_written = 0;
    int chunk = std::min(std::min(available_capacity(), capacity() - mEnd), bytes);
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
}  // Buffer::write

int Buffer::read(char *dest, int pos, size_t bytes)
{
    int bytes_written = 0;
    int chunk = std::min(std::min(len(), capacity() - mStart), bytes);
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
}  // Buffer::read

}  // namespace kernel

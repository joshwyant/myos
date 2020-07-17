#ifndef __KERNEL_DEQUE_H__
#define __KERNEL_DEQUE_H__

#ifdef __cplusplus

#include <stddef.h>
#include <utility>
#include "error.h"
#include "memory.h"
#include "vector.h"

namespace kernel
{
template <typename T>
class KDeque
{
public:
    KDeque(size_t capacity = 0)
		: KDeque(capacity, 0, 0, 0) {}
    KDeque(const KDeque& other)
		: KDeque(other.capacity, other.elem_count, other.start, other.end)
	{
		for (int i = 0; i < elem_count; i++)
		{
			this[i] = other[i];
		}
	}
	KDeque(KDeque&& other) noexcept
	{
		swap(*this, other);
	}
	KDeque& operator=(KDeque other)
	{
		swap(*this, other);
		return *this;
	}
    ~KDeque() {
		if (buffer)
		{
			for (auto i = 0; i < elem_count; i++)
			{
				this[i].~T();
			}
			kfree(buffer);
		}
		buffer = nullptr;
		start 
            = end
            = elem_count
            = elem_capacity
            = 0;
    }
	friend void swap(KDeque& a, KDeque &b)
	{
		using std::swap;
		swap(a.buffer, b.buffer);
		swap(a.elem_capacity, b.elem_capacity);
		swap(a.elem_count, b.elem_count);
        swap(a.start, b.start);
        swap(a.end, b.end);
	}
    T& push_back(T);
    T& push_front(T);
    T pop_back() {
        --elem_count;
        end = (end + elem_capacity - 1) % elem_capacity;
        return buffer[end];
    }
    T pop_front() {
        auto val = this[start];
        --elem_count;
        start = (start + 1) % elem_capacity;
        return val;
    }
    T& operator[](size_t index)
    {
        if (index < 0 || index >= elem_count) [[unlikely]]
        {
            throw OutOfBoundsError();
        }
        return buffer[(start + index) % elem_capacity];
    }
    size_t len() { return elem_count; }
    size_t capacity() { return elem_capacity; }
protected:
    T *buffer;
    size_t start;
    size_t end;
    size_t elem_count;
    size_t elem_capacity;
private:
    KDeque(size_t capacity, size_t count, size_t start, size_t end)
		: elem_capacity(capacity),
		  buffer(capacity ? (T *)kmalloc(capacity * sizeof(T)) : nullptr),
          start(start),
          end(end) {}
};  // class KDeque
}  // namespace kernel

#endif  // __cplusplus

#endif  // __KERNEL_DEQUE_H__

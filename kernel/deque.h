#ifndef __KERNEL_DEQUE_H__
#define __KERNEL_DEQUE_H__

#include <stddef.h>
#include "error.h"
#include "memory.h"
#include "sync.h"
#include "vector.h"

#ifdef __cplusplus
#include <utility>

namespace kernel
{
template <typename T>
class Deque
{
public:
    explicit Deque(size_t capacity = 0)
		: Deque(capacity, 0, 0, 0) {}
    Deque(const Deque& other)
		: Deque(other.capacity, other.elem_count, other.start, other.end)
	{
		for (int i = 0; i < elem_count; i++)
		{
			this[i] = other[i];
		}
	}
	Deque(Deque&& other) noexcept
        : Deque()
	{
		swap(*this, other);
	}
	Deque& operator=(Deque other)
	{
		swap(*this, other);
		return *this;
	}
    virtual ~Deque() {
		if (buffer)
		{
			for (auto i = 0; i < elem_count; i++)
			{
				(*this)[i].~T();
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
	friend void swap(Deque& a, Deque &b)
	{
		using std::swap;
		swap(a.buffer, b.buffer);
		swap(a.elem_capacity, b.elem_capacity);
		swap(a.elem_count, b.elem_count);
        swap(a.start, b.start);
        swap(a.end, b.end);
	}
    T& push_back(T value)
    {
        ScopedLock lock(data_lock);
		if (elem_count >= elem_capacity) [[unlikely]]
		{
            expand();
		}
        auto prev_end = end;
        end = (end + 1) % elem_capacity;
        elem_count++;
		// result of assignment is lvalue
		return buffer[prev_end] = std::move(value);
    }
    T& push_front(T value)
    {
        ScopedLock lock(data_lock);
		if (elem_count >= elem_capacity) [[unlikely]]
		{
            expand();
		}
        elem_count++;
        start = (start + elem_capacity - 1) % elem_capacity;
		// result of assignment is lvalue
		return buffer[start] = std::move(value);
    }
    T pop_back() {
        ScopedLock lock(data_lock);
        if (!elem_count) [[unlikely]] throw OutOfBoundsError();
        --elem_count;
        end = (end + elem_capacity - 1) % elem_capacity;
        if (elem_count == 0) start = end;
        return std::move(buffer[end]);
    }
    T pop_front() {
        ScopedLock lock(data_lock);
        if (!elem_count) [[unlikely]] throw OutOfBoundsError();
        auto val = std::move(buffer[start]);
        --elem_count;
        start = (start + 1) % elem_capacity;
        if (elem_count == 0) end = start;
        return std::move(val);
    }
    T& operator[](int index)
    {
        return const_cast<T&>(static_cast<const Deque&>(*this)[index]);
    }
    const T& operator[](int index) const
    {
        ScopedLock lock(data_lock);
        if (index < 0 || index >= elem_count) [[unlikely]]
        {
            throw OutOfBoundsError();
        }
        return buffer[(start + index) % elem_capacity];
    }
	T& top()
	{
        return const_cast<T&>(static_cast<const Deque&>(*this).top());
	}
	T& bottom()
	{
        return const_cast<T&>(static_cast<const Deque&>(*this).bottom());
	}
	const T& top() const
	{
        ScopedLock lock(data_lock);
		if (!elem_count)
			throw OutOfBoundsError();
		return this[elem_count - 1];
	}
	const T& bottom() const
	{
        ScopedLock lock(data_lock);
		if (!elem_count)
			throw OutOfBoundsError();
		return buffer[start];
	}
    size_t len() { ScopedLock lock(data_lock); return elem_count; }
    size_t capacity() { ScopedLock lock(data_lock); return elem_capacity; }
protected:
    T *buffer;
    size_t start;
    size_t end;
    size_t elem_count;
    size_t elem_capacity;
    mutable int data_lock;
private:
    Deque(size_t capacity, size_t count, size_t start, size_t end)
		: elem_capacity(capacity),
          elem_count(count),
		  buffer(capacity ? (T *)kmalloc(capacity * sizeof(T)) : nullptr),
          start(start),
          end(end),
          data_lock(0) {}
    void expand()
    {
        if (elem_count < elem_capacity) [[unlikely]] return;

        auto new_capacity = elem_capacity ? elem_capacity * 2 : 4;
        auto expanded_count = new_capacity - elem_capacity;
        T *new_buffer = (T *)krealloc(buffer, new_capacity * sizeof(T));
        if (!new_buffer) [[unlikely]] throw OutOfMemoryError();
        buffer = new_buffer;
        // Recalculate end position
        if (elem_count > 0 && end <= start)
        {
            auto tail_count = elem_capacity - start;
            // todo: try std::move
            kmemmove(buffer + start + expanded_count, buffer + start, tail_count * sizeof(T));
            kzeromem(buffer + start, expanded_count * sizeof(T));
            if (start != 0)
            {
                start += expanded_count;
            }
        }
        else
        {
            end = (start + elem_count) % new_capacity;
            kzeromem(buffer + end, expanded_count * sizeof(T));
        }
        elem_capacity = new_capacity;
    }
};  // class Deque
}  // namespace kernel

#endif  // __cplusplus

#endif  // __KERNEL_DEQUE_H__

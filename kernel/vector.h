#ifndef __KERNEL_VECTOR_H__
#define __KERNEL_VECTOR_H__

#include <stddef.h>
#include "error.h"
#include "memory.h"

#ifdef __cplusplus
#include <new>
#include <utility>

namespace kernel
{

template <typename T>
class Vector
{
public:
	explicit Vector(size_t capacity = 0)
		: Vector(capacity, 0) {}
	Vector(const Vector& other)
		: Vector(other.capacity, other.elem_count)
	{
		for (int i = 0; i < elem_count; i++)
		{
			buffer[i] = other.buffer[i];
		}
	}
	Vector(Vector&& other) noexcept
		: Vector()
	{
		swap(*this, other);
	}
	Vector& operator=(Vector other)
	{
		swap(*this, other);
		return *this;
	}
	virtual ~Vector()
	{
		if (buffer)
		{
			for (auto i = 0; i < elem_count; i++)
			{
				buffer[i].~T();
			}
			kfree(buffer);
		}
		buffer = nullptr;
		elem_count = 0;
		elem_capacity = 0;
	}
	friend void swap(Vector& a, Vector &b)
	{
		using std::swap;
		swap(a.buffer, b.buffer);
		swap(a.elem_capacity, b.elem_capacity);
		swap(a.elem_count, b.elem_count);
	}
	T& operator[](int index)
	{
		return const_cast<T&>(static_cast<const Vector&>(*this)[index]);
	}
	const T& operator[](int index) const
	{
		if (index < 0 || index >= elem_count) [[unlikely]]
		{
			throw OutOfBoundsError();
		}
		return buffer[index];
	}
	size_t len() const { return elem_count; }
	size_t capacity() const { return elem_capacity; }
	T& push_back(T value)
	{
		if (elem_count >= elem_capacity) [[unlikely]]
		{
			expand();
		}
		// Construct an empty object first -- this is important, even if memory is zeroed out!
		// The constructor ensures the vTable, etc. is there.
		new (&buffer[elem_count]) T();
		// result of assignment is lvalue
		return buffer[elem_count++] = std::move(value);
	}
	T pop_back()
	{
		if (!elem_count)
			throw OutOfBoundsError();
		return std::move(buffer[--elem_count]);
	}
	T& top()
	{
        return const_cast<T&>(static_cast<const Vector&>(*this).top());
	}
	T& bottom()
	{
        return const_cast<T&>(static_cast<const Vector&>(*this).bottom());
	}
	const T& top() const
	{
		if (!elem_count)
			throw OutOfBoundsError();
		return buffer[elem_count - 1];
	}
	const T& bottom() const
	{
		if (!elem_count)
			throw OutOfBoundsError();
		return buffer[0];
	}
	T *begin() { return const_cast<T*>(static_cast<const Vector&>(*this).begin()); }
	T *end() { return const_cast<T*>(static_cast<const Vector&>(*this).end()); }
	const T *begin() const { return buffer; }
	const T *end() const { return buffer + elem_count; }
	void reserve(size_t amount)
	{
		expand(amount);
	}
private:
	Vector(size_t capacity, size_t count)
		: elem_capacity(capacity),
		  buffer(capacity ? (T *)kcalloc(capacity * sizeof(T)) : nullptr),
		  elem_count(count) {}
	void expand(size_t size = 0)
	{
		size_t new_capacity = size ? size : elem_capacity == 0 ? 4 : elem_capacity * 2;
		if (new_capacity < elem_capacity) return;
		T *newbuf = (T *)krealloc(buffer, new_capacity * sizeof(T));
		if (!newbuf)
		{
			throw OutOfMemoryError();
		}
		kzeromem(newbuf + elem_capacity, sizeof(T) * (new_capacity - elem_capacity));
		buffer = newbuf;
		elem_capacity = new_capacity;
	}
	T *buffer;
	size_t elem_count;
	size_t elem_capacity;
};  	// class Vector

}  		// namespace kernel

#endif // __cplusplus

#endif  // __KERNEL_VECTOR_H__

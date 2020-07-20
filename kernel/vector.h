#ifndef __KERNEL_VECTOR_H__
#define __KERNEL_VECTOR_H__

#ifdef __cplusplus

#include <stddef.h>
#include <utility>
#include "error.h"
#include "memory.h"

namespace kernel
{

template <typename T>
class KVector
{
public:
	KVector(size_t capacity = 0)
		: KVector(capacity, 0) {}
	KVector(const KVector& other)
		: KVector(other.capacity, other.elem_count)
	{
		for (int i = 0; i < elem_count; i++)
		{
			buffer[i] = other.buffer[i];
		}
	}
	KVector(KVector&& other) noexcept
	{
		swap(*this, other);
	}
	KVector& operator=(KVector other)
	{
		swap(*this, other);
		return *this;
	}
	~KVector() {
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
	friend void swap(KVector& a, KVector &b)
	{
		using std::swap;
		swap(a.buffer, b.buffer);
		swap(a.elem_capacity, b.elem_capacity);
		swap(a.elem_count, b.elem_count);
	}
	T& operator[](int index)
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
		// result of assignment is lvalue
		return buffer[elem_count++] = std::move(value);
	}
	T *begin() { return buffer; }
	T *end() { return buffer + elem_count; }
	void reserve(size_t amount)
	{
		expand(amount);
	}
private:
	KVector(size_t capacity, size_t count)
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
};  	// class KVector

}  		// namespace kernel

#endif // __cplusplus

#endif  // __KERNEL_VECTOR_H__

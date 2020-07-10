#ifndef __KERNEL_VECTOR_H__
#define __KERNEL_VECTOR_H__

#include <stddef.h>

namespace kernel
{

template <typename T>
class KVector
{
public:
	KVector() { }
	~KVector() {
		if (elem_count && buffer)
		{
			delete buffer;
		}
		buffer = nullptr;
		elem_count = 0;
		elem_capacity = 0;
	}
	bool push_back(const T& value);
	T& operator[](size_t index) { return buffer[index]; }
	size_t len() { return elem_count; }
	size_t capacity() { return elem_capacity; }

protected:
	T *buffer;
	size_t elem_count;
	size_t elem_capacity;
};  	// class KVector

}  		// namespace kernel

#endif  // __KERNEL_VECTOR_H__

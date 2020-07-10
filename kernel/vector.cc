#include "kernel.h"
#include "vector.h"

template <typename T>
bool kernel::KVector::push_back(const T &value)
{
	if (elem_count >= elem_capacity)
	{
		size_t new_capacity = elem_capacity == 0 ? 4 : elem_capacity * 2;
		T newbuf[] = new T[new_capacity];
		if (!newbuf)
		{
			 return false;
		}
		if (elem_count && buffer)
		{
			kmemcpy(newbuf, buffer, elem_count * sizeof(T));
			delete buffer;
		}
		buffer = newbuf;
		elem_capacity = new_capacity;
	}
	this[elem_count++] = value;
	return true;
}

#include "kernel.h"
#include "vector.h"

extern "C"
{
	
void *kvector_append(kernel_vector *v, const void *value)
{
	if (v->elem_count >= v->capacity)
	{
		unsigned new_capacity = v->capacity == 0 ? 4 : v->capacity * 2;
		void *newbuf = kmalloc(new_capacity * v->elem_size);
		if (!newbuf)
		{
			 return 0;
		}
		if (v->buffer)
		{
			kmemcpy(newbuf, v->buffer, v->elem_count * v->elem_size);
			kfree(v->buffer);
		}
		v->buffer = newbuf;
		v->capacity = new_capacity;
	}
	++v->elem_count;
	return kvector_set(v, v->elem_count - 1, value);
}

}  // extern "C"

namespace kernel
{



}  // namespace kernel

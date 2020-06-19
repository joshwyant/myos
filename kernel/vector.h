#ifndef __KERNEL_VECTOR_H__
#define __KERNEL_VECTOR_H__

#ifdef __cplusplus
extern "C" {
#endif

typedef struct kernel_vector
{
	unsigned	elem_size;
	unsigned	elem_count;
	unsigned	capacity;
	void 	*buffer;
} kernel_vector;

inline static void kvector_create(kernel_vector *v, unsigned elem_size)
{
	v->elem_size = elem_size;
	v->elem_count = 0;
	v->capacity = 0;
	v->buffer = 0;
}

inline static void kvector_destroy(kernel_vector *v)
{
	if (v->buffer)
	{
		kfree(v->buffer);
		v->buffer = 0;
	}
	v->elem_size = 0;
	v->elem_count = 0;
	v->capacity = 0;
}

inline static int kvector_count(kernel_vector *v)
{
	return v->elem_count;
}

inline static int kvector_capacity(kernel_vector *v)
{
	return v->capacity;
}

inline static int kvector_elem_size(kernel_vector *v)
{
	return v->elem_size;
}

inline static void *kvector_get(kernel_vector *v, unsigned index)
{
	if (!v->buffer) return 0;
	if (index < 0 || index >= v->elem_count) return 0;
	return (void*)((unsigned char*)v->buffer + index * v->elem_size);
}

inline static void *kvector_set(kernel_vector *v, unsigned index, const void *value)
{
	void *ptr = kvector_get(v, index);
	if (ptr)
	{
		kmemcpy(ptr, value, v->elem_size);
	}
	return ptr;
}

void *kvector_append(kernel_vector *v, const void *value);

#ifdef __cplusplus
}  // extern "C"
#endif

// C++ Wrapper
#ifdef __cplusplus

namespace kernel
{

template <typename T>
class KVector
{
public:
	KVector() { kvector_create(&c_vector, sizeof(T)); }
	~KVector() { kvector_destroy(&c_vector); }
	kernel_vector& get_c_vector() { return c_vector; }
	bool push_back(const T& value) { return kvector_append(&c_vector, &value) ? true : false; }
	T& operator[](unsigned index) { return *(T*)kvector_get(&c_vector, index); }
	unsigned len() { return c_vector.elem_count; }
	unsigned capacity() { return c_vector.capacity; }
	unsigned elem_size() { return c_vector.elem_size; }
	T **buffer() { return &(T*)c_vector.buffer; }

protected:
	kernel_vector c_vector;
};  	// class KVector

}  		// namespace kernel

#endif  // __cplusplus

#endif  // __KERNEL_VECTOR_H__

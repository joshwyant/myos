#ifndef __KERNEL_DEQUE_H__
#define __KERNEL_DEQUE_H__

#include <stddef.h>
#include "vector.h"

namespace kernel
{
template <typename T>
class KDeque
{
public:
    KDeque() {}
    KDeque(size_t capacity) {
        elem_capacity = capacity;
        buffer = new T[capacity];
    }
    ~KDeque() {
        if (elem_count && buffer)
        {
            delete buffer;
        }
        buffer = nullptr;
        start = end = elem_count = 0;
    }
    bool push_back(const T&);
    bool push_front(const T&);
    T& pop_back() {
        --elem_count;
        end = (end + elem_capacity - 1) % elem_capacity;
        return buffer[end];
    }
    T& pop_front() {
        auto i = start;
        --elem_count;
        start = (start + 1) % elem_capacity;
        return this[i];
    }
    T& operator[](size_t index) { return buffer[(start + index) % elem_capacity]; }
    size_t len() { return elem_count; }
    size_t capacity() { return elem_capacity; }
protected:
    T *buffer;
    size_t start;
    size_t end;
    size_t elem_count;
    size_t elem_capacity;
};  // class KDeque
}  // namespace kernel

#endif  // __KERNEL_DEQUE_H__

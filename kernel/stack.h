#ifndef __KERNEL_STACK_H__
#define __KERNEL_STACK_H__

#include "vector.h"

#ifdef __cplusplus
#include <utility>

namespace kernel
{

template <
    typename T,
    typename Container = Vector<T> >
class Stack
    : private Container
{
public:
    Stack() : Container() {}
    Stack(const Stack& other) : Container(other) {}
    Stack(Stack&& other) noexcept : Container(other) {}
    Stack& operator=(Stack other)
    {
        return Container::operator=(*this, other);
    }
    friend void swap(Stack& a, Stack& b)
    {
        Container::swap(a, b);
    }
    using Container::push_back;
    using Container::pop_back;
    using Container::len;
    using Container::top;
}; // class Stack
} // namespace kernel
#endif // __cplusplus
#endif // __KERNEL_STACK_H__

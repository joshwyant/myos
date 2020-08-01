#ifndef __KERNEL_QUEUE_H__
#define __KERNEL_QUEUE_H__

#include "deque.h"

#ifdef __cplusplus

#include <utility>

namespace kernel
{

template <
    typename T,
    typename Container = Deque<T> >
class Queue
    : private Container
{
public:
    Queue() : Container() {}
    Queue(const Queue& other) : Container(other) {}
    Queue(Queue&& other) noexcept : Container(other) {}
    Queue& operator=(Queue other)
    {
        return Container::operator=(*this, other);
    }
    friend void swap(Queue& a, Queue& b)
    {
        Container::swap(a, b);
    }
    using Container::push_back;
    using Container::pop_front;
    using Container::len;
    using Container::bottom;
}; // class Queue
} // namespace kernel
#endif // __cplusplus
#endif // __KERNEL_QUEUE_H__

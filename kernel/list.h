#ifndef __KERNEL_LIST_H__
#define __KERNEL_LIST_H__

#ifdef __cplusplus

#include <memory>
#include "pool.h"

namespace kernel
{

template <typename T>
class List
{
public:
    struct Node;
    List(std::shared_ptr<MemoryPool<Node> > pool = nullptr)
        : mFirst(nullptr),
          mLast(nullptr),
          mPool(pool ? pool : std::make_shared<MemoryPool<Node> >()) {}
    List(List&& other) noexcept
        : List()
    {
        swap(*this, other);
    }
    List(List&) = delete;
    List& operator=(List) = delete;
    friend void swap(List& a, List& b)
    {
        using std::swap;
        swap(a.mFirst, b.mFirst);
        swap(a.mLast, b.mLast);
        swap(a.mPool, b.mPool);
    }
    ~List()
    {
        Node *n = mFirst;
        while (n)
        {
            auto next_node = n->next;
            mPool->deallocate(*n);
            n = next_node;
        }
    }
    struct Node
    {
        T value;
        Node *next;
        Node *prev;
        Node()
            : value(nullptr), next(nullptr), prev(nullptr) {}
        Node(T value, Node *next, Node *prev)
            : value(std::move(value)), next(next), prev(prev) {}
        Node(Node&& other) noexcept
            : Node()
        {
            swap(*this, other);
        }
        ~Node()
        {
            value.~T();
        }
        Node(Node&) = delete;
        Node& operator=(Node) = delete;
        friend void swap(Node& a, Node& b)
        {
            using std::swap;
            swap(a.value, b.value);
            swap(a.next, b.next);
            swap(a.prev, b.prev);
        }
    }; // struct Node
    struct Iterator
    {
        List *list;
        Node *current;
        Iterator()
            : list(nullptr), current(nullptr) {}
        Iterator(List *list, Node *current)
            : list(list), current(current) {}
        Iterator(Iterator&& other) noexcept
            : Iterator()
        {
            swap(*this, other);
        }
        Iterator(Iterator&) = delete;
        Iterator& operator=(Iterator) = delete;

        bool operator==(Iterator& other)
        {
            return current = other.current;
        }
        Iterator& operator++()
        {
            current = current->next;
            return *this;
        }
        Node& operator*()
        {
            return current;
        }
    }; // struct Node
    Iterator begin() { return Iterator(this, mFirst); }
    Iterator end() { return Iterator(this, nullptr); }
    Node& push_back(T value)
    {
        Node *n = mPool->allocate(Node(std::move(value),  nullptr, mLast));
        if (mLast)
        {
            mLast->next = n;
        }
        else
        {
            mFirst = n;
        }
        mLast = n;
        return *n;
    }
    Node& push_front(T value)
    {
        Node *n = mPool->allocate(Node(std::move(value),  mFirst, nullptr));
        if (mFirst)
        {
            mFirst->prev = n;
        }
        else
        {
            mLast = n;
        }
        mFirst = n;
        return *n;
    }
    Node *front() { return mFirst; }
    Node *back() { return mLast; }
    void remove(Node *n)
    {
        if (n->next)
        {
            n->next->prev = n->prev;
        }
        if (n->prev)
        {
            n->prev->next = n->next;
        }
        if (n == mFirst)
        {
            mFirst = n->next;
        }
        if (n == mLast)
        {
            mLast = n->prev;
        }
        mPool->deallocate(*n);
    }
    T pop_front()
    {
        if (mFirst == nullptr) throw OutOfBoundsError();
        auto val = std::move(mFirst->value);
        remove(mFirst);
        return val; // copy elision
    }
    T pop_back()
    {
        if (mLast == nullptr) throw OutOfBoundsError();
        auto val = std::move(mLast->value);
        remove(mLast);
        return val; // copy elision
    }
    void allocator(std::shared_ptr<MemoryPool<Node> > allocator)
    {
        if (mFirst)
            throw InvalidOperationError("Allocator already in use");
        mPool = allocator;
    }
protected:
    Node *mFirst, *mLast;
    std::shared_ptr<MemoryPool<Node> > mPool;
}; // List

} // namespace kernel

#endif // __cplusplus

#endif // __KERNEL_LIST_H__

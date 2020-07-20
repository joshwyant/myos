#ifndef __KERNEL_FORWARD_LIST_H__
#define __KERNEL_FORWARD_LIST_H__

#ifdef __cplusplus

#include <memory>
#include "error.h"
#include "pool.h"

namespace kernel
{

template <typename T>
class ForwardList
{
public:
    struct Node;

    ForwardList(std::shared_ptr<MemoryPool<Node> > pool = nullptr)
        : mFirst(nullptr),
          mLast(nullptr),
          mPool(pool ? pool : std::make_shared<MemoryPool<Node> >()) {}
    ForwardList(ForwardList&& other) noexcept
        : ForwardList()
    {
        swap(*this, other);
    }
    ForwardList(ForwardList&) = delete;
    ForwardList& operator=(ForwardList) = delete;
    friend void swap(ForwardList& a, ForwardList& b)
    {
        using std::swap;
        swap(a.mFirst, b.mFirst);
        swap(a.mLast, b.mLast);
        swap(a.mPool, b.mPool);
    }
    ~ForwardList()
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
        Node& operator++()
        {
            return *this->next;
        }
        Node& operator*()
        {
            return *this;
        }
        Node()
            : value(nullptr), next(nullptr) {}
        Node(T value, Node *next)
            : value(std::move(value)), next(next) {}
        Node(Node&& other) noexcept
            : Node()
        {
            swap(*this, other);
        }
        Node(Node&) = delete;
        ~Node()
        {
            value.~T();
        }
        Node& operator=(Node) = delete;
        friend void swap(Node& a, Node& b)
        {
            using std::swap;
            swap(a.value, b.value);
            swap(a.next, b.next);
        }
    }; // struct Node
    struct Iterator
    {
        ForwardList *list;
        Node *current;
        Iterator()
            : list(nullptr), current(nullptr) {}
        Iterator(ForwardList *list, Node *current)
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
        Node *n = mPool->allocate(Node(std::move(value),  nullptr));
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
        Node *n = mPool->allocate(Node(std::move(value),  mFirst));
        if (!mLast)
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
        if (n == mFirst)
        {
            mFirst = n->next;
            if (n == mLast)
            {
                mLast = nullptr;
            }
            mPool->deallocate(*n);
        }
        else if (n == mLast)
        {
            auto next_to_last = mFirst;
            while (next_to_last)
            {
                if (next_to_last->next == mLast)
                    break;
                next_to_last = next_to_last->next;
            }
            next_to_last->next = nullptr;
            mLast = next_to_last;
            mPool->deallocate(*n);
        }
        else
        {
            auto empty = n->next;
            swap(*n, *n->next);
            // Now n is the next node, and n->next should be deallocated.
            mPool->deallocate(*empty);
        }
    }
    void allocator(std::shared_ptr<MemoryPool<Node> > allocator)
    {
        if (mFirst)
            throw InvalidOperationError("Allocator already in use");
        mPool = allocator;
    }
    T pop_front()
    {
        if (mFirst == nullptr) throw OutOfBoundsError();
        auto val = std::move(mFirst->value);
        remove(mFirst);
        return val; // copy elision
    }
    // No pop_back because it's O(N) for singly linked list
protected:
    Node *mFirst, *mLast;
    std::shared_ptr<MemoryPool<Node> > mPool;
}; // ForwardList

} // namespace kernel

#endif // __cplusplus

#endif // __KERNEL_FORWARD_LIST_H__

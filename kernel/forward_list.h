#ifndef __KERNEL_FORWARD_LIST_H__
#define __KERNEL_FORWARD_LIST_H__

#include "error.h"

#ifdef __cplusplus
#include <memory>
#include <utility>

namespace kernel
{

template <typename T>
class ForwardList
{
public:
    struct Node;

    ForwardList()
        : mFirst(nullptr),
          mLast(nullptr) {}
    ForwardList(ForwardList&& other) noexcept
        : ForwardList()
    {
        swap(*this, other);
    }
    ForwardList(ForwardList&) = delete;
    ForwardList& operator=(ForwardList&& other) // Move assignment
    {
        swap(*this, other);
        return *this;
    }
    friend void swap(ForwardList& a, ForwardList& b)
    {
        using std::swap;
        swap(a.mFirst, b.mFirst);
        swap(a.mLast, b.mLast);
    }
    virtual ~ForwardList()
    {
        Node *n = mFirst;
        while (n)
        {
            auto next_node = n->next;
            delete n;
            n = next_node;
        }
        mFirst = nullptr;
        mLast = nullptr;
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
            : next(nullptr) {}
        Node(T&& value, Node *next)
            : Node()
        {
            using std::swap;
            swap(this->value, value);
            swap(this->next, next);
        }
        Node(Node&& other) noexcept
            : Node()
        {
            swap(*this, other);
        }
        Node(Node&) = delete;
        Node& operator=(Node&) = delete; // Copy-assignment
        virtual ~Node()
        {
            value.~T();
        }
        Node& operator=(Node&& other) // Move-assignment
        {
            swap(*this, other);
            return *this;
        }
        friend void swap(Node& a, Node& b)
        {
            using std::swap;
            swap(a.value, b.value);
            swap(a.next, b.next);
        }
    }; // struct Node
    struct Iterator
    {
        const ForwardList *list;
        Node *current;
        Iterator()
            : list(nullptr), current(nullptr) {}
        Iterator(const ForwardList *list, Node *current)
            : list(list), current(current) {}
        Iterator(Iterator&& other) noexcept
            : Iterator()
        {
            swap(*this, other);
        }
        Iterator(Iterator&) = delete;
        Iterator& operator=(Iterator) = delete; // Copy-assign
        Iterator& operator=(Iterator&& other) // Move-assign
        {
            swap(*this, other);
            return *this;
        }
        friend void swap(Iterator& a, Iterator& b)
        {
            using std::swap;
            swap(a.list, b.list);
            swap(a.current, b.current);
        }

        bool operator!=(Iterator& other)
        {
            return current != other.current;
        }
        Iterator& operator++()
        {
            current = current->next;
            return *this;
        }
        Node& operator*()
        {
            return *current;
        }
    }; // struct Node
    Iterator begin() const { return Iterator(this, mFirst); }
    Iterator end() const { return Iterator(this, nullptr); }
    Node& push_back(T value)
    {
        Node *n = new Node(std::move(value), nullptr);
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
        Node *n = new Node(std::move(value), mFirst);
        if (!mLast)
        {
            mLast = n;
        }
        mFirst = n;
        return *n;
    }
    Node *front() { return const_cast<Node*>(static_cast<const ForwardList&>(*this).front()); }
    Node *back() { return const_cast<Node*>(static_cast<const ForwardList&>(*this).back()); }
    const Node *front() const { return mFirst; }
    const Node *back() const { return mLast; }
    void remove(Node *n)
    {
        if (n == mFirst)
        {
            mFirst = n->next;
            if (n == mLast)
            {
                mLast = nullptr;
            }
            delete n;
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
            delete n;
        }
        else
        {
            auto empty = n->next;
            swap(*n, *n->next);
            // Now n is the next node, and n->next should be deallocated.
            delete empty;
        }
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
}; // ForwardList

} // namespace kernel

#endif // __cplusplus

#endif // __KERNEL_FORWARD_LIST_H__

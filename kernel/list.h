#ifndef __KERNEL_LIST_H__
#define __KERNEL_LIST_H__

#ifdef __cplusplus

#include <memory>

namespace kernel
{

template <typename T>
class List
{
public:
    struct Node;
    List()
        : mFirst(nullptr),
          mLast(nullptr) {}
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
    virtual ~List()
    {
        Node *n = mFirst;
        while (n)
        {
            auto next_node = n->next;
            delete n;
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
        virtual ~Node()
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
        Node *n = new Node(std::move(value), nullptr, mLast);
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
        Node *n = new Node(std::move(value), mFirst, nullptr);
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
    Node *front() { return const_cast<Node*>(static_cast<const List&>(*this).front()); }
    Node *back() { return const_cast<Node*>(static_cast<const List&>(*this).back()); }
    const Node *front() const { return mFirst; }
    const Node *back() const { return mLast; }
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
        delete n;
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
protected:
    Node *mFirst, *mLast;
}; // List

} // namespace kernel

#endif // __cplusplus

#endif // __KERNEL_LIST_H__

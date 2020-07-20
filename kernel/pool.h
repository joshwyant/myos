#ifndef __KERNEL_POOL_H__
#define __KERNEL_POOL_H__

#ifdef __cplusplus

#include <utility>
#include "vector.h"

namespace kernel
{
template <typename T>
class Allocator
{
public:
virtual T *allocate(T value) = 0;
virtual void deallocate(T *value) = 0;
};

template <typename T>
class HeapAllocator
    : Allocator<T>
{
public:
T *allocate(T value) override
{
    using std::swap;
    T *ptr = kcalloc(sizeof(T));
    swap(*ptr, value);
    return ptr;
}
void deallocate(T *value) override
{
    value->~T();
    kfree(value);
}
};

template <typename T>
class MemoryPool
    : Allocator<T>
{
public:
    MemoryPool(size_t capacity = 0)
        : first_free(nullptr),
          last_free(nullptr)
    {
        if (capacity)
        {
            list.reserve(capacity);
        }
    };
    MemoryPool(MemoryPool&& other) noexcept
        : MemoryPool()
    {
        swap(*this, other);
    }
    // Disallow copy and assign
    MemoryPool(const MemoryPool&) = delete;
    MemoryPool& operator=(MemoryPool) = delete;

    friend void swap(MemoryPool& a, MemoryPool &b)
	{
		using std::swap;
        swap(a.first_free, b.first_free);
        swap(a.last_free, b.last_free);
        swap(a.list, b.list);
    }

    T *allocate(T value) override
    {
        if (first_free)
        {
            // Remove the slot from the free list
            auto item = first_free;
            remove_free_item(item);
            // Place the value in the slot and return it.
            return &(item->value = std::move(value));
        }
        else
        {
            // Just add the item to the vector.
            Item item = { std::move(value) };
            return &list.push_back(std::move(item)).value;
        }
    }
    void deallocate(T *value) override
    {
        // Deconstruct the value item
        value->~T();
        // Add the slot to the free list
        auto item = (Item *)value;
        add_free_item(item);
    }
    void reserve(size_t amount)
    {
        list.reserve(amount);
    }
protected:
    union Item;
    struct FreeListNode
    {
        Item *prev;
        Item *next;
    };
    union Item
    {
        T value;
        FreeListNode node;
    };
    Item *first_free;
    Item *last_free;
    KVector<Item> list;
    void remove_free_item(Item *item)
    {
        if (item == first_free)
        {
            first_free = item->node.next;
        }
        if (item == last_free)
        {
            last_free = item->node.prev;
        }
        if (item->node.prev)
        {
            item->node.prev->node.next = item->node.next;
        }
        if (item->node.next)
        {
            item->node.next->node.prev = item->node.prev;
        }
    }
    void add_free_item(Item *item)
    {
        if (!first_free)
        {
            first_free = item;
        }
        item->node.prev = last_free;
        item->node.next = nullptr;
        if (last_free)
        {
            last_free->node.next = item;
        }
        last_free = item;
    }
};
}

#endif  // __cplusplus

#endif  // __KERNEL_POOL_H__

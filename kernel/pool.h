#ifndef __KERNEL_POOL_H__
#define __KERNEL_POOL_H__

#ifdef __cplusplus

#include <utility>
#include "vector.h"

namespace kernel
{
template <typename T>
class MemoryPool
{
public:
    T& allocate(T value)
    {
        if (first_free)
        {
            // Remove the slot from the free list
            auto item = *first_free;
            remove(item);
            // Place the value in the slot and return it.
            return item = std::move(value);
        }
        else
        {
            // Just add the item to the vector.
            return list.push_back(std::move(value));
        }
    }
    void deallocate(T& value)
    {
        // Deconstruct the value item
        value.~T();
        // Add the slot to the free list
        auto item = (Item *)&value;
        add(item->node);
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
    Item *first_free = nullptr;
    Item *last_free = nullptr;
    KVector<Item> list;
    void remove(Item& node)
    {
        if (&node == first_free)
        {
            first_free = node.node.next;
        }
        if (&node == last_free)
        {
            last_free = node.node.prev;
        }
        if (node.node.prev)
        {
            node.node.prev.next = node.node.next;
        }
        if (node.node.next)
        {
            node.node.next.prev = node.node.prev;
        }
    }
    void add(Item& node)
    {
        if (!first_free)
        {
            first_free = &node;
        }
        node.node.prev = last_free;
        node.node.next = nullptr;
        if (last_free)
        {
            last_free->next = &node;
            last_free = &node;
        }
    }
};
}

#endif  // __cplusplus

#endif  // __KERNEL_POOL_H__

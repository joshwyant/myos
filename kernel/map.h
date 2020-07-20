#ifndef __KERNEL_MAP_H__
#define __KERNEL_MAP_H__

#ifdef __cplusplus

#include <memory>
#include <utility>
#include "forward_list.h"
#include "hash.h"
#include "pool.h"
#include "vector.h"

namespace kernel
{

template <typename TKey, typename TValue>
class Map
{
public:
    Map(Map&) = delete;
    Map& operator=(Map&) = delete;
    struct KeyValuePair
    {
        TKey key;
        TValue value;
        KeyValuePair() {}
        KeyValuePair(TKey key, TValue value)
            : key(key), value(value) {}
        KeyValuePair(KeyValuePair& other)
            : KeyValuePair(other.key, other.value) {}
        KeyValuePair(KeyValuePair&& other) noexcept
            : KeyValuePair()
        {
            swap(*this, other);
        }
        KeyValuePair& operator=(KeyValuePair other)
        {
            swap(*this, other);
            return *this;
        }
        friend void swap(KeyValuePair& a, KeyValuePair& b)
        {
            using std::swap;
            swap(a.key, b.key);
            swap(a.value, b.value);
        }
    };
    virtual TValue& operator[](TKey&) = 0;
    virtual void insert(TKey key, TValue value) = 0;
}; // class Map

template <
    typename TKey,
    typename TValue,
    typename FHash = Hash<TKey>,
    typename FEqual = Equal<TKey> >
class UnorderedMap
    : Map<TKey, TValue>
{
public:
    UnorderedMap(int capacity = 0)
        : mCapacity(capacity),
          mCount(0),
          mNodePool(
            std::make_shared<
                MemoryPool<
                    typename ForwardList<
                        typename Map<TKey, TValue>
                            ::KeyValuePair
                    >::Node
                > 
            >()
          )
    {
        if (capacity)
        {
            mBuckets.reserve(capacity);
            mNodePool->reserve(capacity);
            for (auto& bucket: mBuckets)
            {
                bucket.allocator(mNodePool);
            }
        }
    }
    UnorderedMap(UnorderedMap&& other) noexcept
        : UnorderedMap()
    {
        swap(*this, other);
    }
    UnorderedMap(UnorderedMap&) = delete;
    UnorderedMap& operator=(UnorderedMap) = delete;
    friend void swap(UnorderedMap& a, UnorderedMap& b)
    {
        using std::swap;
        swap(a.mBuckets, b.mBuckets);
        swap(a.mNodePool, b.mNodePool);
        swap(a.mCapacity, b.mCapacity);
        swap(a.mCount, b.mCount);
    }
    ~UnorderedMap()
    {
        // TODO
    }
    TValue& operator[](TKey& key) override
    {
        int hash = FHash(key);
        int bucket = hash % mCapacity;
        for (auto& node : mBuckets[bucket])
        {
            if ((FHash(node.value.key) == hash) && FEqual(node.value.key, key)) [[likely]]
            {
                return node.value.value;
            }
        }
        throw NotFoundError();
    }
    void insert(TKey key, TValue value) override
    {
        if (mCapacity >= mCount) [[unlikely]]
        {
            expand();
        }
        int hash = FHash(key);
        int bucket = hash % mCapacity;
        mBuckets[bucket].push_back(typename Map<TKey, TValue>::KeyValuePair(std::move(key), std::move(value)));
    }
    void reserve(size_t amount)
    {
        expand(amount);
    }
protected:
    void expand(size_t size = 0)
    {
        if (size != 0 && size <= mCapacity) [[unlikely]] return;
        UnorderedMap newMap(size ? size : mCapacity ? 4 : mCapacity);
        for (auto& bucket: mBuckets)
        {
            for (auto& node : bucket)
            {
                newMap.insert(std::move(node.value.key), std::move(node.value.value));
            }
        }
        swap(*this, newMap);
    }
private:
    size_t mCapacity;
    size_t mCount;
    KVector<ForwardList<typename Map<TKey, TValue>::KeyValuePair> > mBuckets;
    std::shared_ptr<MemoryPool<typename ForwardList<typename Map<TKey, TValue>::KeyValuePair>::Node> > mNodePool;
}; // class UnorderedMap

} // namespace kernel

#endif // __cplusplus

#endif // __KERNEL_MAP_H__

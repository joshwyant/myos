#ifndef __KERNEL_MAP_H__
#define __KERNEL_MAP_H__

#include "forward_list.h"
#include "hash.h"
#include "vector.h"

#ifdef __cplusplus
#include <memory>
#include <utility>

namespace kernel
{

template <typename TKey, typename TValue>
class Map
{
public:
    Map(Map&) = delete;
    Map& operator=(Map&) = delete;
    virtual ~Map() {}
    struct KeyValuePair
    {
        TKey key;
        TValue value;
        KeyValuePair() {}
        KeyValuePair(TKey key, TValue value)
            : key(std::move(key)), value(std::move(value)) {}
        KeyValuePair(KeyValuePair& other)
            : KeyValuePair(other.key, other.value) {}
        KeyValuePair(KeyValuePair&& other) noexcept
            : KeyValuePair()
        {
            swap(*this, other);
        }
        virtual ~KeyValuePair() {}
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
    virtual KeyValuePair *find(const TKey& key) const = 0;
    virtual bool contains (const TKey& key) const = 0;
    virtual TValue& operator[](const TKey&) = 0;
    virtual const TValue& operator[](const TKey&) const = 0;
    virtual KeyValuePair& insert(TKey key, TValue value) = 0;
    virtual KeyValuePair& set(TKey key, TValue value) = 0;
protected:
    Map() {}
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
    explicit UnorderedMap(int capacity = 0)
        : mCapacity(capacity),
          mCount(0)
    {
        if (capacity)
        {
            mBuckets.reserve(capacity);
            for (int i = 0; i < capacity; i++)
            {
                mBuckets.push_back(ForwardList<typename Map<TKey, TValue>::KeyValuePair>());
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
        swap(a.mCapacity, b.mCapacity);
        swap(a.mCount, b.mCount);
    }
    ~UnorderedMap() override
    {
        mCapacity = 0;
        mCount = 0;
    }
    typename Map<TKey, TValue>::KeyValuePair *find(const TKey& key) const override
    {
        if (mCapacity == 0 || mCount == 0) [[unlikely]] return nullptr;
        FHash hash;
        FEqual equal;
        int h = hash(key);
        int bucket = h % mCapacity;
        for (auto& node : mBuckets[bucket])
        {
            if ((hash(node.value.key) == h) && equal(node.value.key, key)) [[likely]]
            {
                return &node.value;
            }
        }
        return nullptr;
    }
    bool contains(const TKey& key) const override
    {
        return find(key) != nullptr;
    }
    TValue& operator[](const TKey& key) override
    {
        return const_cast<TValue&>(static_cast<const UnorderedMap&>(*this)[key]);
    }
    const TValue& operator[](const TKey& key) const override
    {
        auto pair = find(key);
        if (pair != nullptr)
        {
            return pair->value;
        }
        throw NotFoundError();
    }
    typename Map<TKey, TValue>::KeyValuePair& insert(TKey key, TValue value) override
    {
        if (contains(key)) throw DuplicateKeyError();
        FHash hash;
        if (mCount >= mCapacity) [[unlikely]]
        {
            expand();
        }
        int h = hash(key);
        int bucket = h % mCapacity;
        auto& pair = mBuckets[bucket].push_back(typename Map<TKey, TValue>::KeyValuePair(std::move(key), std::move(value))).value;
        mCount++;
        return pair;
    }
    typename Map<TKey, TValue>::KeyValuePair& set(TKey key, TValue value) override
    {
        auto pair = find(key);
        if (pair != nullptr)
        {
            pair->value = std::move(value);
            return *pair;
        }
        else
        {
            return insert(std::move(key), std::move(value));
        }
    }
    void reserve(size_t amount)
    {
        expand(amount);
    }
protected:
    void expand(size_t size = 0)
    {
        if (size != 0 && size <= mCapacity) [[unlikely]] return;
        UnorderedMap newMap(size ? size : mCapacity ? mCapacity * 2 : 4);
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
}; // class UnorderedMap

} // namespace kernel

#endif // __cplusplus

#endif // __KERNEL_MAP_H__

#ifndef __KERNEL_HASH_H__
#define __KERNEL_HASH_H__

#ifdef __cplusplus

#include "string.h"

namespace kernel
{

template <typename T>
class Hash
{
public:
int operator()(const T& value) { return 0; }
}; // class Hash

template<typename CharT>
class Hash<KBasicString<CharT> >
{
public:
int operator()(const KBasicString<CharT>& value)
{
    return value.hash();
}
}; // Hash<KBasicString<CharT> >

template<>
class Hash<int>
{
public:
int operator()(const int& value)
{
    return value;
}
}; // Hash<int>

template<>
class Hash<unsigned>
{
public:
int operator()(const unsigned& value)
{
    return value;
}
}; // Hash<unsigned>

template<>
class Hash<char>
{
public:
int operator()(const char& value)
{
    return value;
}
}; // Hash<char>

template<>
class Hash<unsigned char>
{
public:
int operator()(const unsigned char& value)
{
    return value;
}
}; // Hash<unsigned char>

template<>
class Hash<short>
{
public:
int operator()(const short& value)
{
    return value;
}
}; // Hash<short>

template<>
class Hash<unsigned short>
{
public:
int operator()(const unsigned short& value)
{
    return value;
}
}; // Hash<unsigned short>

template<typename T>
class Hash<T*>
{
public:
int operator()(const T*& value)
{
    return (unsigned)value;
}
}; // Hash<T*>

template <typename T>
class Equal
{
public:
bool operator()(const T& a, const T&b) { return &a == &b || a == b; }
}; // class Equal

}  // namespace kernel

#endif  // __cplusplus

#endif  // __KERNEL_HASH_H__

#ifndef __KERNEL_ERROR_H__
#define __KERNEL_ERROR_H__

#ifdef __cplusplus
#include "string.h"

namespace kernel
{
class Error
{
public:
    Error() : msg("") {}
    Error(const char *msg) : msg(msg) {}
    virtual const char *what() { return msg; }
private:
    const char *msg;
};  // class Error

class OutOfMemoryError : public Error
{
public:
    OutOfMemoryError() : Error("Out of memory") {}
};  // class OutOfMemoryError

class OutOfBoundsError : public Error
{
public:
    OutOfBoundsError() : Error("Out of bounds") {}
};  // class OutOfBoundsError

class InvalidOperationError : public Error
{
public:
    InvalidOperationError(const char *msg) : Error(msg) {}
};  // class InvalidOperationError

class NotFoundError : public Error
{
public:
    NotFoundError() : Error("Not found") {}
    NotFoundError(const char *msg) : Error(msg) {}
};  // class NotFoundError
} // namespace kernel
#endif  // __cplusplus
#endif  // __KERNEL_ERROR_H__

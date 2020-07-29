#ifndef __KERNEL_ERROR_H__
#define __KERNEL_ERROR_H__

#ifdef __cplusplus

namespace kernel
{
class Error
{
public:
    Error() : msg("") {}
    explicit Error(const char *msg) : msg(msg) {}
    virtual const char *what() const { return msg; }
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

class ArgumentError : public Error
{
public:
    ArgumentError() : Error("Invalid argument") {}
    ArgumentError(const char *msg) : Error(msg) {}
};  // class ArgumentError

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

class NotImplementedError : public Error
{
public:
    NotImplementedError() : Error("Not implemented") {}
    NotImplementedError(const char *msg) : Error(msg) {}
};  // class NotImplementedError

class EndOfFileError : public Error
{
public:
    EndOfFileError() : Error("End of file") {}
    EndOfFileError(const char *msg) : Error(msg) {}
};  // class EndOfFileError

class DuplicateKeyError : public Error
{
public:
    DuplicateKeyError() : Error("Duplicate Key") {}
    DuplicateKeyError(const char *msg) : Error(msg) {}
};  // class DuplicateKeyError
} // namespace kernel
#endif  // __cplusplus
#endif  // __KERNEL_ERROR_H__

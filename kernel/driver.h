#ifndef __KERNEL_DRIVER_H__
#define __KERNEL_DRIVER_H__

#include "string.h"

#ifdef __cplusplus
#include <utility>

namespace kernel
{
class Driver
{
public:
    Driver(KString device_name)
        : device_name(std::move(device_name)) {}
    Driver(const Driver&) = delete;
    Driver(Driver&& other) noexcept
        : Driver()
    {
        swap(*this, other);
    }
    Driver& operator=(Driver&& other)
    {
        swap(*this, other);
        return *this;
    }
    friend void swap(Driver& a, Driver& b)
    {
        using std::swap;
        swap(a.device_name, b.device_name);
    }
    const KString& name() const { return device_name; }
    virtual void start() {}
    virtual ~Driver() {}
protected:
    Driver() {}
private:
    KString device_name;
}; // class Driver
} // namespace kernel
#endif // __cplusplus
#endif // __KERNEL_DRIVER_H__

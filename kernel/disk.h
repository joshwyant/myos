#ifndef __disk_h__
#define __disk_h__

#ifdef __cplusplus

#include <utility>
#include "driver.h"
#include "io.h"
#include "string.h"

namespace kernel
{
class DiskDriver
    : public Driver
{
public:
    DiskDriver(KString name = "hda") : Driver(name) {}
    virtual void read_sectors(char *buffer, int sector_count, int sector_num) = 0;
    virtual void write_sectors(const char *buffer, int sector_count, int sector_num) = 0;
    friend void swap(DiskDriver& a, DiskDriver& b)
    {
        using std::swap;
        swap(static_cast<Driver&>(a), static_cast<Driver&>(b));
    }
    virtual ~DiskDriver() {}
}; // DiskDriver

class PIODiskDriver : public DiskDriver
{
public:
    PIODiskDriver(KString device_name = "hda", int controller = 0, int drive = 0)
        : base(controller ? 0x170 : 0x1F0),
          controller(controller),
          drive(drive),
          disklock(0),
          DiskDriver(device_name) {}
    PIODiskDriver(PIODiskDriver&) = delete;
    PIODiskDriver(PIODiskDriver&& other) noexcept
        : PIODiskDriver()
    {
        swap(*this, other);
    }
    PIODiskDriver& operator=(PIODiskDriver&& other) // move assign
    {
        swap(*this, other);
        return *this;
    }
    ~PIODiskDriver() override {}
    friend void swap(PIODiskDriver& a, PIODiskDriver& b)
    {
        using std::swap;
        swap(static_cast<DiskDriver&>(a), static_cast<DiskDriver&>(b));
        swap(a.base, b.base);
        swap(a.controller, b.controller);
        swap(a.drive, b.drive);
        swap(a.disklock, b.disklock);
    }
    void read_sectors(char *buffer, int sector_count, int sector_num) override;
    void write_sectors(const char *buffer, int sector_count, int sector_num) override;
private:
    int base;
    int controller;
    int drive;
    int disklock;
}; // PIODiskDriver
} // namespace kernel
#endif // __cplusplus
#endif // __disk_h__

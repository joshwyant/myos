#ifndef __KERNEL_FS_H__
#define __KERNEL_FS_H__

#include <sys/stat.h>
#include <stddef.h>
#include "driver.h"
#include "string.h"

#ifdef __cplusplus
#include <memory>
#include <utility>

extern "C" {
#endif

#ifdef __cplusplus
}  // extern "C"

namespace kernel
{
class FileSystemNode
{
public:
    virtual void stat(struct stat *stat) = 0;
    virtual KString name() = 0;
}; // class FileSystemNode

class FileSystemDirectoryNode
    : public FileSystemNode
{
public:
    //virtual FileSystemNode
}; // class FileSystemDirectoryNode

class File
{
public:
    virtual ~File() {}
    virtual bool seek(unsigned pos) = 0;
    virtual unsigned read(char *buffer, unsigned bytes) = 0;
    virtual char getch() = 0;
    virtual char peekc() = 0;
    virtual bool eof() = 0;
    virtual void close() = 0;
    template <typename O> void read(O& o)
    {
        O obj;
        if (read((char*)(void*)&obj, sizeof(O)) != sizeof(O)) throw EndOfFileError();
        o = obj;
    }
    template<typename O> O read()
    {
        O o;
        read(o);
        return std::move(o);
    }
}; // class File

class FileSystemDriver
    : public Driver
{
public:
    FileSystemDriver(KString device_name = "hda1")
        : Driver(device_name) {}
    virtual ~FileSystemDriver() {}
    virtual bool read_file(const char* filename, void* buffer) = 0;
    virtual unsigned int file_size(const char* filename) = 0;
    virtual bool file_exists(const char* filename) = 0;
    virtual bool directory_exists(const char* dirname) = 0;
    virtual bool chdir(const char* dir) = 0;
    virtual const char* current_directory() = 0;
    virtual std::unique_ptr<File> file_open(const char *filename) = 0;
}; // class FileSystemDriver
}  // namespace kernel
#endif  // __cplusplus


#endif  // __KERNEL_FS_H__

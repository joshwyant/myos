#ifndef __KERNEL_FS_H__
#define __KERNEL_FS_H__

#include <sys/stat.h>
#include <stddef.h>
#include "driver.h"
#include "map.h"
#include "queue.h"
#include "stack.h"
#include "string.h"
#include "vector.h"

#ifdef __cplusplus
#include <memory>
#include <utility>

extern "C" {
#endif

// Forward declaration for PipeFile
struct Process;

#ifdef __cplusplus
}  // extern "C"

namespace kernel
{
class FileSystemNode;
class FileSystemDirectoryNode;
class File;
class FileSystemDriver;
class MemoryFile;
class VirtualFileEntry;
class ScratchVirtualFileEntry;
class PipeFile;
class VirtualDirectoryEntry;
class FileSystem;

class FileSystemNode
{
public:
    virtual void stat(struct stat *stat) = 0;
    virtual String name() = 0;
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
    File()
        : _id(-1) {}
    virtual ~File() {}
    virtual bool seek(unsigned pos) = 0;
    virtual unsigned read(char *buffer, unsigned bytes) = 0;
    virtual char getch() = 0;
    virtual char peekc() = 0;
    virtual bool eof() = 0;
    virtual void close() = 0;
    virtual void write(const char *buffer, unsigned bytes) = 0;
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
    void set_id(int id) { _id = id; }
    int get_id() { return _id; }
private:
    int _id;
}; // class File

class FileSystemDriver
    : public Driver
{
public:
    FileSystemDriver(String device_name = "hda1")
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

class MemoryFile
    : public File
{
public:
    MemoryFile(std::shared_ptr<Vector<char> > data)
        : data(data),
          pos(0) {}
    MemoryFile()
        : MemoryFile(std::make_shared<Vector<char> >()) {}
    ~MemoryFile() override {}
    bool seek(unsigned pos) override
    {
        if (pos > data->len()) return false;
        this->pos = pos;
        return true;
    }
    unsigned read(char *buffer, unsigned bytes) override
    {
        unsigned bytes_read = 0;
        while (pos < data->len() && bytes--)
        {
            *(buffer++) = (*data)[pos++];
            bytes_read++;
        }
        return bytes_read;
    }
    char getch() override { return (*data)[pos++]; }
    char peekc() override { return (*data)[pos]; }
    bool eof() override { return pos >= data->len(); }
    void close() override {}
    void write(const char *buffer, unsigned bytes) override
    {
        while (bytes--)
        {
            if (pos < data->len())
            {
                (*data)[pos++] = *(buffer++);
            }
            else
            {
                data->push_back(*(buffer++));
                pos++;
            }
        }
    }
private:
    std::shared_ptr<Vector<char> > data;
    unsigned pos;
}; // class MemoryFile

class PipeFile
    : public File
{
public:
    PipeFile(std::shared_ptr<Queue<char> > data)
        : data(data),
          closed(false),
          waiting_process(nullptr) {}
    PipeFile()
        : PipeFile(std::make_shared<Queue<char> >()) {}
    ~PipeFile() override { pipe_close(); }
    bool seek(unsigned pos) override;
    unsigned read(char *buffer, unsigned bytes) override;
    char getch() override { check_eof(); return data->pop_front(); }
    char peekc() override { check_eof(); return data->bottom(); }
    bool eof() override { return closed || (data->len() == 0 && !closed); }
    void close() override { pipe_close(); }
    void write(const char *buffer, unsigned bytes) override;
private:
    void pipe_close(); // Must be non-virtual for destructor
    void wait_for_data();
    inline void unblock_waiting_process();
    inline void check_eof()
    {
        if (eof()) [[unlikely]]
        {
            throw EndOfFileError();
        }
    }
    std::shared_ptr<Queue<char> > data;
    bool closed;
    Process *waiting_process;
}; // class MemoryFile

class VirtualFileEntry
{
public:
    virtual std::unique_ptr<File> open() = 0;
private:
}; // class VirtualFile

class ScratchVirtualFileEntry
    : public VirtualFileEntry
{
public:
    ScratchVirtualFileEntry()
        : ScratchVirtualFileEntry(std::make_shared<Vector<char> >()) {}
    ScratchVirtualFileEntry(std::shared_ptr<Vector<char> > data)
        : data(data) {}
    std::unique_ptr<File> open() override
    {
        // TODO: Exclusive mode
        return std::make_unique<MemoryFile>(data);
    }
private:
    std::shared_ptr<Vector<char> > data;
}; // class VirtualScratchFile

class VirtualDirectoryEntry
{
public:
    bool contains_dir(String dir) { return sub_dirs.contains(dir); }
    bool contains_file(String file) { return virtual_files.contains(file); }
    std::unique_ptr<VirtualFileEntry>& get_file(String file) { return virtual_files[file]; }
    std::unique_ptr<VirtualDirectoryEntry>& get_dir(String dir) { return sub_dirs[dir]; }
    std::unique_ptr<VirtualDirectoryEntry>& mkdir(String dir) { return sub_dirs.insert(dir, std::make_unique<VirtualDirectoryEntry>()).value; }
    std::unique_ptr<VirtualFileEntry>& create_file(String file, std::unique_ptr<VirtualFileEntry>&& entry) { return virtual_files.insert(file, std::move(entry)).value; }
    std::unique_ptr<VirtualFileEntry>& create_file(String file) { return create_file(file, std::make_unique<ScratchVirtualFileEntry>()); }
private:
    UnorderedMap<String, std::unique_ptr<VirtualDirectoryEntry> > sub_dirs;
    UnorderedMap<String, std::unique_ptr<VirtualFileEntry> > virtual_files;
};

class FileSystem
{
public:
    FileSystem()
        : _virtual_root(std::make_unique<VirtualDirectoryEntry>()),
          _next_file_id(3),
          _add_descriptor_lock(0),
          _free_descriptor_lock(0) {}
    void mount(String, std::shared_ptr<FileSystemDriver>);
    std::unique_ptr<File> open(String);
    std::unique_ptr<VirtualDirectoryEntry>& virtual_root() { return _virtual_root; }
    int open_descriptor(String);
    void close_descriptor(int id);
    std::unique_ptr<File>& get_by_descriptor(int id) { return _file_table[id]; }
private:
    UnorderedMap<String, std::shared_ptr<FileSystemDriver> > _mount_points;
    std::unique_ptr<VirtualDirectoryEntry> _virtual_root;
    Vector<std::unique_ptr<File> > _file_table;
    Stack<int> _available_file_ids;
    int _next_file_id;
    int _add_descriptor_lock;
    int _free_descriptor_lock;
};
}  // namespace kernel
#endif  // __cplusplus


#endif  // __KERNEL_FS_H__

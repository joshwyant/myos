#include "process.h"
#include "fs.h"
#include "string.h"
#include "sync.h"

using namespace kernel;

void FileSystem::mount(String dir, std::shared_ptr<FileSystemDriver> driver)
{
    if (dir.len() <= 0 || dir[0] != '/') throw ArgumentError();
    if (dir.len() > 1 && dir[dir.len()-1] == '/')
    {
        dir = dir.substring(0, dir.len()-1);
    }
    _mount_points.insert(dir, driver);
}

std::unique_ptr<File> FileSystem::open(String file)
{
    std::shared_ptr<FileSystemDriver> driver;
    String driver_mount_point;
    auto vdir = _virtual_root.get();
    int dir_end = 0, mount_dir_end = 0;
    for (auto i = 0; i < file.len(); i++)
    {
        if (file[i] == '/')
        {
            // Copy the current directory to a string.
            auto dir_name = file.substring(dir_end, i);
            dir_end = i + 1; // inclusive of final slash
            auto current_dir = i == 0 ? String("/") : file.substring(0, i);
            auto driver_pair = _mount_points.find(current_dir);
            if (driver_pair)
            {
                driver_mount_point = current_dir;
                driver = driver_pair->value;
                mount_dir_end = i; // excludes final slash
            }
            if (dir_name.len() && vdir && vdir->contains_dir(dir_name))
            {
                vdir = vdir->get_dir(dir_name).get();
            }
            else if (i > 0)
            {
                vdir = nullptr;
            }
            
        }
    }
    auto file_name = file.substring(dir_end, file.len());
    if (vdir && vdir->contains_file(file_name))
    {
        return vdir->get_file(file_name)->open();
    }
    return driver->file_open(file.substring(mount_dir_end, file.len()).c_str());
}

bool PipeFile::seek(unsigned pos)
{
    throw NotImplementedError();
}
unsigned PipeFile::read(char *buffer, unsigned bytes)
{
    wait_for_data();
    unsigned bytes_read = 0;
    while (!eof() && bytes--)
    {
        *(buffer++) = data->pop_front();
        bytes_read++;
    }
    return bytes_read;
}
inline void PipeFile::unblock_waiting_process()
{
    if (waiting_process) [[unlikely]]
    {
        waiting_process->blocked = false;
        waiting_process = nullptr;
    }
}
void PipeFile::write(const char *buffer, unsigned bytes)
{
    while (bytes--)
    {
        data->push_back(*(buffer++));
        unblock_waiting_process();
    }
}
void PipeFile::pipe_close() // Must be non-virtual for destructor
{
    closed = true;
    unblock_waiting_process();
}
void PipeFile::wait_for_data()
{
    while (!closed && data->len() == 0)
        // while should not be needed here; but in case the process
        // gets rescheduled while there is still no data, block again.
    {
        current_process->blocked = true;
        waiting_process = current_process;
        process_yield();
    }
}
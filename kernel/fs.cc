#include "fs.h"
#include "string.h"

using namespace kernel;

void FileSystem::mount(KString dir, std::shared_ptr<FileSystemDriver> driver)
{
    if (dir.len() <= 0 || dir[0] != '/') throw ArgumentError();
    if (dir.len() > 1 && dir[dir.len()-1] == '/')
    {
        dir = dir.substring(0, dir.len()-1);
    }
    _mount_points.insert(dir, driver);
}

std::unique_ptr<File> FileSystem::open(KString file)
{
    std::shared_ptr<FileSystemDriver> driver;
    KString driver_mount_point;
    auto vdir = _virtual_root.get();
    int dir_end = 0, mount_dir_end = 0;
    for (auto i = 0; i < file.len(); i++)
    {
        if (file[i] == '/')
        {
            // Copy the current directory to a string.
            auto dir_name = file.substring(dir_end, i);
            dir_end = i + 1; // inclusive of final slash
            auto current_dir = i == 0 ? KString("/") : file.substring(0, i);
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

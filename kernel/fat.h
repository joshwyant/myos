#ifndef __FATC_H__
#define __FATC_H__

#include <stddef.h>
#include "driver.h"
#include "disk.h"
#include "error.h"
#include "fs.h"
#include "stack.h"

#ifdef __cplusplus
#include <memory>
#include <utility>

namespace kernel
{
class FATDriver;

class FATFile
    : public File
{
public:
    unsigned filesize;
    unsigned position;
    bool seek(unsigned pos) override;
    unsigned read(char *buffer, unsigned bytes) override;
    void write(const char *buffer, unsigned bytes) override
    {
        throw NotImplementedError();
    }
    char getch() override;
    char peekc() override;
    bool eof() override;
    void close() override { fat_close(); }
    ~FATFile() override
    {
        fat_close();
    }
private:
    FATFile(std::weak_ptr<FATDriver> fat_driver)
        : filesize(0),
          position(0),
          fat_driver(fat_driver),
          closed(false),
          File() {}
    void fat_close();
    bool push_cluster();
    bool pop_cluster();
    friend class FATDriver;
    std::weak_ptr<FATDriver> fat_driver;
    Stack<unsigned> cluster_stack;
    unsigned firstcluster;
    unsigned currentcluster;
    char* buffer;
    bool closed;
}; // class FATFile

class FATDriver
    : public FileSystemDriver,
      public std::enable_shared_from_this<FATDriver>
{
public:
    class FileStream;
    FATDriver(std::shared_ptr<DiskDriver> disk_driver, KString device_name = "hda1")
        : disk_driver(std::move(disk_driver)),
          FileSystemDriver(device_name)
    {
        fat_init();
    }
    ~FATDriver() override {}
    FATDriver(FATDriver&) = delete;
    FATDriver(FATDriver&&) noexcept = delete;
    FATDriver& operator=(FATDriver) = delete;
    bool read_file(const char* filename, void* buffer);
    unsigned int file_size(const char* filename);
    bool file_exists(const char* filename);
    bool directory_exists(const char* dirname);
    bool chdir(const char* dir);
    const char* current_directory();
    std::unique_ptr<File> file_open(const char *filename) override;
private:
    friend class FATFile;
    void fat_init();
    bool read_cluster(int, void**);
    bool write_cluster(int, void*);
    bool _read_file_info(const char*,int);
    bool trace_info(const char*, int);
    bool read_file_info(const char*);
    bool read_dir_info(const char*);
    bool read_root_file(const char*, int is_dir);
    bool dot_to_83(const char*, char*);
    bool streq(const char*, const char*);
    void read_sector(int sector, void** buffer);
    void write_sector(int, void*);
    bool find_file(const char*, int size, int attributes);
    unsigned int next_cluster(int cluster);
    unsigned int alloc_cluster(unsigned int);
    int set_next_cluster(unsigned int, unsigned int);
    unsigned int alloc_next_cluster(unsigned int);
    inline bool eof(unsigned);

    std::shared_ptr<DiskDriver> disk_driver;
    struct __attribute__ ((__packed__)) BPBMain
    {
        unsigned char	jmpBoot[3];
        unsigned char	OEMName[8];
        unsigned short	BytsPerSec;
        unsigned char	SecPerClus;
        unsigned short	RsvdSecCnt;
        unsigned char	NumFATs;
        unsigned short	RootEntCnt;
        unsigned short	TotSec16;
        unsigned char	Media;
        unsigned short	FATSz16;
        unsigned short	SecPerTrk;
        unsigned short	NumHeads;
        unsigned int	HiddSec;
        unsigned int	TotSec32;
    } bpb;
    struct __attribute__ ((__packed__)) BPB16
    {
        unsigned char	DrvNum;
        unsigned char	Reserved1;
        unsigned char	BootSig;
        unsigned int	VolID;
        unsigned char	VolLab[11];
        unsigned char	FilSysType[8];
    } bpb16;
    struct __attribute__ ((__packed__)) BPB32
    {
        unsigned int	FATSz32;
        unsigned short	ExtFlags;
        unsigned short	FSVer;
        unsigned int	RootClus;
        unsigned short	FSInfo;
        unsigned short	BkBootSec;
        unsigned char	Reserved[12];
        unsigned char	DrvNum;
        unsigned char	Reserved1;
        unsigned char	BootSig;
        unsigned int	VolID;
        unsigned char	VolLab[11];
        unsigned char	FilSysType[8];
    } bpb32;
    char fat[1024]; // FAT sector cache, supports 512 byte sectors (space to read 2 sectors in case of fat12)
    char disk[0x10000]; // up to 64k clusters (standard is 32k), some drivers 64k
    struct __attribute__ ((__packed__)) file_entry
    {
        unsigned char filename[11];
        unsigned char attributes;
        unsigned char name_case;
        unsigned char create_time_fine;
        unsigned short create_time;
        unsigned short create_date;
        unsigned short last_access_date;
        unsigned short cluster_high;
        unsigned short last_modified_time;
        unsigned short last_modified_date;
        unsigned short cluster_low;
        unsigned int file_size;
    } file_info, // Current file entry
      dir_info, // Currently handled directory entry
      cdir_info; // Current (user) directory entry
    char current_dir[256]; // Current directory pathname
    int root_sectors; // Root directory sectors (FAT16)
    int data_start; // First data sector
    int root_start; // First root directory sector
    int cluster_size; // Size of a cluster
    int cur_fat_sec; // Cached FAT sector
    int count_of_clusters; // Cluster count
    enum
    {
        TFAT12,
        TFAT16,
        TFAT32
    } fat_type;

    struct __attribute__ ((__packed__)) long_entry
    {
        unsigned char	ord;
        unsigned short	name1[5];
        unsigned char	attr;
        unsigned char	type;
        unsigned char	chksum;
        unsigned short	name2[6];
        unsigned short	fstcluslo;
        unsigned short	name3[2];    
    };
}; // class FATDriver
} // namespace kernel
#endif // __cplusplus
#endif // __FATC_H__

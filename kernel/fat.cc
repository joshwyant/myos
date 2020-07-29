#include <memory>
#include "error.h"
#include "fat.h"
#include "fs.h"

// File Attributes
#define ATTR_READ_ONLY      0x01
#define ATTR_HIDDEN         0x02
#define ATTR_SYSTEM         0x04
#define ATTR_VOLUME_ID      0x08
#define ATTR_DIRECTORY      0x10
#define ATTR_ARCHIVE        0x20
#define ATTR_LONG_NAME      (ATTR_READ_ONLY | ATTR_HIDDEN | ATTR_SYSTEM | ATTR_VOLUME_ID)
#define ATTR_LONG_NAME_MASK (ATTR_READ_ONLY | ATTR_HIDDEN | ATTR_SYSTEM | ATTR_VOLUME_ID | \
                             ATTR_DIRECTORY | ATTR_ARCHIVE)

// FAT Long Directory Entry Structure
#define LDIR_Ord            0
#define LDIR_Name1          1
#define LDIR_Attr           11
#define LDIR_Type           12
#define LDIR_Chksum         13
#define LDIR_Name2          14
#define LDIR_FstClusLO      26
#define LDIR_Name3          28

#define LAST_LONG_ENTRY     0x40

// Call before using any FAT functions.
void kernel::FATDriver::fat_init()
{
    // Copy BIOS parameter block from where the boot sector was loaded
    char* bootsect = (char*)kfindrange(0x1000)+0xC00;
    page_map(bootsect, (void*)0x7000, PF_WRITE);
    bpb = *(BPBMain*)bootsect;
    bpb16 = *(BPB16*)(void*)(bootsect+36);
    bpb32 = *(BPB32*)(void*)(bootsect+36);
    page_unmap(bootsect);
    page_free((void*)0x7000, 1);
    // make current directory the root directory
    kstrcpy(current_dir, "/");
    dir_info.cluster_low = 0; // cluster 0 directory is root
    // sector-based calculations
    root_start = bpb.RsvdSecCnt+(bpb.FATSz16 ? bpb.FATSz16 : bpb32.FATSz32)*bpb.NumFATs;
    root_sectors = bpb.RootEntCnt*32+(bpb.BytsPerSec-1)/bpb.BytsPerSec;
    data_start = root_start+root_sectors;
    cluster_size = bpb.SecPerClus*bpb.BytsPerSec;
    count_of_clusters = ((bpb.TotSec16 ? bpb.TotSec16 : bpb.TotSec32)-data_start)/bpb.SecPerClus;
    // Determine FAT type
    fat_type = count_of_clusters < 4085 ? TFAT12: count_of_clusters < 65525 ? TFAT16 : TFAT32;
    // invalidate current FAT sector number
    cur_fat_sec = 0;
}

std::unique_ptr<kernel::File> kernel::FATDriver::file_open(const char *filename)
{
    if (!read_file_info(filename))
    {
        throw NotFoundError();
    }
    std::unique_ptr<FATFile> f(new FATFile(weak_from_this()));
    f->filesize = file_info.file_size;
    f->position = 0;
    f->buffer = (char *)kmalloc(cluster_size);
    f->firstcluster = file_info.cluster_low|(fat_type == TFAT32 ? (file_info.cluster_high<<16) : 0);
    f->currentcluster = f->firstcluster;
    void* buffer = f->buffer;
    read_cluster(f->firstcluster, &buffer);
    return f;
}

void kernel::FATFile::fat_close()
{
    if (closed) return;
    position = 0;
    filesize = 0;
    closed = true;
}

inline bool kernel::FATDriver::eof(unsigned clus)
{
    if (fat_type == TFAT12) {
        return clus >= 0xFF8;
    } else if (fat_type == TFAT16) {
        return clus >= 0xFFF8;
    } else if (fat_type == TFAT32) {
        return clus >= 0x0FFFFFF8;
    }
    return true;
}

bool kernel::FATFile::push_cluster()
{  
    if (currentcluster == 0) return false;
    auto fat = fat_driver.lock();
    unsigned nextcluster = fat->next_cluster(currentcluster);
    if (fat->eof(nextcluster)) nextcluster = 0;
    // Push cluster on stack
    cluster_stack.push_back(currentcluster);
    // Make cluster current
    currentcluster = nextcluster;
    return true;
}

bool kernel::FATFile::pop_cluster()
{
    // check for stack underflow
    if (!cluster_stack.len()) return false;
    // pop cluster
    currentcluster = cluster_stack.pop_back();
    return true;
}

bool kernel::FATFile::seek(unsigned pos)
{
    if (pos > filesize) return false;
    auto fat = fat_driver.lock();
    unsigned cluster = pos / fat->cluster_size;
    position = pos;
    if (cluster == cluster_stack.len())
        return true;
    while (cluster_stack.len() < cluster)
        push_cluster();
    while (cluster_stack.len() > cluster)
        pop_cluster();
    // read in the buffer
    void* buffer = this->buffer;
    if (currentcluster) fat->read_cluster(currentcluster, &buffer);
    return true;
}

unsigned kernel::FATFile::read(char *buffer, unsigned bytes)
{
    unsigned bytes_read = 0;
    unsigned bytes_to_read;
    auto fat = fat_driver.lock();
    do
    {
        unsigned offset = position % fat->cluster_size;
        unsigned cluster_bytes = fat->cluster_size - offset;
        unsigned user_bytes = bytes - bytes_read;
        unsigned file_bytes = filesize - position;
        unsigned minval = cluster_bytes < file_bytes ? cluster_bytes : file_bytes;
        bytes_to_read = user_bytes < minval ? user_bytes : minval;
        int i;
        for (i = 0; i < bytes_to_read; i++)
            buffer[bytes_read+i] = this->buffer[offset+i];
        bytes_read += bytes_to_read;
        seek(position+bytes_to_read);
    } while (bytes_to_read);
    return bytes_read;
}

char kernel::FATFile::getch()
{
    char c = '\xFF';
    read(&c, 1);
    return c;
}

char kernel::FATFile::peekc()
{
    unsigned pos = position;
    char c = getch();
    seek(pos);
    return c;
}

bool kernel::FATFile::eof()
{
    return position >= filesize;
}

// changes the current directory.
// returns whether succesful.
bool kernel::FATDriver::chdir(const char* dir)
{
    if (!read_dir_info(dir)) return false;
    kstrcpy(current_dir, dir);
    cdir_info = dir_info;
    return true;
}

// Copies the whole file to memory. Returns whether succeeded.
bool kernel::FATDriver::read_file(const char* filename, void* buffer)
{
    if (!read_file_info(filename)) return false;
    int cluster = file_info.cluster_low;
    void* tbuffer = kmalloc(cluster_size);
    void* tb; // so that tbuffer is not overwritten
    int bytes_written = 0, i;
    while (cluster < 0xFFF8)
    {
        tb = tbuffer;
        if (!read_cluster(cluster, &tb)) { kfree(tbuffer); return false; };
        // TODO: use kmcpy (not yet implemented) instead?
        for (i = 0; (i < cluster_size) && (bytes_written < file_info.file_size); i++, bytes_written++)
            ((char*)buffer)[bytes_written] = ((char*)tbuffer)[i];
        cluster = next_cluster(cluster);
    }
    kfree(tbuffer);
    return true;
}

unsigned int kernel::FATDriver::file_size(const char* filename)
{
    if (!read_file_info(filename)) return 0;
    return file_info.file_size;
}

bool kernel::FATDriver::file_exists(const char* filename)
{
    return (read_file_info(filename));
}

bool kernel::FATDriver::directory_exists(const char* dirname)
{
    return (read_dir_info(dirname));
}

const char* kernel::FATDriver::current_directory()
{
    return current_dir;
}

bool kernel::FATDriver::streq(const char* str1, const char* str2)
{
    while (true)
    {
        if (*str1 != *str2) return false;
        if (*str1 == '\0') break;
        str1++; str2++;
    }
    return true;
}

// ignores volume labels.
// returns whether a file was found.
// specify whether a directory or a file. A block of size
// of the directory table is in the disk[] buffer.
bool kernel::FATDriver::find_file(const char* filename, int size, int is_dir)
{
    int i, j, found;
    char fname83[11];
    if (!dot_to_83(filename,fname83)) return false;
    for (i = 0; i < size; i += 32)
    {
        found = (is_dir ? ((disk[i+11]&0x10)==0x10) : 1) && !(disk[i+11]&8);
        if (found) for (j = 0; j < 11; j++)
        {
            if (fname83[j] != disk[i+j])
            {
                found = 0;
                break;
            }
        }
        if (found)
        {
            char* ptr = (is_dir ? (char*)&dir_info : (char*)&file_info);
            for (j = 0; j < 32; j++) ptr[j] = disk[i+j];
            return true;
        }
    }
    return false;
}

// checks whether a file or folder exists in the root
// directory. If it does, the appropriate info structure is
// loaded when the function returns.
bool kernel::FATDriver::read_root_file(const char* filename, int is_dir)
{
    int i;
    void* b;
    unsigned cluster = bpb32.RootClus;
    if (fat_type == TFAT32)
    {
        while (!eof(cluster))
        {
            b = disk;
            read_cluster(cluster, &b);
            if (find_file(filename,cluster_size,is_dir)) return true;
            cluster = next_cluster(cluster);
        }
    }
    else
    {
        for (i = 0; i < root_sectors; i++)
        {
            b = disk;
            read_sector(root_start+i, &b);
            if (find_file(filename,bpb.BytsPerSec,is_dir)) return true;
        }
    }
    return false;
}

// reads the FAT to determine the next cluster in the
// list after the given cluster.
unsigned int kernel::FATDriver::next_cluster(int cluster)
{
    void* b = fat;
    unsigned offset;
    if (fat_type == TFAT12)
        offset = cluster + (cluster / 2);
    else if (fat_type == TFAT16)
        offset = cluster * 2;
    else
        offset = cluster * 4;
    int sector = offset / bpb.BytsPerSec + bpb.RsvdSecCnt;
    if (sector != cur_fat_sec)
    {
        read_sector(sector, &b);
        if (fat_type == TFAT12)
            read_sector(sector+1, &b);
    }
    cur_fat_sec = sector;
    if (fat_type == TFAT12)
    {
        if (cluster & 1)
            return *(unsigned short*)(fat+(offset % bpb.BytsPerSec)) >> 4;
        else
            return *(unsigned short*)(fat+(offset % bpb.BytsPerSec)) & 0xFFF;
    }
    else if (fat_type == TFAT16)
        return *(unsigned short*)(fat+(offset % bpb.BytsPerSec));
    else
        return *(unsigned*)(fat+(offset % bpb.BytsPerSec));
}


// Takes a filename (without the directory)
// and constructs the DOS 8.3 filename.
// returns whether successful.
bool kernel::FATDriver::dot_to_83(const char* fdot, char* f83)
{
    int i;
    for (i = 0; i < 11; i++) f83[i] = 0x20;
    if (fdot[0] == '.') 
    {
        f83[0] = '.';
        if (streq("..",fdot))
        {
            f83[1] = '.';
            return true;
        }
        else if (!streq(".",fdot)) return false;
    } else {
        for (i = 0; i < 8; i++, fdot++)
        {
            if ((*fdot == '.') || (*fdot == 0)) break;
            f83[i] = ktoupper(*fdot);
        }
        if (*fdot == '.') fdot++; else if (*fdot != 0) return false;
        for (i = 0; i < 3; i++, fdot++)
        {
            if (*fdot == 0) break;
            f83[8+i] = ktoupper(*fdot);
        }
        if (*fdot != 0) return false;
    }
    return true;
}

// returns whether the file or directory exists in the current directory,
// and if it does, updates file_info or dir_info.
bool kernel::FATDriver::_read_file_info(const char* filename, int is_dir)
{
    if (dir_info.cluster_low == 0) return read_root_file(filename, is_dir);
    int cluster = dir_info.cluster_low;
    void* b;
    while (!eof(cluster))
    {
        b = disk;
        if (!read_cluster(cluster, &b)) return false;
        if (find_file(filename,cluster_size,is_dir)) return true;
        cluster = next_cluster(cluster);
    }
    return false;
}

// updates the structures and returns whether the file exists.
bool kernel::FATDriver::trace_info(const char* filename, int is_dir)
{
    dir_info = cdir_info;
    if (filename[0] == '/') dir_info.cluster_low = (filename++,0);
    char name[13];
    int i;
    // change the directory.
    while (1)
    {
        for (i = 0; i < 12; i++)
        {
            if ((filename[i] == 0) || (filename[i] == '/'))
            {
                name[i] = 0;
                filename += i;
                break;
            }
            name[i] = filename[i];
        }
        if (*filename == '/')
        {
            filename ++;
            if (!_read_file_info(name, 1)) return false;
        }
        else if (*filename != 0)
            return false;
        else break;
    }
    if (name[0] == 0) return is_dir;
    return _read_file_info(name, is_dir);
}

// returns whether the file exists,
// and if it does, updates file_info.
bool kernel::FATDriver::read_file_info(const char* filename)
{
    return trace_info(filename,0);
}

bool kernel::FATDriver::read_dir_info(const char* name)
{
    return trace_info(name,1);
}

// pass the address of a pointer, and it will be updated.
// char* buffer;
// read_cluster (cluster, &buffer);
// read_cluster (next_cluster(cluster), &buffer);
bool kernel::FATDriver::read_cluster(int cluster, void** buffer)
{
    int i;
    if (cluster < 2) return false;
    for (i = 0; i < bpb.SecPerClus; i++)
        read_sector(data_start+(cluster-2)*bpb.SecPerClus+i, buffer);
    return true;
}

// read sector within the FAT partition.
// Updates pointer past the bytes read.
// sector: Linear Block Address
// buffer: Address of pointer to current buffer position
void kernel::FATDriver::read_sector(int sector, void** buffer)
{
    // // using CHS
    // int heads, spt;
    // sector += bpb.HiddSec;
    // heads = bpb.NumHeads;
    // spt = bpb.SecPerTrk;
    // ReadSectorsCHS(*buffer, 1, (BPB[BS_DrvNum]&0xF)/2, BPB[BS_DrvNum]%2, sector/spt/heads, (sector/spt)%heads, (sector%spt)+1, *((unsigned short*)(BPB+BPB_BytsPerSec)));
    // buffer += *((unsigned short*)(BPB+BPB_BytsPerSec));

    // 4/5/08
    // I DID IT!!! YES!!!!! 
    // This bug took days to track. I figured it
    // out in bochs debugger: Hidden sectors was 0xFFF-something,
    // it should have been 0. I realized that the below line was
    // typed in error: *((unsigned int)BPB+BPB_HiddSec);
    // This would fetch the value from the FAT buffer, which lies
    // past the BPB, because BPB_HiddSec was treated as an offset
    // into an int array, not an int at a byte offset.
    sector += bpb.HiddSec;
    disk_driver->read_sectors(*(char**)buffer, 1, sector);
    *(char*)buffer += 512;
}

void kernel::FATDriver::write_sector(int sector, void* buffer)
{
    sector += bpb.HiddSec;
    disk_driver->write_sectors((const char*)buffer, 1, sector);
}

bool kernel::FATDriver::write_cluster(int cluster, void* buffer)
{
    int i;
    if (cluster < 2) return false;
    for (i = 0; i < bpb.SecPerClus; i++)
        write_sector(data_start+(cluster-2)*bpb.SecPerClus+i, buffer);
    return true;
}

// This function only looks for free clusters starting from cluster_start.
unsigned int kernel::FATDriver::alloc_cluster(unsigned int cluster_start)
{
    unsigned int i;
    for (i = cluster_start<2?2:cluster_start; i < count_of_clusters+2; i++)
    {
        if (!next_cluster(i)) return i;
    }
    return 0;
}

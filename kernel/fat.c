#include "fatc.h"

// Call before using any FAT functions.
void fat_init()
{
    // Copy BIOS parameter block from where the boot sector was loaded
    void* bootsect = kfindrange(0x1000)+0xC00;
    page_map(bootsect, (void*)0x7000);
    bpb = *(BPBMain*)bootsect;
    bpb16 = *(BPB16*)(bootsect+36);
    bpb32 = *(BPB32*)(bootsect+36);
    page_unmap(bootsect);
    page_free((void*)0x7000, 1);
    // make current directory the root directory
    kstrcpy("/", current_dir);
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

int file_open(char *filename, FileStream *fs)
{
    if (!read_file_info(filename))
    {
        fs->data = 0;
        fs->position = 0;
        fs->filesize = 0;
        return 0;
    }
    fs->filesize = file_info.file_size;
    fs->position = 0;
    fs->data = kmalloc(sizeof(struct fsdata));
    fs->data->buffer = kmalloc(cluster_size);
    const int clusterstacksize = 4;
    fs->data->clusterstack = kmalloc(clusterstacksize*sizeof(unsigned));
    fs->data->clusterstacksize = clusterstacksize;
    fs->data->firstcluster = file_info.cluster_low|(fat_type == TFAT32 ? (file_info.cluster_high<<16) : 0);
    fs->data->currentcluster = fs->data->firstcluster;
    fs->data->stack_index = 0;
    void* buffer = fs->data->buffer;
    read_cluster(fs->data->firstcluster, &buffer);
}

void file_close(FileStream *fs)
{
    if (!fs) return;
    kfree(fs->data->buffer);
    kfree(fs->data->clusterstack);
    kfree(fs->data);
    fs->data = 0;
    fs->position = 0;
    fs->filesize = 0;
}

static inline int eof(unsigned clus)
{
    if (fat_type == TFAT12) {
        return clus >= 0xFF8;
    } else if (fat_type == TFAT16) {
        return clus >= 0xFFF8;
    } else if (fat_type == TFAT32) {
        return clus >= 0x0FFFFFF8;
    }
}

int push_cluster(FileStream *fs)
{  
    if (fs->data->currentcluster == 0) return 0;
    unsigned nextcluster = next_cluster(fs->data->currentcluster);
    if (eof(nextcluster)) nextcluster = 0;
    // Resize stack if needed
    if (fs->data->stack_index >= fs->data->clusterstacksize)
    {
        unsigned* newptr = kmalloc(fs->data->clusterstacksize*2*sizeof(unsigned));
        int i;
        for (i = 0; i < fs->data->stack_index; i++)
            newptr[i] = fs->data->clusterstack[i];
        kfree(fs->data->clusterstack);
        fs->data->clusterstack = newptr;
        // THE BUG IS FIXED! >>>
        fs->data->clusterstacksize *= 2;
    }
    // Push cluster on stack
    fs->data->clusterstack[fs->data->stack_index++] = fs->data->currentcluster;
    // Make cluster current
    fs->data->currentcluster = nextcluster;
    return 1;
}

int pop_cluster(FileStream *fs)
{
    // check for stack underflow
    if (fs->data->stack_index == 0) return 0;
    // pop cluster
    fs->data->currentcluster = fs->data->clusterstack[--fs->data->stack_index];
    // Resize stack if needed
    if (fs->data->stack_index <= fs->data->clusterstacksize/2)
    {
        unsigned* newptr = kmalloc(fs->data->clusterstacksize/2*sizeof(unsigned));
        int i;
        for (i = 0; i < fs->data->stack_index; i++)
            newptr[i] = fs->data->clusterstack[i];
        kfree(fs->data->clusterstack);
        fs->data->clusterstack = newptr;
        // THE BUG IS FIXED! >>>
        fs->data->clusterstacksize /= 2;
    }
    return 1;
}

int file_seek(FileStream *fs, unsigned pos)
{
    if (pos > fs->filesize) return 0;
    unsigned cluster = pos / cluster_size;
    if (cluster == fs->data->stack_index)
    {
        fs->position = pos;
        return 1;
    }
    while (fs->data->stack_index < cluster)
        push_cluster(fs);
    while (fs->data->stack_index > cluster)
        pop_cluster(fs);
    fs->position = pos;
    // read in the buffer
    void* buffer = fs->data->buffer;
    if (fs->data->currentcluster) read_cluster(fs->data->currentcluster, &buffer);
    return 1;
}

unsigned file_read(FileStream *fs, void *buffer, unsigned bytes)
{
    unsigned bytes_read = 0;
    unsigned bytes_to_read;
    do
    {
        unsigned offset = fs->position % cluster_size;
        unsigned cluster_bytes = cluster_size - offset;
        unsigned user_bytes = bytes - bytes_read;
        unsigned file_bytes = fs->filesize - fs->position;
        unsigned minval = cluster_bytes < file_bytes ? cluster_bytes : file_bytes;
        bytes_to_read = user_bytes < minval ? user_bytes : minval;
        int i;
        for (i = 0; i < bytes_to_read; i++)
            ((char*)buffer)[bytes_read+i] = ((char*)fs->data->buffer)[offset+i];
        bytes_read += bytes_to_read;
        file_seek(fs, fs->position+bytes_to_read);
    } while (bytes_to_read);
    return bytes_read;
}

char file_getc(FileStream *fs)
{
    char c = '\xFF';
    file_read(fs, &c, 1);
    return c;
}

char file_peekc(FileStream *fs)
{
    unsigned pos = fs->position;
    char c = file_getc(fs);
    file_seek(fs, pos);
    return c;
}

int file_eof(FileStream *fs)
{
    return fs->position >= fs->filesize;
}

// changes the current directory.
// returns whether succesful.
int chdir(const char* dir)
{
    if (!read_dir_info(dir)) return 0;
    kstrcpy(dir,current_dir);
    cdir_info = dir_info;
    return 1;
}

// Copies the whole file to memory. Returns whether succeeded.
int read_file(const char* filename, void* buffer)
{
    if (!read_file_info(filename)) return 0;
    int cluster = file_info.cluster_low;
    void* tbuffer = kmalloc(cluster_size);
    void* tb; // so that tbuffer is not overwritten
    int bytes_written = 0, i;
    while (cluster < 0xFFF8)
    {
        tb = tbuffer;
        if (!read_cluster(cluster, &tb)) { kfree(tbuffer); return 0; };
        // TODO: use kmcpy (not yet implemented) instead?
        for (i = 0; (i < cluster_size) && (bytes_written < file_info.file_size); i++, bytes_written++)
            ((char*)buffer)[bytes_written] = ((char*)tbuffer)[i];
        cluster = next_cluster(cluster);
    }
    kfree(tbuffer);
    return 1;
}

unsigned int file_size(const char* filename)
{
    if (!read_file_info(filename)) return 0;
    return file_info.file_size;
}

int file_exists(const char* filename)
{
    return (read_file_info(filename));
}

int directory_exists(const char* dirname)
{
    return (read_dir_info(dirname));
}

const char* current_directory()
{
    return current_dir;
}

/* ************************************ */
/* ********* STATIC FUNCTIONS ********* */
/* ************************************ */


static void kstrcpy(const char* src, char* dest)
{
    do { *dest++ = *src; } while (*src++);
}

static int streq(const char* str1, const char* str2)
{
    while (1)
    {
        if (*str1 != *str2) return 0;
        if (*str1 == 0) break;
        str1++; str2++;
    }
    return 1;
}

// ignores volume labels.
// returns whether a file was found.
// specify whether a directory or a file. A block of size
// of the directory table is in the disk[] buffer.
static int find_file(const char* filename, int size, int is_dir)
{
    int i, j, found;
    char fname83[11];
    if (!dot_to_83(filename,fname83)) return 0;
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
            return 1;
        }
    }
    return 0;
}

// checks whether a file or folder exists in the root
// directory. If it does, the appropriate info structure is
// loaded when the function returns.
static int read_root_file(const char* filename, int is_dir)
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
            if (find_file(filename,cluster_size,is_dir)) return 1;
            cluster = next_cluster(cluster);
        }
    }
    else
    {
        for (i = 0; i < root_sectors; i++)
        {
            b = disk;
            read_sector(root_start+i, &b);
            if (find_file(filename,bpb.BytsPerSec,is_dir)) return 1;
        }
    }
    return 0;
}

// reads the FAT to determine the next cluster in the
// list after the given cluster.
static unsigned int next_cluster(int cluster)
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

// Returns c in uppercase.
static char ktoupper(char c)
{
    if ((c < 'a') || (c > 'z')) return c;
    return c-'a'+'A';
}

// Takes a filename (without the directory)
// and constructs the DOS 8.3 filename.
// returns whether successful.
static int dot_to_83(const char* fdot, char* f83)
{
    int i;
    for (i = 0; i < 11; i++) f83[i] = 0x20;
    if (fdot[0] == '.') 
    {
        f83[0] = '.';
        if (streq("..",fdot))
        {
            f83[1] = '.';
            return 1;
        }
        else if (!streq(".",fdot)) return 0;
    } else {
        for (i = 0; i < 8; i++, fdot++)
        {
            if ((*fdot == '.') || (*fdot == 0)) break;
            f83[i] = ktoupper(*fdot);
        }
        if (*fdot == '.') fdot++; else if (*fdot != 0) return 0;
        for (i = 0; i < 3; i++, fdot++)
        {
            if (*fdot == 0) break;
            f83[8+i] = ktoupper(*fdot);
        }
        if (*fdot != 0) return 0;
    }
    return 1;
}

// returns whether the file or directory exists in the current directory,
// and if it does, updates file_info or dir_info.
static int _read_file_info(const char* filename, int is_dir)
{
    if (dir_info.cluster_low == 0) return read_root_file(filename, is_dir);
    int cluster = dir_info.cluster_low;
    void* b;
    while (!eof(cluster))
    {
        b = disk;
        if (!read_cluster(cluster, &b)) return 0;
        if (find_file(filename,cluster_size,is_dir)) return 1;
        cluster = next_cluster(cluster);
    }
    return 0;
}

// updates the structures and returns whether the file exists.
static int trace_info(const char* filename, int is_dir)
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
            if (!_read_file_info(name, 1)) return 0;
        }
        else if (*filename != 0)
            return 0;
        else break;
    }
    if (name[0] == 0) return is_dir;
    return _read_file_info(name, is_dir);
}

// returns whether the file exists,
// and if it does, updates file_info.
static int read_file_info(const char* filename)
{
    return trace_info(filename,0);
}

static int read_dir_info(const char* name)
{
    return trace_info(name,1);
}

// pass the address of a pointer, and it will be updated.
// char* buffer;
// read_cluster (cluster, &buffer);
// read_cluster (next_cluster(cluster), &buffer);
static int read_cluster(int cluster, void** buffer)
{
    int i;
    if (cluster < 2) return 0;
    for (i = 0; i < bpb.SecPerClus; i++)
        read_sector(data_start+(cluster-2)*bpb.SecPerClus+i, buffer);
    return 1;
}

// read sector within the FAT partition.
// Updates pointer past the bytes read.
// sector: Linear Block Address
// buffer: Address of pointer to current buffer position
static void read_sector(int sector, void** buffer)
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
    ReadSectors(*buffer, 1, 0, 0, sector);
    *buffer += 512;
}

static void write_sector(int sector, void* buffer)
{
    sector += bpb.HiddSec;
    WriteSectors(buffer, 1, 0, 0, sector);
}

static int write_cluster(int cluster, void* buffer)
{
    int i;
    if (cluster < 2) return 0;
    for (i = 0; i < bpb.SecPerClus; i++)
        write_sector(data_start+(cluster-2)*bpb.SecPerClus+i, buffer);
    return 1;
}

// This function only looks for free clusters starting from cluster_start.
static unsigned int alloc_cluster(unsigned int cluster_start)
{
    unsigned int i;
    for (i = cluster_start<2?2:cluster_start; i < count_of_clusters+2; i++)
    {
        if (!next_cluster(i)) return i;
    }
    return 0;
}
/*
static int set_next_cluster(unsigned int cluster, unsigned int cluster_after)
{
    /*if ((cluster_after == cluster) || (cluster_after < 2) || (cluster >= (count_of_clusters+2)) || (cluster_after >= (count_of_clusters+2))) return 0;
    int sector = cluster*2 / *(unsigned short*)(BPB+BPB_BytsPerSec) + *(unsigned short*)(BPB+BPB_RsvdSecCnt);
    if ((sector-*(unsigned short*)(BPB+BPB_RsvdSecCnt)) >= *(unsigned short*)(BPB+BPB_FATSz16)) return 0;
    void* b = fat;
    if (sector != cur_fat_sec)
        read_sector(sector, &b);
    cur_fat_sec = sector;
    int offset = ((cluster*2) % *(unsigned short*)(BPB+BPB_BytsPerSec));
    *(unsigned short*)(fat+offset) = cluster_after;
    write_sector(sector, fat);
    return 1;*//*
}

static unsigned int alloc_next_cluster(unsigned int cluster)
{
    unsigned int cluster_after = alloc_cluster(0);
    if (!cluster_after) return 0;
    if (!set_next_cluster(cluster,cluster_after)) return 0;
    return cluster_after;
}
*/

#ifndef __FATC_H__
#define __FATC_H__

# include "disk.h"
# include "klib.h"


// Boot Sector and BPB Structure
#define BS_jmpBoot          0
#define BS_OEMName          3
#define BPB_BytsPerSec      11
#define BPB_SecPerClus      13
#define BPB_RsvdSecCnt      14
#define BPB_NumFATs         16
#define BPB_RootEntCnt      17
#define BPB_TotSec16        19
#define BPB_Media           21
#define BPB_FATSz16         22
#define BPB_SecPerTrk       24
#define BPB_NumHeads        26
#define BPB_HiddSec         28
#define BPB_TotSec32        32

#define TFAT12		0
#define TFAT16		1
#define TFAT32		2

#ifdef __cplusplus
extern "C" {
#endif

typedef struct  __attribute__ ((__packed__)) {
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
} BPBMain;

// Fat12 and Fat16 Structure Starting at Offset 36
#define BS_DrvNum           36
#define BS_Reserved1        37
#define BS_BootSig          38
#define BS_VolID            39
#define BS_VolLab           43
#define BS_FilSysType       54

typedef struct  __attribute__ ((__packed__)) {
    unsigned char	DrvNum;
    unsigned char	Reserved1;
    unsigned char	BootSig;
    unsigned int	VolID;
    unsigned char	VolLab[11];
    unsigned char	FilSysType[8];
} BPB16;

// FAT32 Structure Starting at Offset 36
#define BPB_FATSz32         36
#define BPB_ExtFlags        40
#define BPB_FSVer           42
#define BPB_RootClus        44
#define BPB_FSInfo          48
#define BPB_BkBootSec       50
#define BPB_Reserved        52
#define BS32_DrvNum         64
#define BS32_Reserved1      65
#define BS32_BootSig        66
#define BS32_VolID          67
#define BS32_VolLab         71
#define BS32_FilSysType     82

typedef struct  __attribute__ ((__packed__)) {
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
} BPB32;

// FAT32 FSInfo Sector Structure and Backup Boot Sector
#define FSI_LeadSig         0
#define FSI_Reserved1       4
#define FSI_StrucSig        484
#define FSI_Free_Count      488
#define FSI_Nxt_Free        492
#define FSI_Reserved2       496
#define FSI_TrailSig        508

// FAT 32 Byte Directory Entry Structure
#define DIR_Name            0
#define DIR_Attr            11
#define DIR_NTRes           12
#define DIR_CrtTimeTenth    13

#define DIR_CrtTime         14
#define DIR_CrtDate         16
#define DIR_LstAccDate      18

#define DIR_FstClusHI       20
#define DIR_WrtTime         22
#define DIR_WrtDate         24
#define DIR_FstClusLO       26
#define DIR_FileSize        28

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
};

typedef struct __attribute__ ((__packed__)) {
    unsigned char	ord;
    unsigned short	name1[5];
    unsigned char	attr;
    unsigned char	type;
    unsigned char	chksum;
    unsigned short	name2[6];
    unsigned short	fstcluslo;
    unsigned short	name3[2];    
} long_entry;

#undef FileStream
struct fsdata
{
    // Private data
    unsigned firstcluster;
    unsigned clusterstacksize;
    unsigned* clusterstack;
    unsigned stack_index;
    unsigned currentcluster;
    char* buffer;
};

typedef struct
{
    unsigned filesize;
    unsigned position;
    struct fsdata *data;
} FileStream;

static BPBMain bpb;
static BPB16 bpb16;
static BPB32 bpb32;
static char fat[1024]; // FAT sector cache, supports 512 byte sectors (space to read 2 sectors in case of fat12)
static char disk[0x10000]; // up to 64k clusters (standard is 32k), some drivers 64k
static struct file_entry file_info; // Current file entry
static struct file_entry dir_info; // Currently handled directory entry
static struct file_entry cdir_info; // Current (user) directory entry
static char current_dir[256]; // Current directory pathname
static int root_sectors; // Root directory sectors (FAT16)
static int data_start; // First data sector
static int root_start; // First root directory sector
static int cluster_size; // Size of a cluster
static int cur_fat_sec; // Cached FAT sector
static int count_of_clusters; // Cluster count
static int fat_type; // fat16 or fat32

static int read_cluster(int, void**);
static int write_cluster(int, void*);
static int _read_file_info(const char*,int);
static int trace_info(const char*, int);
static int read_file_info(const char*);
static int read_dir_info(const char*);
static int read_dir(const char* dir);
static int read_root_file(const char*, int is_dir);
static int dot_to_83(const char*, char*);
static int streq(const char*, const char*);
static void read_sector(int sector, void** buffer);
static void write_sector(int, void*);
static int find_file(const char*, int size, int attributes);
static unsigned int next_cluster(int cluster);
static unsigned int alloc_cluster(unsigned int);
static int set_next_cluster(unsigned int, unsigned int);
static unsigned int alloc_next_cluster(unsigned int);
static inline int eof(unsigned);
static int push_cluster(FileStream *);
static int pop_cluster(FileStream *);

void fat_init();
int read_file(const char* filename, void* buffer);
unsigned int file_size(const char* filename);
int file_exists(const char* filename);
int directory_exists(const char* dirname);
int chdir(const char* dir);
const char* current_directory();
int  file_open(const char *filename, FileStream *fs);
void file_close(FileStream *fs);
int file_seek(FileStream *fs, unsigned pos);
unsigned file_read(FileStream *fs, void *buffer, unsigned bytes);
char file_getc(FileStream *fs);
char file_peekc(FileStream *fs);
int file_eof(FileStream *fs);

#ifdef __cplusplus
}  // extern "C"
#endif

#endif

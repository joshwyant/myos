#ifndef __fat_h__
#define __fat_h__

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
    unsigned filesize;
    unsigned position;
    unsigned reserved;
} FileStream;

// Whatever is defined here MUST be defined in FATC.H!

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

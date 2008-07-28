#ifndef __klib_h__
#define __klib_h__

// Include inline IO functions with klib
#include "io.h"
#include "loader_info.h"

// defines
#define PF_NONE		0
#define PF_READ		0
#define PF_SUPERVISOR	0
#define PF_PRESENT	1<<0
#define PF_WRITE	1<<1
#define PF_USER		1<<2
#define PF_WRITETHROUGH	1<<3
#define PF_CACHEDISABLE 1<<4
#define PF_ACCESSED	1<<5
#define PF_DIRTY	1<<6
#define PF_PAT		1<<7
#define PF_GLOBAL	1<<8
#define PF_AVAIL1	1<<9
#define PF_AVAIL2	1<<10
#define PF_AVAIL3	1<<11

#define PF_LOCKED	PF_AVAIL1


// Structures



// Functions

// (crude) memory management
void* kmalloc(int);
void kfree(void*);
void* findpage();

// exceptions
extern void register_isr(int num, void* offset);
extern void register_trap(int num, void* offset);
extern void register_task(int num, unsigned short selector);
static inline void kmemcpy(void* dest, const void* src, unsigned bytes)
{
    asm volatile (
        "cld; rep; movsb":
        "=c"(bytes),"=S"(src),"=D"(dest):
        "c"(bytes),"S"(src),"D"(dest)
    );
}
static inline void kzeromem(void* dest, unsigned bytes)
{
    asm volatile (
        "cld; rep; stosb":
        "=c"(bytes),"=D"(dest):
        "c"(bytes),"D"(dest),"a"(0)
    );
}
// various
static inline char ktoupper(char c)
{
    if ((c < 'a') || (c > 'z')) return c;
    return c-'a'+'A';
}
static inline char ktolower(char c)
{
    if ((c < 'A') || (c > 'Z')) return c;
    return c-'A'+'a';
}
int kstrlen(char* str);
extern const char* ksprintf(char* dest, const char* format, ...);
extern const char* ksprinthexb(char*, char);
extern const char* ksprinthexw(char*, short);
extern const char* ksprinthexd(char*, int);
extern const char* ksprintdec(char*, int);
extern const char* kstrcpy(char*, const char*);
extern const char* kstrcat(char*, const char*);
extern int kstrcmp(const char*, const char*);

typedef struct {
    int Second;
    int Minute;
    int Hour;
    int Day;
    int Month;
    int Year;
} DateTime;

DateTime get_time();
void ksprintdatetime(char* dest, DateTime dt);\

extern void* pgdir;
extern void* next_page;

// Globals

// elf.c
extern int load_kernel(loader_info *li);
extern char *elf_last_error();

#endif

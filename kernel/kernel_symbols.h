#ifndef __KERNEL_SYMBOLS_H__
#define __KERNEL_SYMBOLS_H__

#include "klib.h"

// Kernel symbol table

typedef struct
{
    char *name;
    void *ptr;
} kernel_symbol;

// Keep this list in alphabetical order.
const kernel_symbol kernel_symbols[] = 
{
    {"base_alloc",	&base_alloc},
    {"elf_last_error",	&elf_last_error},
    {"extended_alloc",	&extended_alloc},
    {"get_physaddr",	&get_physaddr},
    {"kfindrange",	&kfindrange},
    {"kfree",		&kfree},
    {"kmalloc",		&kmalloc},
    {"ksprintdec",	&ksprintdec},
    {"ksprintf",	&ksprintf},
    {"ksprinthexb",	&ksprinthexb},
    {"ksprinthexd",	&ksprinthexd},
    {"ksprinthexw",	&ksprinthexw},
    {"kstrcat",		&kstrcat},
    {"kstrcmp",		&kstrcmp},
    {"kstrcpy",		&kstrcpy},
    {"kstrlen",		&kstrlen},
    {"page_alloc",	&page_alloc},
    {"page_free",	&page_free},
    {"page_map",	&page_map},
    {"page_unmap",	&page_unmap},
    {"process_create",	&process_create},
    {"process_enqueue",	&process_enqueue},
    {"process_start",	&process_start},
    {"register_isr",	&register_isr},
    {"register_task",	&register_task},
    {"register_trap",	&register_trap},
};

#endif /* __KERNEL_SYMBOLS_H__ */

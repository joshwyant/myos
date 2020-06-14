#ifndef __KERNEL_H__
#define __KERNEL_H__

#include "../include/VESA.h"
#include "../include/loader_info.h"
#include "VESA.h"
#include "clock.h"
#include "disk.h"
#include "drawing.h"
#include "elf.h"
#include "exceptions.h"
#include "fat.h"
#include "interrupt.h"
#include "keyboard.h"
#include "klib.h"
#include "memory.h"
#include "mouse.h"
#include "process.h"
#include "string.h"
#include "syscall.h"
#include "task.h"
#include "timer.h"
#include "video.h"

// main
extern loader_info loaderInfo;
extern void kmain(loader_info *li);
extern void init_loader_info();
extern void init_symbols(loader_info *li);

// initialization
extern void show_splash();
extern void start_shell();

// Misc
extern Elf32_Sym *find_symbol(const char* name);
extern void invoke(const char* function); // random fun test function (invoke("kmain"))

#endif  // __KERNEL_H__

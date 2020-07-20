#ifndef __KERNEL_H__
#define __KERNEL_H__

#include <stddef.h>
#include "../include/VESA.h"
#include "../include/loader_info.h"
#include "VESA.h"
#include "clock.h"
#include "deque.h"
#include "disk.h"
#include "drawing.h"
#include "elf.h"
#include "error.h"
#include "exceptions.h"
#include "fat.h"
#include "forward_list.h"
#include "fs.h"
#include "fpu.h"
#include "interrupt.h"
#include "list.h"
#include "keyboard.h"
#include "klib.h"
#include "map.h"
#include "memory.h"
#include "mouse.h"
#include "pool.h"
#include "priority_queue.h"
#include "process.h"
#include "string.h"
#include "syscall.h"
#include "task.h"
#include "timer.h"
#include "vector.h"
#include "video.h"

#ifdef __cplusplus

#include <new>

_GLIBCXX_NODISCARD void* operator new(std::size_t) _GLIBCXX_THROW (std::bad_alloc);
 
_GLIBCXX_NODISCARD void* operator new[](std::size_t) _GLIBCXX_THROW (std::bad_alloc);
 
void operator delete(void*) _GLIBCXX_USE_NOEXCEPT;
 
void operator delete[](void*) _GLIBCXX_USE_NOEXCEPT;

extern "C" {
#endif

// main
extern loader_info loaderInfo;
extern void init_loader_info();
extern void init_symbols(loader_info *li);

// initialization
extern void show_splash();
extern void start_shell();

// Misc
extern void invoke(const char* function); // random fun test function (invoke("kmain"))

#ifdef __cplusplus
}  // extern "C"
#endif

#endif  // __KERNEL_H__

#ifndef __KERNEL_H__
#define __KERNEL_H__

#include <stddef.h>
#include "../include/VESA.h"
#include "../include/loader_info.h"
#include "VESA.h"
#include "buffer.h"
#include "clock.h"
#include "deque.h"
#include "disk.h"
#include "drawing.h"
#include "drivers.h"
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
#include "priority_queue.h"
#include "process.h"
#include "queue.h"
#include "stack.h"
#include "string.h"
#include "string_buffer.h"
#include "syscall.h"
#include "task.h"
#include "timer.h"
#include "vector.h"
#include "video.h"

#ifdef __cplusplus

void* operator new(std::size_t);
 
void* operator new[](std::size_t);
 
void operator delete(void*);
 
void operator delete[](void*);

extern "C" {
#include <memory>
#include "fs.h"

#endif

// main
extern loader_info loaderInfo;
extern void init_loader_info();

#ifdef __cplusplus
}  // extern "C"

// initialization
extern void show_splash(
    std::shared_ptr<kernel::GraphicsDriver> graphics_driver,
    std::shared_ptr<kernel::FileSystemDriver> fs_driver);
extern void start_shell(std::shared_ptr<kernel::FileSystemDriver> fs_driver);
#endif

#endif  // __KERNEL_H__

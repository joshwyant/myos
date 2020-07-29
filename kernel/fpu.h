#ifndef __KERNEL_FPU__
#define __KERNEL_FPU__

#include "process.h"

#ifdef __cplusplus
extern "C" {
#endif

void init_fpu();
void fpu_task_switch();

extern Process *fpu_process;

#ifdef __cplusplus
}  // __cplusplus
#endif

#endif //  __KERNEL_FPU__

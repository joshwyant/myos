
#include "io.h"
#include "memory.h"
#include "process.h"
#include "task.h"

volatile unsigned int __attribute__ ((aligned(8))) gdt[] = {
    0x00000000,0x00000000, // 0x0000 Unused - null descriptor
    0x0000FFFF,0x00CF9A00, // 0x0008 DPL0 code - kernel
    0x0000FFFF,0x00CF9200, // 0x0010 DPL0 data - kernel
    0x0000FFFF,0x00CFBA00, // 0x0018 DPL1 code
    0x0000FFFF,0x00CFB200, // 0x0020 DPL1 data
    0x0000FFFF,0x00CFDA00, // 0x0028 DPL2 code
    0x0000FFFF,0x00CFD200, // 0x0030 DPL2 data
    0x0000FFFF,0x00CFFA00, // 0x0038 DPL3 code - user
    0x0000FFFF,0x00CFF200, // 0x0040 DPL3 data - user
    0x00000000,0x00000000, // 0x0048 System TSS (initialized later)
    0x00000000,0x00000000, // 0x0050 VM8086 TSS
};

struct TSS system_tss, *vm8086_tss;

void init_gdt()
{
    lgdt(gdt, sizeof(gdt)); // Load global descriptor table
}

void init_tss()
{
    // Initialize the system TSS
    kzeromem(&system_tss, sizeof(system_tss));
    system_tss.ss0 = 16;
    system_tss.esp0 = 0xC0000000;
    gdt[18] = 0x00000067 | (((unsigned)&system_tss)<<16);
    gdt[19] = 0x00008900 | (((unsigned)&system_tss&0x00FF0000)>>16) | ((unsigned)&system_tss&0xFF000000);
    // Now the VM8086 TSS (a TSS with IO permission for all ports)
    vm8086_tss = kmalloc(sizeof(struct TSS)+8192);
    kzeromem(vm8086_tss, sizeof(struct TSS)+8192);
    vm8086_tss->ss0 = 16;
    vm8086_tss->esp0 = 0xC0000000;
    vm8086_tss->bitmap = sizeof(struct TSS);
    gdt[20] = 0x00002067 | (((unsigned)vm8086_tss)<<16);
    gdt[21] = 0x00008900 | (((unsigned)vm8086_tss&0x00FF0000)>>16) | ((unsigned)vm8086_tss&0xFF000000);
}

/* kernel.ld for MyOS v0.0.1, Josh Wyant */
OUTPUT(../bin/kernel)
OUTPUT_FORMAT(elf32-i386)
OUTPUT_ARCH(i386)
ENTRY(_start)
/* Input files */
INPUT (
./obj/startup.o ,
./obj/kernel.o ,
./obj/video.o ,
./obj/isr.o ,
./obj/disk.o ,
./obj/fat.o ,
./obj/elf.o
)
/* Sections */
SECTIONS
{
    .text : { *(.text) }
    .data : { *(.data) }
    .bss : { *(.bss) }
}

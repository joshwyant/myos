/* kernel.ld for MyOS v0.0.1, Josh Wyant */
OUTPUT(../bin/kernel.img)
OUTPUT_FORMAT(binary)
OUTPUT_ARCH(i386)
/* Must be first in kernel image! */
STARTUP("./obj/startup.o")
/* Input files */
INPUT (
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
    /* loaded to 0x100000 */
    . = 0xC0000000;
    kernel = . ;
    .text : { *(.text) }
    .data : { *(.data) }
    .rodata : { *(.rodata) }
    .bss : { *(.bss) }
    /* end of binary image, marks start of free space */
    image_end = . ;
}

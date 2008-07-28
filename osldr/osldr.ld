/* osldr.ld for MyOS v0.0.1, Josh Wyant */
OUTPUT(./obj/osldr)
OUTPUT_FORMAT(elf32-i386)
OUTPUT_ARCH(i386)
/* Must be first in loader image! */
STARTUP("./obj/init.o")
/* Input files */
INPUT (
./obj/osldr.o ,
./obj/video.o ,
./obj/isr.o ,
./obj/disk.o ,
./obj/fat.o ,
./obj/elf.o 
)
/* Sections */
SECTIONS
{
    . = 0x8000 ;
    loader = . ;
    .text : { *(.text) }
    .data : { *(.data) }
    _bstart = . ;
    .bss : { *(.bss) }
    _bend = . ;
    /* end of binary image, marks start of free space */
    image_end = . ;
}

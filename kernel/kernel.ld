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
./obj/dictionary.o ,
./obj/elf.o ,
./obj/clock.o ,
./obj/drawing.o ,
./obj/exceptions.o ,
./obj/interrupt.o ,
./obj/keyboard.o ,
./obj/memory.o ,
./obj/mouse.o ,
./obj/process.o ,
./obj/string.o ,
./obj/syscall.o ,
./obj/task.o ,
./obj/timer.o ,
./obj/vector.o ,
./obj/VESA.o
)
/* Sections */
SECTIONS
{
    .text : { *(.text) }
    .data : { *(.data) }
    .bss : { *(.bss) }
}

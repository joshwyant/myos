/* kernel.ld for MyOS v0.0.1, Josh Wyant */
OUTPUT(../bin/kernel)
OUTPUT_FORMAT(elf32-i386)
OUTPUT_ARCH(i386)
ENTRY(_start)
/* Input files */
INPUT (
./obj/startup.o ,
./obj/crti.o ,
./obj/crtbegin.o ,
./obj/kernel.o ,
./obj/VESA.o ,
./obj/buffer.o ,
./obj/video.o ,
./obj/clock.o ,
./obj/disk.o ,
./obj/drawing.o ,
./obj/elf.o ,
./obj/exceptions.o ,
./obj/fat.o ,
./obj/fs.o ,
./obj/fpu.o ,
./obj/interrupt.o ,
./obj/isr.o ,
./obj/kcpp.o ,
./obj/keyboard.o ,
./obj/memory.o ,
./obj/mouse.o ,
./obj/process.o ,
./obj/string.o ,
./obj/syscall.o ,
./obj/task.o ,
./obj/timer.o ,
./obj/vector.o ,
./obj/libgcc.a ,
./obj/libsupc++.a ,
./obj/crtend.o ,
./obj/crtn.o
)
/* Sections */
SECTIONS
{
    .text : { *(.text) }
    .eh_frame : ALIGN(4)
    {
        start_eh_frame = .;
        *(.eh_frame)
        QUAD(0)
    }
    .data : { *(.data) }
    .bss : { *(.bss) }
}

#include "io.h"
#include "interrupt.h"

volatile int __attribute__ ((aligned(8))) idt [512];

// Initialize the Interrupt Descriptor Table
void init_idt()
{
    // In BSS section - IDT initialized to 0s
    lidt((void*)idt,sizeof(idt)); // Load the IDT
}

// Initialize the Programmable Interrupt Controller
void init_pic()
{
    // Re-program Programmable Interrupt Controller,
    // setting Interrupt Request vectors to 0x20-0x2F.
    outb(0x20, 0x11); // start the initialization sequence
    io_wait();
    outb(0xa0, 0x11);
    io_wait();
    outb(0x21, 0x20); // define PIC vectors
    io_wait();
    outb(0xa1, 0x28);
    io_wait();
    outb(0x21, 4);    // continue initialization sequence
    io_wait();
    outb(0xa1, 2);
    io_wait();

    outb(0x21, 1);    // ICW4_8086
    io_wait();
    outb(0xa1, 1);
    io_wait();
    
    outb(0x21, 0xFB); // mask irqs except for cascade
    io_wait();
    outb(0xa1, 0xFF);
    io_wait();
}

// Add a normal ISR to the IDT
void register_isr(int num, int dpl, void* offset)
{
    volatile struct IDTDescr* gate;
    gate=&(((struct IDTDescr*)idt)[num]);
    gate->offset_1  = (unsigned short)(unsigned int)offset;
    gate->selector  = 8;
    gate->zero      = 0;
    gate->type_attr = 0x8E|(dpl<<5);
    gate->offset_2  = (unsigned short)(((unsigned int)offset)>>16);
}

// Add a trap gate to the IDT
void register_trap(int num, int dpl, void* offset)
{
    volatile struct IDTDescr* gate;
    gate=&(((struct IDTDescr*)idt)[num]);
    gate->offset_1  = (unsigned short)(unsigned int)offset;
    gate->selector  = 8;
    gate->zero      = 0;
    gate->type_attr = 0x8F|(dpl<<5);
    gate->offset_2  = (unsigned short)(((unsigned int)offset)>>16);
}

// Add a task gate to the IDT
void register_task(int num, int dpl, unsigned short selector)
{
    volatile struct IDTDescr* gate;
    gate=&(((struct IDTDescr*)idt)[num]);
    gate->offset_1  = 0;
    gate->selector  = selector;
    gate->zero      = 0;
    gate->type_attr = 0x85|(dpl<<5);
    gate->offset_2  = 0;
}

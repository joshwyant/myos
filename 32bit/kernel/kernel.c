#include "kernel.h"

void kmain()
{
    kernel_init();

    print(msg);
}

void kernel_init()
{
    // before initializing hardware that uses
    // ISRs, initialize the IDT.
    init_idt();
    // now initialize the keyboard.
    kbd_init();
    // Before using any video functions, initialize video.
    init_video();

    // clear the screen.
    cls();
}

void init_idt()
{
    int i;
    // remap Programmable Interrupt Controller
    PIC_remap(0x20,0x28);
    // zero all entries
    for (i = 0; i < 512; i++) idt[i] = 0;
    // mask all IRQs but IRQ1
    outb(0x21,0xFD);
    outb(0xa1,0xFF);
    // load idt
    lidt((void*)idt,sizeof(idt));
    // enable ints
    sti();
}

void kbd_init()
{
    // fill descriptor 0x21 (irq 1) for keyboard handler
    struct IDTDescr* gate;
    gate=&((struct IDTDescr*)idt)[0x21];
    gate->offset_1  = (unsigned short)(unsigned int)irq1;
    gate->selector  = 8;
    gate->zero      = 0;
    gate->type_attr = 0x8E;
    gate->offset_2  = (unsigned short)(((unsigned int)irq1)>>16);
    kbd_escaped = false;
    kbd_shift = false;
}

// called by wrapper keyboard_handler
void handle_keyboard()
{
    char scancode=inb(0x60);
    bool escaped = kbd_escaped;
    kbd_escaped = false;
    if (escaped)
    {
    }
    else switch ((unsigned char)scancode)
    {
    case 0x0E:
        print("\b\x20\b");
        break;
    case 0x1C:
        //print("\r\n");
        break;
    case 0x2A:
    case 0x36:
        kbd_shift = true;
        break;
    case 0xAA:
    case 0xB6:
        kbd_shift = false;
        break;
    case 0xE0:
        kbd_escaped = true;
        break;
    default:
        if (!(scancode&0x80))
        {
            char c = kbd_shift?kbd_uppercase[scancode]:kbd_lowercase[scancode];
            print_char(c);
        }
        break;
    }
    eoi(1);
}

bits 16

; enable the A20 gate with shell command "a20on"
mov si,cmda20on
mov ah,0
int 21h

cli ; clear interrupts

; load GDTR
lidt [idt48]
lgdt [gdt48]

; switch to protected mode
mov eax,cr0
or  eax,1
mov cr0,eax

mov ax,32
mov ds,ax
mov es,ax
mov ax,16
mov fs,ax
mov gs,ax
mov ss,ax
mov esp,00007C00h

;in 16 bit segment: 32 bit jmp far 8:(code32+40000h)
jmp dword 8:code32+40000h

; ALL 32-BIT CODE BEYOND THIS POINT
bits 32
code32:
call _initvideo
mov esi,message
mov bl,4
call prtstr
hlt32:
hlt
jmp hlt32

%include "common.inc"

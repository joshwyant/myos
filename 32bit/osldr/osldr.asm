cli
xor ax,ax
mov ss,ax
mov sp,0x7C00
sti
mov bx,0x3000
mov ds,bx
mov es,bx

mov si,str1
call prtstr
abc:
hlt
jmp abc
str1: db "Well, it's loaded!",0


mov [DriveNumber],dl
%ifdef DEBUG
mov si,titlestr
call prtstr
%endif
call a20_enable
call enter_unreal
call disk_init
call read_memmap
jmp  load_kernel
%include "init.inc"
%include "common.inc"
%include "data.inc"
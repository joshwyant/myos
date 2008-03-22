cli
xor ax,ax
mov ss,ax
mov sp,0x7C00
sti
mov bx,0x3000
mov ds,bx
mov es,bx
mov [DriveNumber],dl
%ifdef DEBUG
mov si,titlestr
call prtstr
%endif
call a20_enable
call enter_unreal
call disk_init
call show_splash
jmp  load_kernel
%include "init.inc"
%include "common.inc"
%include "data.inc"
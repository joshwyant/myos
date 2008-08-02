; MyOS version 0.0.1 Loader
; Copyright 2008 Josh Wyant

%include "defines.inc"

; Setup stack frame (SS:SP = 0000:7C00)
cli
 xor ax,ax
  mov ss,ax

 mov sp,0x7C00
sti

; Segments (DS = ES = 0x3000)
mov bx,0x3000
 mov ds,bx
 mov es,bx

; Save drive number
mov [DriveNumber],dl

%ifdef DEBUG
  ; Print greeting
  mov si,titlestr
  call prtstr
%endif

; Call initialization routines
call a20_enable		; Enable the A20 line
call enter_unreal	; Enter unreal mode
call disk_init		; Initialize disk access routines
call read_memmap	; Read the system memory map
%ifdef VESA
call init_vbe		; Init VESA VBE2
%endif
jmp  load_kernel	; Load and execute the kernel image

; Common code and data
%include "init.inc"	; Initialization routines
%include "common.inc"	; Common routines (disk, video, etc.)
%include "data.inc"	; Loader data

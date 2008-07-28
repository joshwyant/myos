; MyOS version 0.0.1 Loader
; Copyright 2008 Josh Wyant

bits 16

%include "defines.inc"

global _start

_start:

; Setup stack frame (SS:SP = 0000:7C00)
cli
 xor ax,ax
  mov ss,ax

 mov sp,0x7C00
sti

; Segments (DS = ES = 0x0000)
mov bx,0x0000
 mov ds,bx
 mov es,bx

; Save drive number
mov [DriveNumber],dl

; Clear the screen
call cls

; Call initialization routines
call a20_enable		; Enable the A20 line
call read_memmap	; Read the system memory map
jmp  init32		; Go into protected mode and finish up loading

; Common code and data
%include "init.inc"	; Initialization routines
%include "common.inc"	; Common routines (video, etc.)
%include "data.inc"	; Loader data

; OSLDR
; ----------------------------------------
; MyOS v0.0.1 x86 protected-mode OS-loader
; (C) 2008 Josh Wyant
; 
; Dependencies:
; - OSLDR (this file) was loaded to 3000:0000 (Real address mode)
; - The file system is FAT16
; - The drive number is stored in DL
;
; Compiles under NASM
; > nasm osldr.asm
; same as > nasm osldr.asm -fbin -o osldr
; Save as OSLDR to root directory of drive with MyOS boot sector.
; 

; bits 16

; At 3000:0000
%include "data.inc"

; 1. Setup stack frame
; 2. Setup DS and ES
; 3. Initialze disk
; 4. Show splash screen
; 5. Hook INT 21h
; 6. Print greeting
; 7. Start prompting

start:
xor ax,ax ; ax = 0
; setup frame
cli ; disable interrupts while setting up stack frame (clear Interrupt Flag)
mov ss,ax
mov sp,7c00h
sti ; allow interrupts
; segments
mov bx,3000h
mov ds,bx    ; DS = 3000h
mov es,bx    ; ES = 3000h

; init
call initdisk

; show splash screen
mov si,splashfile
call lookup
cmp cx,0
je nosplash
; set video mode
mov ax,13h
int 10h
push es
mov ax,4000h
mov es,ax
xor di,di
push es
call ReadFile
pop es
mov dx,3C8h
mov al,0
out dx,al ; set all pallette entries
xor di,di
mov cx,768
mov dx,3C9h
pallette:
mov al,[es:di]
inc di
out dx,al
loop pallette
push ds
mov ax,es
mov ds,ax
mov ax,0A000h
mov es,ax
mov si,768
xor di,di
mov cx,32000
rep movsw
pop ds
pop es
mov ah,0
int 16h
nosplash:

; Enable INT 21h services
push ds
xor ax,ax
mov ds,ax
mov word [84h],Int21
mov word [86h],3000h 
pop ds

mov si,titlestr
call prtstr

prmpt:
call prompt
jmp prmpt

%include "common.inc"
%include "command.inc"
%include "int21.inc"
%include "initdisk.asm"
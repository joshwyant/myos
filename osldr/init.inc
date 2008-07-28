; Defines for memory layout
%define IVT		0x00000000
%define BDA		0x00000400
%define SMAPPage	0x00001000
;define BootSect	0x00007C00
%define Loader		0x00008000

; Physical memory layout:
; 0x00000000 - 0x000003FF IVT
; 0x00000400 - 0x000004FF BIOS data area
; 0x00000500 - 0x00000FFF Free space
; 0x00001000 - 0x00001FFF Kernel stack
; 0x00002000 - 0x00002FFF System memory map
; 0x00003000 - 0x00003FFF Initial Page directory
; 0x00004000 - 0x00004FFF Initial Page table 0   (0x00000000)
; 0x00005000 - 0x00005FFF Initial Page table 768 (0xC0000000)
; 0x00006000 - 0x00006FFF Initial Page table 960 (0xF0000000)
; 0x00007000 - 0x00007FFF Boot sector
; 0x00008000 - 0x0009FBFF Free space
; 0x0009C000 - 0x000FFFFF Reserved
; 0x00100000 - Kernel End Kernel
; Kernel End - 0x00FFFFFF ISA DMA memory, free memory
; 0x01000000 - 0xFFFFFFFF Free space
; Paged layout:
; 0xC0000000 - 0xCFFFFFFF Kernel code, data, bss, etc.
; 0xD0000000 - 0xDFFFFFFF Kernel heap
; 0xE0000000 - 0xEFFFFFFF 
; 0xF0000000 - 0xFFBFFFFF Reserved for phys. mem. mapping
; 0xFFC00000 - 0xFFFFFFFF Page tables
; 0xFFFFF000 - 0xFFFFFFFF Page directory
; Mapped at high 256MB:
; 0xF0000000 - 0xF0000FFF Kernel Stack
; 0xF0001000 - 0xF0001FFF System memory map

a20_enable:
; 1: ax=2401 int 15
; 2: set bit 2 port 92h
; 3: use keyb controller

; is it already enabled?
call a20_test
jnc .1

; Try method 1
mov ax,2401h
int 15h
call a20_test
jnc .1

; Try method 2
in al,92h
test al,2
jnz .0
or al,2
and al,0FEh
out 92h,al
call a20_test
jnc .1
.0:

; Try method 3
cli
call kbd_wait
mov al,0ADh
out 64h,al
call kbd_wait
mov al,0D0h
out 64h,al
call kbd_wait1
in al,60h
push ax
call kbd_wait
mov al,0D1h
out 64h,al
call kbd_wait
pop ax
or al,2
out 60h,al
call kbd_wait
mov al,0AEh
out 64h,al
;call kbd_wait
sti
call a20_test
jnc .1

; No method worked.
mov si,a20err
call prtstr
call halt

.1:
ret

; cf: 1=a20 disabled, 0=a20 enabled
a20_test:
mov ax,0
mov fs,ax
mov ax,0FFFFh
mov gs,ax
mov cx,2
.0:
mov ax,[fs:7DFEh]
cmp ax,[gs:7E0Eh]
jne .1
rol word [fs:7DFEh],8
loop .0
stc
ret
.1:
mov word [fs:7DFEh],0AA55h
clc
ret

read_memmap:
; read memory map to 0:SMAPPage
push es
xor ebx,ebx ; continuation value
mov es,bx
mov di,SMAPPage
.1: ; int 15h ax=E820h
mov edx,534D4150h ; 'SMAP'
mov ecx,20
mov eax,0E820h
int 15h
jc .1.1 ; test what carry flag means
cmp eax,534D4150h
jne .4 ; Error!
add edi,ecx
cmp ecx, 24
je .1.2
mov eax,1
stosd
.1.2:
test ebx,ebx
jz .3 ; finished
jmp .1 ; continue
.1.1:
cmp eax,534D4150h ; must be this
jne .2 ; doesn't work
test ebx,ebx ; must be nonzero
jz .2 ; doesn't work
; carry means finished
; because ebx is nonzero
; and eax = 'SMAP', so 
; finish
add edi,ecx
cmp ecx,24
je .1.1.1
mov eax,1
stosd
.1.1.1:
jmp .3
.2: ; get amount of ram with E801h to emulate E820h
mov ax,0xE801
int 15h
mov eax,0x100000
stosd
xor eax,eax
stosd
movzx eax,dx
shl eax,16
add eax,0xF00000
stosd
xor eax,eax
stosd
mov eax,1
stosd
stosd
.3: ; finish; write zero entry
xor eax,eax
mov cx,6
rep stosd
pop es
ret
.4: ; error
pop es
mov si,err_smap
call prtstr
call halt
ret

init32:
; Get ready for protected mode
cli ; clear interrupts
; load GDTR
lidt [idt48]
lgdt [gdt48]
; switch to protected mode (without paging)
mov eax,cr0
or  eax,0x00000001
mov cr0,eax
; invalidate ldt
xor eax,eax
lldt ax
; set segments and stack
mov ax,16
mov ds,ax
mov es,ax
mov fs,ax
mov gs,ax
mov ss,ax
movzx esp,sp ; the stack
extern loader_main
call dword 8:loader_main ; dword here means jump to 32bit code
bits 32
hlt
jmp $-1
bits 16

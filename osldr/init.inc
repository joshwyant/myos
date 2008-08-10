; Defines for memory layout
%define IVT		0x00000000
%define BDA		0x00000400
%define KernelStackP	0x00001000
%define SMAPPage	0x00002000
%define PgDir		0x00003000
%define PgTbl0		0x00004000
%define PgTbl768	0x00005000
%define PgTbl960	0x00006000
;define BootSect	0x00007C00
%define VESAInfo	0x00008000
%define VESAMode	0x00008200
%define Loaded		0x00030000
;define OSVirtualAddr	0xC0000000
%define KernelStackL	0xF0000000
%define KernelStackSize	0x00001000
%ifndef XRES
%define XRES 1024
%define YRES 768
%endif

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

%ifdef DEBUG
mov si,msg_a20
call prtstr
%endif

; is it already enabled?
%ifdef DEBUG
mov si,a20_gate
%endif
call a20_test
jnc a20ret

; Try method 1
%ifdef DEBUG
mov si,a20_bios
%endif
mov ax,2401h
int 15h
call a20_test
jnc a20ret

; Try method 2
%ifdef DEBUG
mov si,a20_fast
%endif
in al,92h
test al,2
jnz no92
or al,2
and al,0FEh
out 92h,al
call a20_test
jnc a20ret
no92:

; Try method 3
%ifdef DEBUG
mov si,a20_keyb
%endif
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
jnc a20ret

; No method worked.
mov si,a20err
call prtstr
call halt

a20ret:
%ifdef DEBUG
call prtstr
%endif
ret

; cf: 1=a20 disabled, 0=a20 enabled
a20_test:
mov ax,0
mov fs,ax
mov ax,0FFFFh
mov gs,ax
mov cx,2
testing_a20:
mov ax,[fs:7DFEh]
cmp ax,[gs:7E0Eh]
jne a20_enabled
rol word [fs:7DFEh],8
loop testing_a20
stc
ret
a20_enabled:
mov word [fs:7DFEh],0AA55h
clc
ret

enter_unreal:
pusha
%ifdef DEBUG
mov si,msg_unr
call prtstr
%endif
cli                      ; no interrupts
push ds                  ; save ds
push es
lgdt [gdtunreal] ; load unreal gdt
mov eax,cr0
or al,1
mov cr0,eax              ; set pmode bit
mov bx,8
mov ds,bx                ; load descriptor
mov es,bx
and al,0xFE
mov cr0,eax              ; back to real mode
pop es
pop ds                   ; recall ds
sti                      ; allow interrupts
popa
ret

read_memmap:
%ifdef DEBUG
mov si,msg_mem
call prtstr
%endif
; read memory map to 0:2000h
push es
xor ebx,ebx ; continuation value
mov es,bx
mov di,SMAPPage
.1: ; int 15h ax=E820h
mov edx,534D4150h ; 'SMAP'
mov ecx,20
mov eax,0E820h
int 15h
jc .11 ; test what carry flag means
cmp eax,534D4150h
jne .4 ; Error!
add edi,ecx
cmp ecx, 24
je .12
mov eax,1
stosd
.12:
test ebx,ebx
jz .3 ; finished
jmp .1 ; continue
.11:
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
je .111
mov eax,1
stosd
.111:
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



disk_init:
%ifdef DEBUG
mov si,msg_dsk
call prtstr
%endif
; Copy BPB
push ds
mov bx,07C0h
mov ds,bx
xor si,si
mov di,BPB
mov cx,45
cld
rep movsw
pop ds
call testlba
; calculate root sectors
mov cx,word[BytesPerSector]
mov ax,32
mul word [RootEntries]
add ax,cx
dec ax
div cx
movzx ecx,ax ; ecx = root dir sectors
mov [RootSectors],cx
; calculate root location
movzx eax,byte[NumFATs]
movzx ebx,word[FATSectors16]
and bx,bx
jnz .1
mov ebx,[FATSectors32]
.1:
mul ebx
movzx ebx,word [ReservedSectors]
add eax,ebx ; eax = root dir
mov [RootSector],eax
; Calculate First data sector
add eax,ecx
mov [FirstDataSector], eax ; First data sector
; Determine whether FAT16 or FAT32
mov ebx,eax
movzx eax,word[TotalSectors16]
and ax,ax
jnz .2
mov eax,[TotalSectors32]
.2:
sub eax,ebx ; eax = data sectors
movzx ebx,byte [SectorsPerCluster]
xor edx,edx
div ebx
cmp eax,65525
jc .3
mov word[Lookup],lookup32
mov word[MaskCluster],MaskCluster32
mov word[CompareCluster],CompareCluster32
mov word[FATOffset],FATOffset32
ret
.3:
mov byte[IsFAT16],1
mov word[Lookup],lookup16
mov word[MaskCluster],MaskCluster16
mov word[CompareCluster],CompareCluster16
mov word[FATOffset],FATOffset16
ret

; testlba
testlba:
%ifdef DEBUG
mov si,msg_lba
call prtstr
%endif
mov dl,[DriveNumber]
mov ah,41h
mov bx,55AAh
int 13h
jc testlbafail
cmp bx,0AA55h
jne testlbafail
test cl,1
jz testlbafail
ret
testlbafail:
mov si,LBAFailStr
call prtstr
call halt

%ifdef VESA
init_vbe:
%ifdef DEBUG
mov si,msg_vbe
call prtstr
%endif
push es
xor ax,ax
mov es,ax
mov di,VESAInfo
mov ebx,[ds:vbe2]
mov [es:di],ebx
mov ax,0x4F00
int 10h
cmp ax,4fh
jne .novbe
mov di,[es:VESAInfo+14]
mov ax,[es:VESAInfo+16]
mov es,ax
.0:
mov cx,[es:di]
cmp cx,0ffffh
je .novbe
push di
mov di,VESAMode
mov ax,4f01h
int 10h
test byte[es:di],1
jz .1
cmp word[es:di+18], XRES
jne .1
cmp word[es:di+20], YRES
jne .1
cmp byte[es:di+25], 24
jge .2

.1:
pop di
add di,2
jmp .0


.novbe:
pop es
ret

.2:
pop di
pop es ; es
mov bx,0x4000
or bx,cx
mov ax,4f02h
int 10h
ret
%endif


load_kernel:
%ifdef DEBUG
mov si,msg_knl
call prtstr
%endif
; copy to 0x100000
xor bx,bx
mov es,bx
mov edi,0x100000
; read kernel.img
mov si,kernelimg
call ReadFile
%ifdef DEBUG
mov si,msg_jmp
call prtstr
%endif
; create initial page table and page directory
;     wipe the tables
mov di,0x3000
mov eax,0
mov cx,4096 ; 4 tables x 1024 entries
rep stosd
; Temporarily identity map first megabyte
mov di,PgTbl0
mov cx,256
mov eax,0x00000000|PF_PRESENT|PF_WRITE
paging1:
stosd
add eax,0x00001000
loop paging1
; Map kernel to 0xC0000000
mov di,PgTbl768
; number of pages in kernel: (kernel_end-0xC0000000+0xFFF)/0x1000
; THE BUG: USE ES REGISTER!
mov ecx,[dword es:0x00100080]
add ecx,0x0FFF
shr ecx,12
; poke in entries
mov eax,0x00100000|PF_PRESENT|PF_WRITE|PF_LOCKED
paging2:
stosd
add eax,0x00001000
loop paging2
mov dword [es:PgDir+0*4], PgTbl0|PF_WRITE|PF_PRESENT ; Identity map first megabyte (temporarily)
mov dword [es:PgDir+768*4], PgTbl768|PF_WRITE|PF_PRESENT ; set 768th entry of PDT to address of its table
mov dword [es:PgDir+960*4], PgTbl960|PF_WRITE|PF_PRESENT ; set 960th entry of PDT to address of its table
mov dword [es:PgDir+1023*4], PgDir|PF_WRITE|PF_PRESENT|PF_LOCKED ; map the page directory into itself
mov dword [es:PgTbl960+0*4], KernelStackP|PF_WRITE|PF_PRESENT|PF_LOCKED ; Map the stack to 0xF0000000
mov dword [es:PgTbl960+1*4], SMAPPage|PF_WRITE|PF_PRESENT|PF_LOCKED ; Map the system memory map to 0xF0001000
mov eax,PgDir
mov cr3,eax ; load CR3 with address of page directory
; Get ready for protected mode
cli ; clear interrupts
; load GDTR
lidt [idt48]
lgdt [gdt48]
; switch to protected mode and enable paging
mov eax,cr0
or  eax,0x80000001
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
mov esp,KernelStackL + KernelStackSize ; the stack
jmp dword 8:0xC0000000 ; dword here means jump to 32bit code
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
; copy FAT to 1000:0000
%ifdef DEBUG
mov si,msg_fat
call prtstr
%endif
push es
mov ax,1000h
mov es,ax
xor edi,edi
xor dx,dx
mov ax,[ReservedSectors]
mov cx,[FATSectors16]
call ReadSectors
%ifdef DEBUG
mov si,msg_rtd
call prtstr
%endif
; calculate root sectors
mov cx,word[BytesPerSector]
mov ax,32
mul word [RootEntries]
add ax,cx
dec ax
div cx
mov cx,ax ; cx = root dir sectors
; calculate root location
movzx ax,[NumFATs]
mul word [FATSectors16]
add ax,[ReservedSectors] ; dx:ax = root dir
push ax
add ax,cx
mov [FirstDataSector], ax ; First data sector
; copy ROOT to 2000:0000
mov ax,2000h
mov es,ax
pop ax
xor edi,edi
call ReadSectors
pop es
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

show_splash:
ret

load_kernel:
%ifdef DEBUG
mov si,msg_knl
call prtstr
%endif
xor bx,bx
mov es,bx
mov edi,0x100000
mov si,kernelimg
call ReadFile
%ifdef DEBUG
mov si,msg_jmp
call prtstr
%endif
cli ; clear interrupts
; load GDTR
lidt [idt48]
lgdt [gdt48]
; switch to protected mode
mov eax,cr0
or  al,1
mov cr0,eax
mov ax,16
mov ds,ax
mov es,ax
mov fs,ax
mov gs,ax
mov ss,ax
mov esp,0x9FC00
jmp dword 8:0x100000
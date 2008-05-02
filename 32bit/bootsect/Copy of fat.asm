; 0000:7C00 = Stack top
; 07C0:0000 = Boot sector
; 07E0:0000 = Globals
;     :0000 = DAP
;     :0010 = Other globals
; 1000:0000 = FAT
; 2000:0000 = Root directory
; 3000:0000 = OS loader
; ...

%include "const.inc"

; ========================
; Boot Sector
; ========================

%include "bpb.inc"

; ===================================
; Boot sector code
; ===================================

start:
xor ax,ax ; ax = 0
; Setup stack
cli
mov ss,ax    ; SS = 0
mov sp,7C00h ; SP = 7C00h
sti

; Set data and extra segments
mov bx,07C0h
mov ds,bx   ; DS = 7C0h
mov es,bx   ; ES = 7C0h

; init bss
mov cx,8                     ; 8 words
mov di,Packet	             ; Packet
rep stosw                    ; Zero memory es:di (ax = 0)
mov [DriveNum], dl                 ; DL = drive number, save it
mov [PacketSize], byte 16      ; Struct size = 16
mov [PacketNumSectors], byte 1 ; 1 Sector

; Check LBA support
mov bx,55AAh  ; must be this
mov ah,41h    ; check lba function
int 13h
jc err       ; error
cmp bx,0AA55h ; bx holds this if it's enabled
jne err
test cl,1     ; cx bit 1 means disk access using packet structure is allowed
jz err

; Calculate number of root directory sectors
mov cx,[bpbBytesPerSector]
mov ax,32
mul word [bpbRootEntries]
add ax,cx
dec ax
div cx
mov cx,ax ; cx = number of root directory sectors

; Calculate first data sector
movzx ax,[bpbNumFATs]
mul      word [bpbFATSectors16]
add   ax,[bpbReservedSectors] ; dx:ax = root directory location
push  ax
add   ax,cx
mov   [FirstDataSector],ax ; first data sector

; Search for "OSLDR"
; cx    = number of sectors to go through
; dx:ax = current root sector
mov ax,2000h
mov es,ax
pop ax
readroot:
push cx
mov cx,1
xor di,di
call ReadSectors
pop cx
xor bx,bx
push ax
search:
mov al,[es:bx]
test al,al
jz err ; No more files to search
mov al,[es:bx+11] ; flags
test al,18h
jnz cont ; continue if dir or volLabel
push cx
mov cx,11 ; ll characters
mov si,filename
mov di,bx
rep cmpsb
pop cx
jz found

; not found
cont:
add bx,32
cmp bx,[bpbBytesPerSector]
je nextRootSec
jmp search

err:
mov si,errstr
mov ah,0Eh
mov bx,7
prtchr:
lodsb
test al,al
jz prtstrend
int 10h
jmp prtchr
prtstrend:
.0:
hlt
jmp .0

nextRootSec:
pop ax
inc ax
loop readroot
jmp err ; no more root sectors to read

found:
; copy "OSLDR" to 3000:0000
mov si,[es:bx+26] ; first cluster
mov ax,3000h
mov es,ax
xor di,di
push es ; prepare retf
push di ;
found0:
call ReadCluster
cmp si,0fff8h
jc found0
mov dl,[DriveNum] ; Drive number
retf ; Go to loaded file

; ================================================
; Function: ReadSectors
;
; es:di = destination
; dx:ax = start sector (lba, 1st sector = 0)
; cx    = number of sectors to read
;
; ================================================

ReadSectors:
push si
push cx
; get ready
mov si,200h ; Packet
; add hidden sectors to dx:ax
add ax, [bpbHiddenSectors]
adc dx, [bpbHiddenSectors+2]

read0:
; Read sector
mov [PacketOffset] ,di ; Destination offset
mov [PacketSegment],es ; Destination segment
mov [PacketLBA0]   ,ax ; LBA Lo-word
mov [PacketLBA16]  ,dx ; LBA Hi-word
mov dl,[DriveNum]      ; Drive number
mov ah,42h       ; Extended read function
int 13h
jc err           ; Halt on error

; advance sector number and destination
add di, [bpbBytesPerSector]
jnc read1 ; if we reach end of ES segment, advance it
mov ax,es
add ax,1000h
mov es,ax
read1: ; Add 1 to the sector number
mov ax, [PacketLBA0]  ; LBA lo
mov dx, [PacketLBA16] ; LBA hi
add ax,1
adc dx,0

; repeat the number of sectors
loop read0
; return
pop cx
pop si
ret

; ===================================
; Function: ReadCluster
;
; es:di  = destination
; si     = cluster number
; returns next cluster in si
; ===================================
ReadCluster:
push bp
mov bp,sp
sub sp,2
mov ax,si
sub ax,2  ; Subtract unused sectors
jb err   ; Given cluster Must be >= 2!
movzx cx,[bpbSectorsPerCluster]
mul cx
add ax,[FirstDataSector] ; Add first data sector
adc dx,0         ; dx:ax = first sector of cluster, cx = SecPerClus, es:di = dest
call ReadSectors
; Find next sector
push es
pusha
mov ax,1000h
mov es,ax
xor di,di
xor dx,dx
; How to compute the FAT sector and offset
%ifdef FAT16
shl si,1 ; entry*2 = offset
mov ax,si
%else
mov ax,si
shr si,1
add ax,si ; entry+entry/2 = offset
%endif
div word [bpbBytesPerSector]
add ax,[bpbReservedSectors] ; dx=offset, ax=sector
push dx
cmp ax,[FATSector]
je .noreadfat
mov [FATSector],ax
xor dx,dx
; Read 2 sectors for FAT12, 1 for FAT16
%ifdef FAT12
mov cx,2
%else
mov cx,1
%endif
call ReadSectors
.noreadfat:
pop di ; was push dx
mov si,[es:di] ; next cluster is in si
; For FAT12, the value read needs to be modified
%ifdef FAT12
test di,1
jz .1
; odd
shr si,4
jmp .2
.1:
; even
and si,0xFFF
.2:
%endif
mov [bp-2],si ; save si before popping registers
popa
pop es
mov si,[bp-2] ; get saved si
leave
ret

; ==========================
; strings
; ==========================
errstr: db 'BOOT FAILURE!',0
filename: db "OSLDR      "

; ==========================
; end
; ==========================

; boot sector magic number at org 510
times 510-($-$$) db 0
dw 0AA55h

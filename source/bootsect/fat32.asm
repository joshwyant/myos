; 0000:7C00 = Stack top
; 07C0:0000 = Boot sector
; 07E0:0000 = Globals
;     :0000 = DAP
;     :0010 = Other globals
; 1000:0000 = FAT
; 2000:0000 = Root directory
; 3000:0000 = OS loader
; ...

org 0x7C00
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
mov ds,ax    ; DS = 0
mov es,ax    ; ES = 0
mov sp,7C00h ; SP = 7C00h
sti

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

; Calculate first data sector
movzx eax,byte [bpbNumFATs]
mul      dword [bpbFATSectors32]
movzx ecx,word [bpbReservedSectors]
add   eax,ecx
mov   [FirstDataSector],eax ; first data sector

; Search for "OSLDR"
mov esi,[bpbRootClus]
mov bx,2000h
mov es,bx
readroot:
xor di,di
call ReadCluster
push esi
xor bx,bx
search:
mov al,[es:bx]
test al,al
jz err ; No more files to search
mov al,[es:bx+11] ; flags
test al,18h
jnz cont ; continue if dir or volLabel
mov cx,11 ; ll characters
mov si,filename
mov di,bx
rep cmpsb
jz found

; not found
cont:
add bx,32
movzx ax,[bpbSectorsPerCluster]
mul word [bpbBytesPerSector]
cmp bx,ax
je nextRootClus
jmp search

err:
;mov si,errstr
;mov ah,0Eh
;mov bx,7
;prtchr:
;lodsb
;test al,al
;jz prtstrend
;int 10h
;jmp prtchr
;prtstrend:
.0:
hlt
jmp .0

nextRootClus:
pop esi
cmp esi,0x0FFFFFF8
jc readroot
jmp err ; no more root clusters to read

found:
; esi was pushed but, no need to get rid of it, the stack has reached the end of its life
; copy "OSLDR" to 3000:0000
mov si,[es:bx+0x14] ; first cluster high
shl esi,16
mov si,[es:bx+0x1A] ; first cluster low
mov ax,3000h
mov es,ax
xor di,di
push es ; prepare retf
push di ;
found0:
call ReadCluster
cmp esi,0x0FFFFFF8
jc found0
mov dl,[DriveNum] ; Drive number
retf ; Go to loaded file

; ================================================
; Function: ReadSectors
;
; es:di   = destination
; edx:eax = start sector (lba, 1st sector = 0)
; cx      = number of sectors to read
;
; ================================================

ReadSectors:
push si
; get ready
; add hidden sectors to edx:eax
add eax, [bpbHiddenSectors]
adc edx, 0
mov [PacketLBA0],  eax
mov [PacketLBA32], edx
.0:
; Read sector
mov si,7E00h ; Packet
mov [PacketOffset] ,di ; Destination offset
mov [PacketSegment],es ; Destination segment
mov dl,[DriveNum]      ; Drive number
mov ah,42h       ; Extended read function
int 13h
jc err
; advance sector number and destination
add di, [bpbBytesPerSector]
jnc .1 ; if we reach end of ES segment, advance it
mov ax,es
add ax,1000h
mov es,ax
.1: ; Add 1 to the sector number
add dword [PacketLBA0],  1 ; LBA lo
adc dword [PacketLBA32], 0 ; LBA lo
loop .0
pop si
ret

; ===================================
; Function: ReadCluster
;
; es:di  = destination
; esi    = cluster number
; returns next cluster in esi
; ===================================
ReadCluster:
push bp
mov bp,sp
sub sp,4
mov eax,esi
sub eax,2  ; Subtract unused sectors
jb err   ; Given cluster Must be >= 2!
movzx ecx,byte [bpbSectorsPerCluster]
mul ecx
add eax,[FirstDataSector] ; Add first data sector
call ReadSectors
; Find next sector
push es
pushad
mov ax,1000h
mov es,ax
xor di,di
; FAT sector, offset
shl esi,2 ; entry*4 = offset
mov eax,esi
movzx ecx, word [bpbBytesPerSector]
xor edx,edx
div ecx
movzx ecx,word [bpbReservedSectors]
add eax,ecx ; edx=offset, eax=sector
push edx
cmp eax,[FATSector]
je .noreadfat
mov [FATSector],eax
xor dx,dx
mov cx,1
call ReadSectors
.noreadfat:
pop edx
mov esi,[es:edx] ; next cluster is in esi
mov [bp-4],esi ; save si before popping registers
popad
pop es
mov esi,[bp-4] ; get saved si
and esi,0x0FFFFFFF
leave
ret

; ==========================
; strings
; ==========================
;errstr: db 'BOOT FAILURE!',0
filename: db "OSLDR      "

; ==========================
; end
; ==========================

; boot sector magic number at org 510
times 510-($-$$) db 0
dw 0AA55h

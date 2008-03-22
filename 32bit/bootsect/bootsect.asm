; 07C0:0000 = OS
; 07E0:0000 = locals
;     :0000 = DAP
;     :0010 = other locals
; 1000:0000 = fat
; 2000:0000 = root directory
; 3000:0000 = loader

%include "const.inc"

; ========================
; Boot Sector
; ========================

jmp short start
nop

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

%ifdef DEBUG
mov si,str_start
call prtstr
%endif

; Check LBA support
mov bx,55AAh  ; must be this
mov ah,41h    ; check lba function
int 13h
jc hang       ; error
cmp bx,0AA55h ; bx holds this if it's enabled
jne hang
test cl,1     ; cx bit 1 means disk access using packet structure is allowed
jz hang

; Copy FAT to 1000:0000
mov ax,1000h
mov es,ax
xor di,di
mov ax,[bpbReservedSectors]
cwd
mov cx,[bpbFATSectors16]
call ReadSectors

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

; Copy Root directory to 2000:0000
mov ax,2000h
mov es,ax
pop ax
xor di,di
call ReadSectors

; Search for "OSLDR"
xor bx,bx
search:
mov al,[es:bx]
test al,al
jz hang ; No more files to search
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
jmp search

; =======================
; hang
; =======================

hang:
mov si,errstr ; load error string
call prtstr
hang0:
mov ax,0
int 16h ; wait for key press
int 19h ; reboot computer

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
jc hang          ; Halt on error

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
; es:di = destination
; si    = cluster number
; returns next cluster in si
; ===================================

ReadCluster:
mov ax,si
sub ax,2  ; Subtract unused sectors
jb hang   ; Given cluster Must be >= 2!
movzx cx,[bpbSectorsPerCluster]
mul cx
add ax,[FirstDataSector] ; Add first data sector
adc dx,0                 ; dx:ax = first sector of cluster
call ReadSectors
; Find next sector
push es
mov ax,1000h
mov es,ax
shl si,1 ; entry*2 = offset
mov si,[es:si] ; next cluster is in si
pop es
ret

; ==============================
; Function: prtstr
; ds:si = null-terminated string
; ==============================

prtstr:
pusha
mov ah,0Eh
mov bx,7
prtchr:
lodsb
test al,al
jz prtstrend
int 10h
jmp prtchr
prtstrend:
popa
ret

; ==========================
; strings
; ==========================
%ifdef DEBUG
str_start: db "MyOS Bootloader",0Dh,0Ah,0
%endif
errstr: db 'Error starting MyOS.',0Dh,0Ah,'Press any key to reboot.',0Dh,0Ah,0
filename: db "OSLDR      "

; ==========================
; end
; ==========================

; boot sector magic number at org 510
times 510-($-$$) db 0
dw 0AA55h

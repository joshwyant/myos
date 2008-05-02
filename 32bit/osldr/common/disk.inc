; ==============================
; Function: ReadSectors
; es:edi = buffer
; dx:ax  = sector = 1st index = 0
; cx     = number of sectors
; ==============================
ReadSectors:
push si
push cx
xor bx,bx
mov si,Packet
add ax,word [HiddenSectors]
adc dx,word [HiddenSectors+2]
read0:
mov word [PacketOffset],disk_buffer
mov [PacketSegment],ds
mov [PacketLBA0],ax
mov [PacketLBA16],dx
mov dl,[DriveNumber]
mov ah,42h
int 13h
jc sectorError
push cx
push ebx
mov ebx,disk_buffer
mov cx,[BytesPerSector]
CopySectorByte:
mov al,[ebx]
mov [es:edi],al
inc edi
inc ebx
loop CopySectorByte
pop ebx
pop cx
mov ax,[PacketLBA0]
mov dx,[PacketLBA16]
add ax,1
adc dx,0
loop read0
pop cx
pop si
ret
sectorError:
mov si,sectorErrStr
call prtstr
call halt

; ===================================
; Function: ReadCluster
;
; es:edi = destination
; si     = cluster number
; returns next cluster in si
; ===================================
ReadCluster:
push bp
mov bp,sp
sub sp,2
mov ax,si
sub ax,2  ; Subtract unused sectors
jb invcluster   ; Given cluster Must be >= 2!
movzx cx,[SectorsPerCluster]
mul cx
add ax,[FirstDataSector] ; Add first data sector
adc dx,0         ; dx:ax = first sector of cluster
call ReadSectors
; Find next sector
push es
pushad
mov ax,1000h
mov es,ax
xor edi,edi
xor dx,dx
shl si,1 ; entry*2 = offset
mov ax,si
div word [BytesPerSector]
add ax,[ReservedSectors] ; dx=offset, ax=sector
push dx
cmp ax,[FATSector]
je .noreadfat
mov [FATSector],ax
xor dx,dx
mov cx,1
call ReadSectors
.noreadfat:
pop si ; was push dx
mov si,[es:si] ; next cluster is in si
mov [bp-2],si
popad
pop es
mov si,[bp-2]
leave
ret
invcluster:
mov si,badclsstr
call prtstr
call halt

; ================================
; Function: ReadFile
; ds:si  = filename
; es:edi = buffer
; ================================
ReadFile:
call lookup
test cx,cx
jz rfnotfound
mov si,cx
rf0:
call ReadCluster
cmp si,0fff8h
jb rf0
ret
rfnotfound:
mov si,nofilestr
call prtstr
call halt

; ================================
; function lookup
; ds:si = filename
; cx will be first cluster or zero
; ================================
lookup:
push edi
push es
mov ax,2000h
mov es,ax
mov cx,[RootSectors]
mov ax,[RootSector]
mov dx,[RootSector+2]
.read:
xor edi,edi
push cx
mov cx,1
call ReadSectors
pop cx
xor di,di
.search:
mov al,[es:di]
test al,al
jz .notfound
mov al,[es:di+11]
test al,18h
jnz .cont
push si
push di
push cx
mov cx,11
rep cmpsb
pop cx
pop di
pop si
je .found
.cont:
add di,32
cmp di,[BytesPerSector]
je .looper
jmp .search
.found:
mov cx,[es:di+26]
.notfound:
pop es
pop edi
ret
.looper:
add ax,1
adc dx,0
loop .read
jmp .notfound
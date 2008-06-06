; ==============================
; Function: ReadSectors
; es:edi = buffer
; edx:eax  = sector = 1st index = 0
; cx     = number of sectors
; ==============================
ReadSectors:
push si ; si is used in ReadCluster and int 14h ah=42h
push cx ; cx is used in ReadCluster? and for BytesPerSector
xor bx,bx
mov si,Packet
add eax,[HiddenSectors] ; add the hidden sectors to edx:eax
adc edx,0
read0: ; get the disk access packet ready
mov word [PacketOffset],disk_buffer
mov [PacketSegment],ds
mov [PacketLBA0],eax
mov [PacketLBA32],edx
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
mov eax,[PacketLBA0] ; Add one to sector
mov edx,[PacketLBA32]
add eax,1
adc edx,0
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
; esi     = cluster number
; returns next cluster in esi
; ===================================
ReadCluster:
push bp
mov bp,sp
; LOCALS:
; BP-4:  Next cluster
sub sp,4
mov eax,esi
sub eax,2  ; Subtract unused sectors
jb invcluster   ; Given cluster Must be >= 2!
movzx ecx,byte [SectorsPerCluster]
mul ecx
add eax,[FirstDataSector] ; Add first data sector
adc edx,0         ; edx:eax = first sector of cluster
call ReadSectors
; Find next sector
push es
pushad
mov ax,1000h
mov es,ax
xor edi,edi
mov al,1		; \
xor al,[IsFAT16]	; | IsFAT16 ? 1 : 2
inc al		; /
shl esi, al		; offset = cluster * (IsFAT16 ? 2 : 4);
mov eax,esi       ;
xor edx,edx				 ;
movzx ebx, [BytesPerSector]	 ;
div ebx				 ;
add eax,word [ReservedSectors] ; edx=offset, eax=sector
push edx
cmp eax,[FATSector]
je .noreadfat
mov [FATSector],eax
xor edx,edx
mov cx,1
call ReadSectors
.noreadfat:
pop edx
mov esi,[es:dx] ; next cluster is in esi
; Mask the cluster appropriately depending on filesystem
cmp byte [IsFAT16],0
je .notrunc ; don't truncate if it's FAT32
and esi,0xFFFF
.notrunc
and esi,0x0FFFFFFF
mov [bp-4],esi
popad
pop es
mov esi,[bp-4]
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
test ecx,ecx
jz rfnotfound
mov esi,ecx
rf0:
call ReadCluster
mov al,[IsFAT16]
and al,al
jz .32
cmp esi,0fff8h
jmp .t
.32:
and esi,0x0FFFFFFF
cmp esi,0x0FFFFFF8
.t
jb rf0
ret
rfnotfound:
mov si,nofilestr
call prtstr
call halt

; ================================
; function lookup
; ds:si = filename
; ecx will be first cluster or zero
; ================================
lookup16:
; ecx lookup16(ds:si filename) {
;   char *ptr = 2000:0;
;   for (int i = RootSectors; i > 0; i--) {
;     ReadSectors(ptr,RootSector+i,1);
;     for (int t = 0; t < BytesPerSector; t += 32) {
;       if (ptr[t] == 0) return 0;
;       if (ptr[t+11] & 0x18) continue;
;       if (!strcmp(filename,ptr+t,11)) return (uint)*(ushort*)(ptr+t+26);
;     }
;   }
;   return 0;
; }
push edi
push es
mov ax,2000h
mov es,ax
mov cx,[RootSectors]
mov eax,[RootSector]
mov edx,0
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
movzx cx,[es:di+26]
.notfound:
pop es
pop edi
ret
.looper:
add ax,1
adc dx,0
loop .read
jmp .notfound

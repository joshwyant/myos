; ==============================
; Function: ReadSectors
; es:edi = buffer
; edx:eax  = sector = 1st index = 0
; cx     = number of sectors
; -------------OK---------------
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
; ----------------OK-----------------
; ===================================
ReadCluster:
mov eax,esi
sub eax,2  ; Subtract unused sectors
jb invcluster   ; Given cluster Must be >= 2!
movzx ecx,byte [SectorsPerCluster]
mul ecx
add eax,[FirstDataSector] ; Add first data sector
adc edx,0         ; edx:eax = first sector of cluster
call ReadSectors
; Fall through to NextCluster

; ===================================
; Function: NextCluster
;
; esi     = cluster number
; returns next cluster in esi
; ----------------OK-----------------
; ===================================
NextCluster:
push bp
mov bp,sp
sub sp,4
push es
pushad
mov ax,1000h
mov es,ax
xor edi,edi
call [FATOffset]		 ;
mov eax,esi       		 ;
xor edx,edx			 ;
movzx ebx, word[BytesPerSector]	 ;
div ebx				 ;
movzx ebx,word[ReservedSectors]  ;
add eax,ebx                      ; edx=offset, eax=sector
push edx
cmp eax,[FATSector]
je .noreadfat
mov [FATSector],eax
xor edx,edx
mov cx,1
call ReadSectors
.noreadfat:
pop edx
mov esi,[es:edx] ; next cluster is in esi
call [MaskCluster]
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
; --------------OK----------------
; ================================
ReadFile:
push si
call lookup
test ecx,ecx
jz rfnotfound
pop si
mov esi,ecx
.NextCluster:
call ReadCluster
call [CompareCluster]
jb .NextCluster
ret
rfnotfound:
mov si,nofilestr
call prtstr
pop si
add si,11
call prtstr
call endl
call halt

; =================================
; function lookup
; ds:si = filename
; ecx will be first cluster or zero
; =================================
lookup:
jmp [Lookup]

lookup16:
    ; ecx lookup16(ds:si filename) {
    ;   es = 2000h;
    ;   for (int i = 0; i < RootSectors; i++) {
    ;     ReadSectors(es:0,RootSector+i,1);
    ;     for (char* ptr = 0; ptr < BytesPerSector; ptr += 32) {
    ;       if (es:ptr == 0) return 0;
    ;       if (es:ptr[11] & 0x18) continue;
    ;       if (strcmp(filename,es:ptr,11) == 0) return (uint)*(ushort*)(es:ptr+26);
    ;     }
    ;   }
    ;   return 0;
    ; }

	; init:
	push edi ; save es:edi
	push es
	mov ax,2000h ; es:edi = 2000:?
	mov es,ax
	mov cx,[RootSectors] ; "i": Number of times to loop (Preserve cx)
	mov eax,[RootSector] ; LBA: Start at the root directory sector
	mov edx,0            ;   (preserve edx:eax)
	; Outer loop:
	.read:
	        ; Read a sector
		  xor edi,edi ; es:edi = 2000:0000
		  push cx ; Save the outer loop index
		   mov cx,1 ; Number of sectors to read
		   call ReadSectors
		  pop cx ; Restore the outer loop index
	
		; Find a matching entry
		  xor di,di ; Current entry ('ptr')
		 .search:
			cmp byte [es:di],0
			je .fail
			test byte [es:di+11],0x18
			jnz .scont
			pusha
			mov cx,11
			rep cmpsb
			popa
			jne .scont
			movzx ecx, word[es:di+26]
			jmp .ret
		 .scont:
		  add di,32
		  cmp di,[BytesPerSector]
		  jc .search
		; end .search
	; Increment sector in root directory. Loop.
	add eax,1
	adc edx,0
	loop .read
	; end .read
	.fail: 		; Failure point (return 0)
	mov ecx,0
	.ret:		; return point
	pop es
	pop edi
	ret
; End lookup16

lookup32:
; ecx lookup32(ds:si filename) {
;   es = 0x2000;
;   edx clus = RootCluster;
;   while (clus < 0x0FFFFFF8) {
;     ReadCluster(es:0,clus);
;     for (char* ptr = 0; ptr < BytesPerCluster; ptr += 32)
;     {
;         if (es:ptr[0] == 0) return 0;
;         if (es:ptr[11] & 0x18) continue;
;         if (srtcmp(filename, es:ptr, 11) == 0) return *(ushort*)(es:ptr+20)<<16|*(ushort*)(es:ptr+26);
;     }
;     clus = NextCluster(clus);
;   }
;   return 0;
; }
	; init:
	push bp
	mov bp,sp
	sub sp,2
	mov [bp-2],si
	push edi ; save es:edi
	push es
	mov ax,2000h ; es:edi = 2000:?
	mov es,ax
	mov esi,[RootCluster]
	; Outer loop:
	.read:
		  cmp esi,0x0FFFFFF8
		  jge .fail
	        ; Read a cluster
		  xor edi,edi ; es:edi = 2000:0000
		  call ReadCluster
	
		; Find a matching entry
		  xor di,di ; Current entry ('ptr')
		 .search:
			cmp byte [es:di],0
			je .fail
			test byte [es:di+11],0x18
			jnz .scont
			pusha
			mov si,[bp-2]
			mov cx,11
			rep cmpsb
			popa
			jne .scont
			mov ecx, [es:di+18] ; high word at 20
			mov cx, [es:di+26]  ; low word at 26
			jmp .ret
		 .scont:
		  add di,32
		  cmp [BytesPerCluster], di
		  jc .search
		; end .search
	loop .read
	; end .read
	.fail: 		; Failure point (return 0)
	mov ecx,0
	.ret:		; return point
	pop es
	pop edi
	leave
	ret
; End lookup32


CompareCluster16:
cmp esi,0xFFF8
ret

CompareCluster32:
cmp esi,0x0FFFFFF8
ret

MaskCluster16:
and esi,0xFFFF
ret

MaskCluster32:
and esi,0x0FFFFFFF
ret

FATOffset16:
shl esi,1
ret

FATOffset32:
shl esi,2
ret

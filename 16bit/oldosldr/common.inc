; malloc NOT GOOD
; returns segment in bx
malloc:
mov bx,4000h
ret

; ==============================
; Function: ReadSectors
; es:di = buffer
; dx:ax = sector = 1st index = 0
; cx    = number of sectors
; ==============================
ReadSectors:
push si
push cx
lea si,[Packet]
add ax,word [HiddenSectors]
adc dx,word [HiddenSectors+2]
read0:
mov [PacketOffset],di
mov [PacketSegment],es
mov [PacketLBA0],ax
mov [PacketLBA16],dx
mov dl,[DriveNumber]
mov ah,42h
int 13h
jc sectorError
add di,[BytesPerSector]
jnc read1
mov ax,es
add ax,1000h
mov es,ax
read1:
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
; es:di = destination
; si    = cluster number
; returns next cluster in si
; ===================================
ReadCluster:
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
mov ax,1000h
mov es,ax
shl si,1 ; entry*2 = offset
mov si,[es:si] ; next cluster is in si
pop es
ret
invcluster:
mov si,badclsstr
call prtstr
call halt

; ================================
; Function: ReadFile
; ds:si = filename
; es:di = buffer
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
push di
xor di,di
xor cx,cx
push es
mov ax,2000h
mov es,ax
search:
mov al,[es:di]
test al,al
jz notfound
mov al,[es:di+11]
test al,18h
jnz cont
push si
push di
push cx
mov cx,11
rep cmpsb
pop cx
pop di
pop si
jz found
cont:
add di,32
jmp search
found:
mov cx,[es:di+26]
notfound:
pop es
pop di
ret

; prtchr al
prtchr:
pusha
push ds
push cs
pop ds
mov bx,word [GlobalText]
pop ds
mov ah,0Eh
int 10h
popa
ret


; ==============================
; Function: prtstr
; ds:si = null-terminated string
; ==============================
prtstr:
pusha
; Make sure we are in text mode
mov ah,0Fh
int 10h
cmp al,3
je prtstrprtchr ; if we are, no need to switch to it
; switch to text mode
mov ax,3
int 10h
prtstrprtchr:
lodsb      ; al = char
test al,al ; is al 0?
jz prtstrend
call prtchr
jmp prtstrprtchr
prtstrend:
popa
ret

endl:
push si
mov si,crlf
call prtstr
pop si
ret

; halt
halt:
;cli
hlt
jmp halt

toupper:
cmp al,61h
jl toupperret
cmp al,7Ah
jg toupperret
and al,0DFh
; is lowercase
toupperret:
ret

; ds:si = buffer; cx = buffer size
getstr:
; get ready
pusha
dec cx
mov bx,0
getchr:
; wait for key
mov ah,0
int 16h
; did we press enter?
cmp al,13
je getstrdone
; did we press backspace?
cmp al,8
je chrbackspace
; are we at the end of the buffer?
cmp bx,cx
je getchr
; Is this a printable character?
cmp al,32
jc getchr
cmp al,127
jnc getchr
call prtchr ; print it
mov [si+bx],al ; store in buffer
inc bx
jmp getchr
; we typed a backspace
chrbackspace:
cmp bx,0 ; is there anything to erase?
je getchr
dec bx ; go back in buffer
push ds
push cs
pop  ds
push si
mov si,backspace ; erase character
call prtstr
pop si
pop ds
jmp getchr
getstrdone: ; finish up
mov byte [si+bx],0
popa
ret

; prtfile ds:si
prtfile:
pusha
mov cx,8
prtfile1:
cmp cx,0
je prtfile1e
dec cx
lodsb
cmp al,20h
je prtfile1
call prtchr
jmp prtfile1
prtfile1e:
cmp byte [si],20h
je prtfileq
mov al,2eh
call prtchr
lodsb
call prtchr
mov cx,2
prtfile2:
lodsb
cmp al,20h
je prtfile2e
call prtchr
prtfile2e:
loop prtfile2
prtfileq:
popa
ret

; ds:si = filename terminated by 0h or 20h
; es:di = buffer, size=11
dotto83:
pusha
mov cx,8
dotto83name:
lodsb
; end of name?
cmp al,'.'
je dotto83dot
cmp al,0
je dotto83noext
cmp al,20h
je dotto83noext
; no, part of name
call toupper
stosb
loop dotto83name
; what is the character after the 8 chars?
lodsb
cmp al,0
je dotto83noext
cmp al,20h
je dotto83noext
cmp al,'.'
je dotto83ext
jmp dotto83err ; uh, the name shouldn't be longer than 8 without dot or space or 0
dotto83dot:
call dotto83fill
; copy the extension
dotto83ext:
mov cx,3
dotto83extloop:
lodsb
cmp al,0
je dotto83end
cmp al,20h
je dotto83end
call toupper
stosb
loop dotto83extloop
lodsb
cmp al,0
je dotto83ret
cmp al,20h
je dotto83ret
jmp dotto83err ; extension should be 3 chars
dotto83noext:
add cx,3
dotto83end:
call dotto83fill
dotto83ret:
popa
ret
dotto83fill:
mov al,20h
rep stosb
ret
dotto83err:
popa
pusha
mov cx,11
mov al,20h
rep stosb
jmp dotto83ret


; autoext
; ds:si=filename (8.3)
; es:di=extension (padded)
; sets the current filename (ds:filename) to the given 8.3 filename,
; defaulting to the given extension if extension is blank.
; i.e. doc.txt (ext: BAT) -> DOC     TXT
;      doc     (ext: BAT) -> DOC     BAT
;      doc.    (ext: BAT) -> DOC     BAT
autoext:
pusha
push ds ; store segments
push es
push bp
mov bp,sp
sub sp,2
; dont't use ES:DI yet, we use it now for string comparisons
push es ; push es now to get it in ds later (so ds:si is the extension)
mov ax,3000h
mov es,ax
mov [bp-2],di ; store di
mov di,filename ; es:di = 3000:filename
; get the filename
call dotto83 ; ds:si=fname,es:di=buffer
; is the extension blank? can we use ours?
mov cx,3
mov ds,ax
mov si,filename+8 ; ds:si = 3000:filename+8
mov di,blankext   ; es:di = blankext
rep cmpsb
pop ds ; so ds:si is what was es:di
jne nocopyext ; if it is not blank, don't bother with substitution
mov cx,3
mov si,[bp-2] ; ds:si = extension
mov di,filename+8 ; es:di = 3000:filename+8
rep movsb
nocopyext:
leave
pop es ; get back our original segments
pop ds
popa
ret
; initdisk
initdisk:
mov [DriveNumber],dl
; Copy BPB
cld
push ds
mov bx,07C0h
mov ds,bx
xor si,si
xor di,di
mov cx,45
rep movsw
pop ds
call testlba
; copy FAT to 1000:0000
push es
mov ax,1000h
mov es,ax
xor di,di
xor dx,dx
mov ax,[ReservedSectors]
mov cx,[FATSectors16]
call ReadSectors
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
xor di,di
call ReadSectors
pop es
ret

; testlba
testlba:
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
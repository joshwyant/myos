; OSLDR
; ----------------------------------------
; MyOS v0.0.1 x86 protected-mode OS-loader
; (C) 2008 Josh Wyant
; 
; Dependencies:
; - OSLDR (this file) was loaded to 3000:0000 (Real address mode)
; - The file system is FAT16
; - The drive number is stored in DL
;
; Compiles under NASM
; > nasm osldr.asm
; same as > nasm osldr.asm -fbin -o osldr
; Save as OSLDR to root directory of drive with MyOS boot sector.
; 

; bits 16

; At 3000:0000
jmp start

times 11-($-$$) db 0
; BPB
BytesPerSector		dw 0
SectorsPerCluster    	db 0
ReservedSectors      	dw 0
NumFATs              	db 0
RootEntries          	dw 0
TotalSectors16      	dw 0
MediaDescriptor     	db 0
FATSectors16        	dw 0
SectorsPerTrack     	dw 0
NumHeads               	dw 0
HiddenSectors       	dd 0
TotalSectors32       	dd 0

times 90-($-$$) db 0
Packet:
PacketSize 		db 16
PacketReserved1 	db 0
PacketSectors		db 1
PacketReserved2 	db 0
PacketOffset		dw 0
PacketSegment		dw 0
PacketLBA0		dw 0
PacketLBA16		dw 0
PacketLBA32		dw 0
PacketLBA48		dw 0

FirstDataSector 	dw 0
DriveNumber		db 0

start:
xor ax,ax ; ax = 0
; setup frame
cli ; disable interrupts while setting up stack frame (clear Interrupt Flag)
mov ss,ax
mov sp,7c00h
sti ; allow interrupts
; segments
mov bx,3000h
mov ds,bx    ; DS = 3000h
mov es,bx    ; ES = 3000h

; print title message
; mov si,titlestr
; call prtstr

; init
call initdisk

; Enable INT 21h services
push es
xor ax,ax
mov es,ax
mov word [84h],Int21
mov word [86h],3000h 
pop es

mov si, titlestr
mov ax,400h
int 21h

;prmpt:
;call prompt
;jmp prmpt
;
;call halt

;prompt:
;mov si,PromptStr
;call prtstr

;ret
;CommandBuffer: times 101 db 0
;PromptStr: db 0Dh,0Ah,'% ',0

; set video mode
mov ax,13h
int 10h
mov si,splashfile
call lookup
cmp cx,0
je nosplash
push es
mov ax,4000h
mov es,ax
xor di,di
push es
call ReadFile
pop es
mov dx,3C8h
mov al,0
out dx,al ; set all pallette entries
xor di,di
mov cx,768
mov dx,3C9h
pallette:
mov al,[es:di]
inc di
out dx,al
loop pallette
push ds
mov ax,es
mov ds,ax
mov ax,0A000h
mov es,ax
mov si,768
xor di,di
mov cx,32000
rep movsw
pop ds
pop es
nosplash:

;; load GDTR
;lgdt [gdt48]
;
;; switch to protected mode
;mov eax,cr0
;or  eax,1
;mov cr0,eax
;
;;in 16 bit segment: 32 bit jmp far 8:(code32+30000h)
;cli ; clear interrupts?
;jmp dword 8:code32+30000h

call halt


; ***** Interrupt 21h **** ;
Int21:
; begin
;
; locals:
; [bp-2]: offset to jump
;
push bp
mov bp,sp
sub sp,2
; get address from jump table
push bx
push ds
mov bx,3000h
mov ds,bx
; bx = Int21AHTable+1+ah*2
movzx bx,ah
shl bx,1
mov bx,word [Int21AHTable+bx+1]
mov [bp-2],bx
cmp al,byte [Int21AHTable]
pop ds
pop bx
jnc Int21BadAH
call [bp-2] ; function must 'ret'
Int21Ret:
leave
iret
Int21BadAH:
; set new flags
push ax
mov ax,[bp+6] ; get flags (bp+ip+cs=f)
push ax ; push value
popf    ; and pop as flags
stc     ; set the carry flag
pushf   ; push on stack
pop ax  ; pop into ax
mov [bp+6],ax ; store as flags
pop ax
; end
leave
iret

Int21AHTable:
db 2 ; number of ints
dw halt   ; ah = 0
dw halt   ; ah = 1
dw halt   ; ah = 2
dw halt   ; ah = 3
dw prtstr ; ah = 4






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

; ==============================
; Function: ReadSectors
; es:di = buffer
; dx:ax = sector - 1st index = 1
; cx    = number of sectors
; ==============================
ReadSectors:
push si
push cx
lea si,[Packet]
read0:
add ax,word [HiddenSectors]
adc dx,word [HiddenSectors+2]
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
GlobalText:
GlobalTextColor: db 7
GlobalTextPage:  db 0

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

; halt
halt:
;cli
hlt
jmp halt

titlestr: db "Greetings from MyOS.",0Dh,0Ah,0
LBAFailStr: db "Error: No LBA support",0Dh,0Ah,0
sectorErrStr: db "Error reading sector",0Dh,0Ah,0
badclsstr: db "Error: Invalid cluster number",0Dh,0Ah,0
nofilestr: db "Error: File not found",0Dh,0Ah,0
splashfile: db "SPLASH  IMG"

align 16

gdt:
dq 0000000000000000h ; first entry unused
dq 00CF9A000000FFFFh ; Kernel Code
dq 00CF92000000FFFFh ; Kernel Data
dq 00009A030000FFFFh ; 64KB Code
dq 000092030000FFFFh ; 64KB Data
gdtEnd:

gdt48:
dw gdtEnd-gdt-1        ; table limit
dd gdt+30000h          ; offset
dw 0                   ; padding

bits 32
code32:
hlt
jmp code32

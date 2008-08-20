restart:
call kbd_wait
mov al,0d1h
out 64h,al
call kbd_wait
mov al,0feh
out 60h,al
ret

kbd_wait:
in al,64h
test al,2
jnz kbd_wait
ret

kbd_wait1:
in al,64h
test al,1
jz kbd_wait1
ret

halt:
call cls
push si
mov si,fatalerror
call prtstr
pop si
call prtln
mov si,haltstr1
call prtln
mov si,haltstr2
call prtln
mov ah,0
int 16h
call restart
hlt
jmp $-1


cls:
pusha
mov ah,2 ; move cursor
mov bh,0 ; page
mov dx,0 ; dh:dl = row:col
int 10h  ; video
mov cx, 2000  ; 2000 words
mov ax, 0720h ; space, white on black
push es
mov bx,0b800h
mov es,bx ; video text buffer
xor di,di
rep stosw ; clear screen
pop es
popa
ret

prtln:
call prtstr
mov si,crlf
; fall through to prtstr

prtstr:
pusha
.0:
lodsb
test al,al
jz .1
mov bx,7
mov ah,0Eh
int 10h
jmp .0
.1:
popa
ret
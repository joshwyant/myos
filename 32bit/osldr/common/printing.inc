; print al
prtchr:
push ax
push bx
mov ah,0xe
mov bx,7
int 10h
pop bx
pop ax
ret

prthexb:
push ax
push dx
push bx
mov dl,al
mov dh,al
and dl,15
shr dh,4
cmp dl,10
jb .0
add dl,7
.0:
cmp dh,10
jb .1
add dh,7
.1:
add dl,30h
add dh,30h
mov ah,0xe
mov bx,7
mov al,dh
int 10h
mov al,dl
int 10h 
pop bx
pop dx
pop ax
ret

prthexw:
ror ax,8
call prthexb
ror ax,8
call prthexb
ret

prthexd:
ror eax,16
call prthexw
ror eax,16
call prthexw
ret

prtdecd:
pusha
xor cx,cx
mov ebx,10
.0:
xor edx,edx
div ebx
push dx
inc cx
and eax,eax
jnz .0
.1:
pop ax
add ax,30h
call prtchr
loop .1
popa
ret

prtdecw:
push eax
movzx eax,ax
call prtdecd
pop eax
ret

prtdecb:
push eax
movzx eax,al
call prtdecd
pop eax
ret

prtsdecd:
push eax
test eax,0x80000000
jz .0
not eax
inc eax
push ax
mov al,'-'
call prtchr
pop ax
.0:
call prtdecd
pop eax
ret

prtsdecw:
push eax
movsx eax,ax
call prtsdecd
pop eax
ret

prtsdecb:
push eax
movsx eax,al
call prtsdecd
pop eax
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
mov ah,0Eh
mov bx,7
int 10h
jmp prtstrprtchr
prtstrend:
popa
ret

prtln:
call prtstr
call endl
ret

endl:
push si
mov si,crlf
call prtstr
pop si
ret
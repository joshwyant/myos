; Initializes the video driver.
_initvideo:
; get cga controller base i/o port address 
mov dx,[fs:463h] ; dx = port
mov [crtbaseio],dx
; get cursor position:
; get low port from index register
mov al,0Fh ; low index
out dx,al
inc dx
in al,dx
mov cl,al
; get high port from index register
dec dx
mov al,0eh
out dx,al
inc dx
in al,dx
mov ch,al
mov [cursorpos], cx
; check if the cursor is hidden
dec dx
mov al,0Ah
out dx,al
inc dx
in al,dx
test al,20h
jz _initvideo_cursorhidden
mov byte [cursorshown],1
_initvideo_cursorhidden:
ret

; bl = color
; al = char
; doesn't call updatecursorindex
_prtchr:
push eax
push edx
push ebx
;mov bl,[cursorshown]
;push ebx
;push eax
;mov al,0
;call showcursor
;pop eax
mov ah,bl
cmp al,8
je _pchbacksp
cmp al,10
je _pchlf
cmp al,13
je _pchcr
cmp al,20h
jc _prtchrend
cmp al,7Fh
jnc _prtchrend
; normal characters
movzx ebx,word [cursorpos]
mov [fs:0b8000h+ebx*2],ax
inc word [cursorpos]
jmp testcursor
_pchlf:
add word [cursorpos],80
jmp testcursor
_pchcr:
mov dx,0
mov bx,80
mov ax,[cursorpos]
div bx
mov dx,80
mul dx
mov [cursorpos],ax
jmp _prtchrend
_pchbacksp:
cmp word [cursorpos],0
je _prtchrend
dec word [cursorpos]
_prtchrend:
;pop eax
;call showcursor
pop ebx
pop edx
pop eax
ret
testcursor:
mov ax,[cursorpos]
cmp ax,2000
jc _prtchrend
sub word [cursorpos], 80
call newline
jmp _prtchrend
newline:
push ecx
push ebx
push eax
; move text one line up
mov ecx,1920
xor ebx,ebx
newline1:
mov ax,[fs:0b80A0h+ebx*2]
mov [fs:0b8000h+ebx*2],ax
inc bx
loop newline1
; clear new line
mov ebx,ecx
add ecx,80
newline2:
mov word [fs:0b8F00h+ebx*2],0720h
inc ebx
loop newline2
pop eax
pop ebx
pop ecx
ret

; ds:esi = null-terminated string
; bl = color
prtstr:
push eax
prtstrloop:
lodsb
cmp al,0
je prtstrend
call _prtchr
jmp prtstrloop
prtstrend:
call updatecursorindex
pop eax
ret

; al = char
; bl = color
; updates cursor on screen
prtchr:
call _prtchr
call updatecursorindex
ret

; al: 1=show cursor, 0=hide
showcursor:
cmp al,[cursorshown]
je dontshowhidecursor
push edx
times 2 push eax
mov dx,[crtbaseio]
mov al,0Ah
out dx,al
inc dx
pop eax
and al,al
jz _hidecursor
; show the cursor
in al,dx
or al,20h
out dx,al
jmp showcursorpop
_hidecursor:
; hide the cursor
in al,dx
and al,0DFh
out dx,al
showcursorpop:
pop eax
pop edx
dontshowhidecursor:
ret

; sets ax to cursorpos
getcursorindex:
mov ax, [cursorpos]
ret

; ax = cursorpos
setcursorindex:
mov [cursorpos], ax
ret

updatecursorindex:
push edx
push eax
mov dx, [crtbaseio]
mov al, 0Fh
out dx,al
inc dx
mov al,[cursorpos]
out dx,al
dec dx
mov al, 0Eh
out dx,al
inc dx
mov al,[cursorpos+1]
out dx,al
pop eax
pop edx
ret
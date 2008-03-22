; print message
mov ah,04h
mov si,msg
int 21h

goagain:

; print message
mov ah,4
mov si,type
int 21h

; get string
mov si, buffer
mov cx,100
mov ah,1
int 21h

mov ah,4
mov si,echo1
int 21h
mov si,buffer
int 21h
mov si,echo2
int 21h

mov ah,0
int 16h

push ax
mov bx,0
mov ah,0eh
int 10h
pop ax

cmp al,0dh
je goagain
cmp al,'y'
je goagain

mov ah,4
mov si,crlf
int 21h

; wait for key press
mov ah,00h
mov si,cmdpause
int 21h

;return
retf

; strings
msg: db "Hello, world!",0
type: db 0dh,0ah,"Please type something: ",0
echo1: db 0dh,0ah,"You typed '",0
echo2: db "'",0dh,0ah,"Go again? [y/n]: y",8,0
cmdpause: db "PAUSE",0
crlf: db 0dh,0ah,0
buffer: times 101 db 0
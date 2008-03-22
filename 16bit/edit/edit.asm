; print message
mov ah,04h
mov si,msg
int 21h
;return
retf
msg: db "EDIT: program is unfinished.",0dh,0ah,"Usage: edit <filename>",0dh,0ah,0
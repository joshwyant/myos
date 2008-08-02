%include "common/printing.inc"
%include "common/disk.inc"

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
mov si,haltstr
call prtstr
mov ah,0
int 16h
call restart
hlt
jmp $-1

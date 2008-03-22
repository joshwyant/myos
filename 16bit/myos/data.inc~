align 16

gdt:
dq 0000000000000000h ; first entry unused
dq 00CF9A000000FFFFh ; Kernel Code
dq 00CF92000000FFFFh ; Kernel Data
dq 00009A040000FFFFh ; 64KB Code
dq 000092040000FFFFh ; 64KB Data
gdtEnd:

gdt48:
dw gdtEnd-gdt-1        ; table limit
dd gdt+40000h          ; offset
dw 0                   ; padding

idt48:
dw 0                   ; table limit
dd 0                   ; offset
dw 0                   ; padding

cursorpos: dw 0
crtbaseio: dw 0
cursorshown: db 0
cmda20on: db "A20ON",0

message:
db "You've entered protected mode.",0ah,0dh
db "The message you're seeing is being displayed by my video driver.",0ah,0dh
db "You can't type anything else right now. MyOS isn't finished yet.",0ah,0dh
db "Please restart your computer and try out the rest of MyOS.",0ah,0dh,
db "Thanks,",0dh,0ah,
db "  Josh Wyant",0ah,0dh,0
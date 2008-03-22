align 16

gdt:
dd 00000000h,00000000h ; first entry unused
dd 0000FFFFh,00CF9A00h ; Kernel Code
dd 0000FFFFh,00CF9200h ; Kernel Data
dd 0000FFFFh,00009A04h ; 64KB Code
dd 0000FFFFh,00009204h ; 64KB Data
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

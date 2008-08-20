global DriveNumber

DriveNumber: db 0
fatalerror: db "Kernel Loader",0Dh,0Ah,"Fatal error: ",0
err_smap: db "Failed to determine the amount of RAM in the system.",0
a20err: db "Failed to enable the A20 line.",0
haltstr1: db "The system cannot boot.",0
haltstr2: db "Please power off your computer, or you may press any key to try booting again.",0
crlf: db 0Dh,0Ah,0

align 8

gdt:
dd 00000000h,00000000h ; first entry unused
dd 0000FFFFh,00CF9A00h ; Loader Code
dd 0000FFFFh,00CF9200h ; Loader Data
gdtEnd:

gdt48:
dw gdtEnd-gdt-1        ; table limit
dd gdt                 ; offset
dw 0                   ; padding

idt48:
dd 0,0

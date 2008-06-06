BPB:
%define BytesPerSector 		11
%define SectorsPerCluster    	13
%define ReservedSectors      	14
%define NumFATs              	16
%define RootEntries          	17
%define TotalSectors16      	19
%define MediaDescriptor     	21
%define FATSectors16        	22
%define SectorsPerTrack     	24
%define NumHeads               	26
%define HiddenSectors       	28
%define TotalSectors32       	32

times 90-($-BPB) db 0
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

FirstDataSector 	dd 0
DriveNumber		db 0
RootSector		dd 0
RootSectors		dw 0
FATSector		dd 0
IsFAT16			db 0

LBAFailStr: db "Error: No LBA support",0Dh,0Ah,0
sectorErrStr: db "Error: Failed reading sector",0Dh,0Ah,0
err_smap: db "Error: Failed to determine amount of RAM",0Dh,0Ah,0
badclsstr: db "Error: Invalid cluster number",0Dh,0Ah,0
nofilestr: db "Error: File not found",0Dh,0Ah,0
a20err: db "Error: A20 enable failed",0Dh,0Ah,0
haltstr: db "Press any key to reboot.",0Dh,0Ah,0
kernelimg: db "KERNEL  IMG"
crlf: db 0dh,0ah,0

%ifdef DEBUG
titlestr: db "Kernel Loader",0Dh,0Ah,0
a20_gate: db "A20 enabled",0dh,0ah,0
a20_bios: db "A20 enabled via BIOS",0Dh,0Ah,0
a20_fast: db "A20 enabled via System Control Port A (Fast A20)",0Dh,0Ah,0
a20_keyb: db "A20 enabled via keyboard controller",0Dh,0Ah,0
msg_a20:  db "Checking A20 gate...",0Dh,0Ah,0
msg_unr:  db "Entering unreal mode...",0Dh,0Ah,0
msg_dsk:  db "Initializing disk...",0Dh,0Ah,0
msg_lba:  db "Testing LBA...",0Dh,0Ah,0
msg_mem:  db "Counting RAM...",0Dh,0Ah,0
msg_knl:  db "Loading kernel...",0Dh,0Ah,0
msg_jmp:  db "Starting MyOS...",0Dh,0Ah,0
msg_dot:  db '.',0
%endif

align 16

gdtu:
dd 0,0
db 0xff, 0xff, 0, 0, 0, 10010011b, 10001111b, 0
gdtuend:

gdtunreal:
dw gdtuend-gdtu-1
dd 0x30000+gdtu
dw 0 ; alignment

gdt:
dd 00000000h,00000000h ; first entry unused
dd 0000FFFFh,00CF9A00h ; Kernel Code
dd 0000FFFFh,00CF9200h ; Kernel Data
dd 0000FFFFh,00009A04h ; 64KB Code
dd 0000FFFFh,00009204h ; 64KB Data
gdtEnd:

gdt48:
dw gdtEnd-gdt-1        ; table limit
dd 0x30000+gdt         ; offset
dw 0                   ; padding

idt48:
dd 0,0

disk_buffer:

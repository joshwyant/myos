jmp start

times 11-($-$$) db 0 ; jmp+oemname = 11
; BPB
BytesPerSector		dw 0
SectorsPerCluster    	db 0
ReservedSectors      	dw 0
NumFATs              	db 0
RootEntries          	dw 0
TotalSectors16      	dw 0
MediaDescriptor     	db 0
FATSectors16        	dw 0
SectorsPerTrack     	dw 0
NumHeads               	dw 0
HiddenSectors       	dd 0
TotalSectors32       	dd 0

times 90-($-$$) db 0
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

FirstDataSector 	dw 0
DriveNumber		db 0

titlestr: db "Greetings from MyOS.",0Dh,0Ah,"Type 'help' for information on commands and use.",0Dh,0Ah,0
LBAFailStr: db "Error: No LBA support",0Dh,0Ah,0
sectorErrStr: db "Error reading sector",0Dh,0Ah,0
badclsstr: db "Error: Invalid cluster number",0Dh,0Ah,0
nofilestr: db "Error: File not found",0Dh,0Ah,0
splashfile: db "SPLASH  IMG"
blankext: times 3 db 20h
rmfext: db "RMF"
filename: times 11 db 0
backspace: db 8h,20h,8h,0
crlf: db 0dh,0ah,0
GlobalText:
GlobalTextColor: db 7
GlobalTextPage:  db 0
out_ver: db "MyOS v0.0.1",0dh,0ah,"Written by Joshua Wyant",0dh,0ah,"(C) 2008",0dh,0ah,0
out_a20on1: db "A20 enabled via BIOS.",0dh,0ah,0
out_a20on2: db "A20 enabled via System Contol Port A (Fast A20).",0dh,0ah,0
out_a20on3: db "A20 enabled via keyboard controller.",0dh,0ah,0
out_a20on4: db "A20 enable was unsuccessful.",0dh,0ah,0
out_a20off1: db "A20 disabled via BIOS.",0dh,0ah,0
out_a20off2: db "A20 disabled via System Contol Port A (Fast A20).",0dh,0ah,0
out_a20off3: db "A20 disabled via keyboard controller.",0dh,0ah,0
out_a20off4: db "A20 disable was unsuccessful.",0dh,0ah,0
out_a20on: db "A20 is enabled.",0dh,0ah,0
out_a20off: db "A20 is disabled.",0dh,0ah,0
out_pause: db "Press a key...",0
out_help:
db "MyOS v0.0.1",0dh,0ah
db "An executable file ends in .RMF; To run it, just type its name",0dh,0ah
db "  followed by any options separated by spaces. I.E., the text editor",0dh,0ah
db "  EDIT.RMF accepts the name of a file as an option. You can run it",0dh,0ah
db "  by typing 'edit myfile.txt.'",0dh,0ah
db "There are also some various built-in commands:",0dh,0ah
db "ver     : Displays version information",0dh,0ah
db "cls     : Clears the screen",0dh,0ah
db "dir     : Lists the files in the root directory",0dh,0ah
db "help    : Shows this help screen",0dh,0ah
db "pause   : Waits for the user to press a key",0dh,0ah
db "restart : Restarts your computer",0dh,0ah
db 0
out_unk: db ": This file is not a program, or it doesn't exist.",0dh,0ah,0
CommandBuffer: times 101 db 0
PromptStr: db "$ ",0

cmdtable: 
dw command_ver
db "ver",0,0,0,0,0
dw command_dir
db "dir",0,0,0,0,0
dw command_cls
db "cls",0,0,0,0,0
dw command_restart
db "restart",0
dw command_help
db "help",0,0,0,0
dw command_pause
db "pause",0,0,0
dw command_a20stat
db "a20stat",0
dw command_a20on
db "a20on",0,0,0
dw command_a20off
db "a20off",0,0
db 0

Int21AHTable:
db 5 ; number of ints
dw shellcmd   ; ah = 0
dw getstr     ; ah = 1
dw Int21BadAH ; ah = 2
dw Int21BadAH ; ah = 3
dw prtstr     ; ah = 4
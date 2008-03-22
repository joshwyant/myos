prompt:
mov si,PromptStr
call prtstr
mov si,CommandBuffer ; Buffer
mov cx,100           ; Buffer size
call getstr
push si
mov si,crlf
call prtstr
pop si
call shellcmd
ret

; cs should be 3000h, whether there was an int,
; or called from our segment
; ds:si = command string
shellcmd:
; we never need to return any registers here,
; better preserve them. This may be called with an int,
; so make sure DS and ES point to our code segment,
; then restore them!
pusha
push es ; use es
cmp byte [ds:si],0
je endcmd
cmp byte [ds:si],20h
je endcmd
push cs
pop es  ; make es cs
mov di,cmdtable
findcommand:
; search through the table and see if that is our command. we would call that
; we would call that location, set ds, and return to endcmd.
; if the command is not found, jump to nocommand.
mov bx,[es:di]
add di,2
and bx,bx
jz nocommand ; no more commands
call cmpcmd ; if command es:di compares to string ds:si, jumps to command in table, after pushing endcmd
add di,8
jmp findcommand ; not yet, so find another command 
nocommand:
; the command is not found. see if there is an rmf file.
; if there is, run it. It will retf to endcmd. If there isn't,
; print that the file couldn't be found, and jump to endcmd.
call findrmf
endcmd:
pop es
popa
ret

; this function calls our command.
; it makes sure that ds and es are ours,
; to be freindly to the command functions,
; and calls the command in bx. it makes
; sure ds and es are back to their
; original values. Then it jumps to endcmd.
; Use it with jmp callcmd. It's being
; jumped to by comparecommand, called by
; shellcmd, so it erases ip from the
; stack left by comparecmd, since we
; are returning to endcmd.
callcmd:
add sp,2
; es already ours
push ds
push cs
pop ds
call bx
pop ds
jmp endcmd

cmpcmd:
pusha
cmpcmd1:
mov al, [es:di]
call toupper
mov ah,al
mov al, [si]
call toupper
; ah = cmdbyte, al=strbyte
inc si
inc di
; we have two choices. Either we are at
; the end of the command, or it matches.
; otherwise, we know it doesn't match.
cmp al, 0
je cmdeof
cmp al,20h
je cmdeof
cmp al,ah
je cmpcmd1
jmp cmdnojump ; does not match
cmdeof:
; we get here at the end of the command
; typed in the string.
; here, we test if cmd byte is 0,
; which means that the commands match.
cmp ah,0
jne cmdnojump
popa
jmp callcmd
cmdnojump:
popa
ret

command_a20on:
; 1: ax=2401 int 15
; 2: set bit 2 port 92h
; 3: use keyb controller

; is it already enabled?
  mov si,out_a20on
  call test_a20
  jnc a20onprint

; Try method 1
  mov si,out_a20on1
mov ax,2401h
int 15h
  call test_a20
  jnc a20onprint

; Try method 2
  mov si,out_a20on2
in al,92h
test al,2
jnz no92
or al,2
and al,0FEh
out 92h,al
  call test_a20
  jnc a20onprint
no92:

; Try method 3
  mov si,out_a20on3
cli
call kbd_wait
mov al,0ADh
out 64h,al
call kbd_wait
mov al,0D0h
out 64h,al
call kbd_wait1
in al,60h
push ax
call kbd_wait
mov al,0D1h
out 64h,al
call kbd_wait
pop ax
or al,2
out 60h,al
call kbd_wait
mov al,0AEh
out 64h,al
;call kbd_wait
sti
  call test_a20
  jnc a20onprint

; No method worked.
mov si,out_a20on4

a20onprint:
call prtstr
ret

command_a20off:
; 1: ax=2400 int 15
; 2: clear bit 2 port 92h
; 3: use keyb controller

; is it already disabled?
  mov si,out_a20off
  call test_a20
  jc a20offprint

; Try method 1
  mov si,out_a20off1
mov ax,2400h
int 15h
  call test_a20
  jc a20offprint

; Try method 2
  mov si,out_a20off2
in al,92h
and al,0FCh
out 92h,al
  call test_a20
  jc a20offprint

; Try method 3
  mov si,out_a20off3
cli
call kbd_wait
mov al,0ADh
out 64h,al
call kbd_wait
mov al,0D0h
out 64h,al
call kbd_wait1
in al,60h
push ax
call kbd_wait
mov al,0D1h
out 64h,al
call kbd_wait
pop ax
and al,0FDh
out 60h,al
call kbd_wait
mov al,0AEh
out 64h,al
;call kbd_wait
sti
  call test_a20
  jc a20offprint

; No method worked.
mov si,out_a20off4

a20offprint:
call prtstr
ret

command_a20stat:
mov si,out_a20on
call test_a20
jnc a20print
mov si,out_a20off
a20print:
call prtstr
ret

; cf: 1=a20 disabled, 0=a20 enabled
test_a20:
mov ax,0
mov fs,ax
mov ax,0FFFFh
mov gs,ax
mov cx,2
testing_a20:
mov ax,[fs:7DFEh]
cmp ax,[gs:7E0Eh]
jne a20_enabled
rol word [fs:7DFEh],8
loop testing_a20
stc
ret
a20_enabled:
mov word [fs:7DFEh],0AA55h
clc
ret


command_dir:
push es
xor di,di
mov ax,2000h
mov es,ax
searchdir:
mov al,[es:di]
test al,al
jz dirend
cmp al,0E5h
je dircont
mov al,[es:di+11]
test al,18h
jnz dircont
push ds
mov ax,es
mov ds,ax
mov si,di
call prtfile
pop ds
mov si,crlf
call prtstr
dircont:
add di,32
jmp searchdir
dirend:
pop es
ret

; unknown command
command_unk:
mov si,CommandBuffer
mov di,filename
call dotto83
mov si,filename
call prtfile
mov si,out_unk
call prtstr
ret

; 'restart' command
command_restart:
;mov ax,0
;mov ds,ax
;mov word [417h],0h
;int 19h
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


; 'help' command
command_help:
mov si,out_help
call prtstr
ret

; 'ver' command
command_ver:
mov si,out_ver
call prtstr
ret

; 'pause' command
command_pause:
mov si,out_pause
call prtstr
mov ah,0
int 16h
mov si,crlf
call prtstr
ret

; 'cls' command
command_cls:
mov ah,2 ; move cursor
mov bh,0 ; page
mov dx,0 ; dh:dl = row:col
int 10h  ; video
mov cx, 2000  ; 2000 words
mov ax, 0720h ; space, white on black
push es
mov bx,0b800h
mov es,bx ; video text buffer
xor di,di
rep stosw ; clear screen
pop es
ret

findrmf:
; try to find a file
push es
mov ax,3000h
mov es,ax
mov di,rmfext
call autoext ; load filename (def. ext: .RMF)
; does this file exist?
push ds
mov ax,es
mov ds,ax
mov si,filename
call lookup
cmp cx,0
je rmfnotfound
mov cx,3
add si,8
mov di,rmfext
rep cmpsb
jne rmfnotfound
; RMF file found!
mov si,filename
push es
mov ax,4000h
mov es,ax
xor di,di
call ReadFile
pop es
; es,ds already pushed
mov ax,4000h
mov ds,ax
mov es,ax
call 4000h:0h
pop ds
pop es
ret
rmfnotfound:
pop ds
pop es
jmp command_unk ; call command_unk+ret
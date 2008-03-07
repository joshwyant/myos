; ***** Interrupt 21h **** ;
Int21:
; begin
;
; locals:
; [bp-2]: offset to jump
;
; 1. 'Enter' function
; 2. Lookup AH in address table, store address
; 3. Reached table limit? goto step 6
; 4. call function
; 5. Leave, return, flags unmodified
; 6. Not a valid function number
; 7. 

; step 1
push bp
mov bp,sp
sub sp,2
; step 2
; get address from jump table
push bx
push ds
mov bx,3000h
mov ds,bx
; bx = Int21AHTable+1+ah*2
movzx bx,ah
shl bx,1
mov bx,word [Int21AHTable+bx+1]
mov [bp-2],bx
; step 3, and end of step 2
; Yes! After 2 days I figured out this bug!
; The following line tested al instead of ah.
; The effect was that an interrupt seemingly randomly
; would just return.
cmp ah,byte [Int21AHTable]
pop ds
pop bx
jnc _Int21BadAH
call [bp-2] ; function must 'ret'
Int21Ret:
leave
iret
_Int21BadAH:
call Int21BadAH
jmp Int21Ret

Int21BadAH:
; set new flags
push ax
mov ax,[bp+6] ; get flags (bp+ip+cs=f) (int 21h's bp)
push ax ; push value
popf    ; and pop as flags
stc     ; set the carry flag
pushf   ; push on stack
pop ax  ; pop into ax
mov [bp+6],ax ; store as flags
pop ax
ret
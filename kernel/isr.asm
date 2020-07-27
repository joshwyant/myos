global irq0
global irq1
global irq8
global irq12
global int0
global int7
global int8
global intd
global inte
global int30
extern current_process
extern handle_timer
extern handle_keyboard
extern handle_clock
extern handle_mouse
extern fpu_task_switch
extern divide_error
extern double_fault
extern gpfault
extern pgfault
extern syscall
extern vm8086_gpfault


int0:
pusha
call divide_error
popa
iret

int8:
pusha
call double_fault
popa
add esp,4 ; remove error code
iret

int7:
pusha
call fpu_task_switch
popa
iret

intd:
test dword [esp+8], 0x20000
jnz .1
pusha
call gpfault
popa
add esp,4 ; remove error code
iret
.1:
jmp dword [vm8086_gpfault]

inte:
pusha
call pgfault
popa
add esp,4 ; remove error code
iret

; syscall interface
int30:
pusha
call syscall
popa
iret

irq0:
pusha
push ds
push es
push fs
push gs
mov eax, [current_process]
test eax,eax
jz .1 ; Process running?
    mov [eax],esp ; Save stack pointer
.1:
call handle_timer
test eax,eax
jz .2 ; switch tasks? (returned from handle_timer)
    mov eax,[current_process]   ; eax = process->esp
    mov esp,[eax]               ; set stack pointer
    mov ebx,[eax+16]            ; ebx = process->gpfault
    mov [vm8086_gpfault],ebx
    cmp dword[eax+12],0         ; process->vm8086
    jz .3 ; is vm8086 process?
        cmp word[.5],0x50
        jz .6 ; TSS needs updating?
            mov word[.5],0x50 ; VM8086 TSS
            jmp .4 ; (update TSS)
    .3:
    cmp word[.5],0x48
    jz .6 ; TSS needs updating?
        mov word[.5],0x48 ; System TSS
    .4:
    ltr word[.5]
    .6:
    mov ebx,[eax+8]             ; process->cr3
    mov eax,cr3
    cmp eax,ebx
    je .2 ; cr3 needs updating?
        mov eax,ebx
        mov cr3,eax
.2:
pop gs
pop fs
pop es
pop ds
mov al,20h ; end of interrupt
out 20h,al
popa
iret
.5:
dw 0 ; cache TSS


irq1:
pusha
call handle_keyboard
popa
iret

irq8:
pusha
call handle_clock
popa
iret

irq12:
pusha
call handle_mouse
popa
iret
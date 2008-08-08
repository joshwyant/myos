global irq0
global irq1
global int0
global int8
global intd
global inte
global int30
extern current_process
extern handle_timer
extern handle_keyboard
extern divide_error
extern double_fault
extern gpfault
extern pgfault
extern syscall


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

intd:
pusha
call gpfault
popa
add esp,4 ; remove error code
iret

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
jz .1
mov [eax],esp
.1:
call handle_timer
test eax,eax
jz .2
mov eax,[current_process]
mov esp,[eax]
mov eax,[eax+8]
mov cr3,eax
.2:
pop gs
pop fs
pop es
pop ds
mov al,20h
out 20h,al
popa
iret

irq1:
pusha
call handle_keyboard
popa
iret
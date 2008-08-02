global irq0
global irq1
global int0
global int8
global intd
global inte
extern current_process
extern handle_timer
extern handle_keyboard
extern divide_error
extern double_fault
extern gpfault
extern pgfault


int0:
pusha
call divide_error
popa
iret

int8:
pusha
call double_fault
popa
iret

intd:
pusha
call gpfault
popa
iret

inte:
pusha
mov eax,[esp+32] ; retreive error code
push eax ; push argument
call pgfault
pop eax ; clear stack
popa
add esp,4 ; remove error code from stack
iret

irq0:
pusha
push ds
push es
push fs
push gs
mov eax,[current_process]
test eax,eax
jz .1
mov [eax], esp
.1:
call handle_timer
mov eax,[current_process]
test eax,eax
jz .2
mov eax,[current_process]
mov esp, [eax]
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
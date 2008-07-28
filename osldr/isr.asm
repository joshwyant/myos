global irq1
global int0
global int8
global intd
global inte
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

irq1:
pusha
call handle_keyboard
popa
iret
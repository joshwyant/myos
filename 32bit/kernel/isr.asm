global irq1
extern handle_keyboard

irq1:
pusha
call handle_keyboard
popa
iret
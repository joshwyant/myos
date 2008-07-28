extern kmain      ; void kmain()
global _start

; Initialization: Go to kmain
_start:
call kmain
; If the kernel's kmain function returns and the kernel code is still running,
; this loop insures that the processor stops until an interrupt is received.
; If the interrupt returns to this code, we need to keep halting. Hopefully
; the interrupt will take us out of this code, and into a process. The kernel's
; design should not allow us to get here anyway, but this is here for completeness.
hang:
hlt
jmp hang
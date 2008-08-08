global kernel_end ; void* kernel_end = image_end
extern kmain      ; void kmain[]
extern image_end  ; char image_end[];

; Initialization: Go to kmain
call kmain
; If the kernel's kmain function returns and the kernel code is still running,
; this loop insures that the processor stops until an interrupt is received.
; If the interrupt returns to this code, we need to keep halting. Hopefully
; the interrupt will take us out of this code, and into a process. The kernel's
; design should not allow us to get here anyway, but this is here for completeness.
hang:
hlt
jmp hang

; org 16
; void* kernel_end = image_end
; kernel_end is defined to be at offset 16 in file, so it
; can be looked up by the loader.
times 16-($-$$) db 0
kernel_end:
dd image_end
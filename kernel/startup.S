global kernel_end ; void* kernel_end = image_end
extern kmain      ; void kmain[]
extern image_end  ; char image_end[];

; Initialization: Go to kmain
call kmain
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
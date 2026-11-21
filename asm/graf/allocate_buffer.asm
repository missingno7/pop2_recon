.MODEL MEDIUM
.DATA
allocate_buffer_callback DW 0
.CODE
PUBLIC ALLOCATE_BUFFER
EXTRN ALLOCATE_FAR_BLOCK:NEAR

ALLOCATE_BUFFER PROC FAR
    mov cx, allocate_buffer_callback
    jcxz default_allocation
    jmp cx

default_allocation:
    push bp
    mov bp, sp
    les ax, DWORD PTR [bp+6]
    push es
    push ax
    push cs
    call ALLOCATE_FAR_BLOCK
    pop bp
    retf 4
ALLOCATE_BUFFER ENDP
END

.MODEL MEDIUM
.DATA
release_buffer_callback DW 0
.CODE
PUBLIC RELEASE_BUFFER
EXTRN CLEAR_FAR_BLOCK_FLAGS:NEAR

RELEASE_BUFFER PROC FAR
    mov cx, release_buffer_callback
    jcxz default_release
    jmp cx

default_release:
    push bp
    mov bp, sp
    les ax, DWORD PTR [bp+6]
    push es
    push ax
    push cs
    call CLEAR_FAR_BLOCK_FLAGS
    pop bp
    retf 4
RELEASE_BUFFER ENDP
END


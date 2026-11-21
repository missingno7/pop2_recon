.MODEL MEDIUM
.DATA
EXTRN BACKING_DRIVER_OFFSET:WORD
EXTRN BACKING_DRIVER_SEGMENT:WORD
EXTRN BACKING_FREE_LOW:WORD
EXTRN BACKING_FREE_HIGH:WORD
.CODE
EXTRN SELECT_BACKING_POSITION:NEAR
EXTRN SELECT_BACKING_EXTENDED:NEAR
EXTRN READ_BACKING_HEADER:NEAR
EXTRN WRITE_BACKING_HEADER:NEAR
PUBLIC RELEASE_BACKING_RECORD, QUERY_BACKING_RECORD_SIZE, QUERY_CALLBACK_U32

RELEASE_BACKING_RECORD PROC FAR
    push bp
    mov bp, sp
    push ds
    push ss
    pop ds
    mov ax, BACKING_DRIVER_OFFSET
    or ax, BACKING_DRIVER_SEGMENT
    jz release_done
    les dx, DWORD PTR [bp+6]
    mov ax, es
    or ax, dx
    jz release_done
    mov cx, es
    sub dx, 4
    sbb cx, 0
    call SELECT_BACKING_POSITION
    call READ_BACKING_HEADER
    push ax
    push dx
    mov dx, 0fffch
    mov cx, 0ffffh
    call SELECT_BACKING_EXTENDED
    pop dx
    pop ax
    and ax, 3fffh
    call WRITE_BACKING_HEADER
    and ax, 01ffh
    add dx, 4
    adc ax, 0
    add BACKING_FREE_LOW, dx
    adc BACKING_FREE_HIGH, ax
release_done:
    pop ds
    pop bp
    retf 4
RELEASE_BACKING_RECORD ENDP

QUERY_BACKING_RECORD_SIZE PROC FAR
    push bp
    mov bp, sp
    push ds
    push ss
    pop ds
    mov ax, BACKING_DRIVER_OFFSET
    or ax, BACKING_DRIVER_SEGMENT
    jz empty
    les dx, DWORD PTR [bp+6]
    mov ax, es
    or ax, dx
    jz empty
    mov cx, es
    sub dx, 4
    sbb cx, 0
    call SELECT_BACKING_POSITION
    call READ_BACKING_HEADER
    mov bx, ax
    and ax, 01ffh
    xchg ax, dx
    and bx, 0600h
    mov cl, 9
    shr bx, cl
    sub ax, bx
    sbb dx, 0
query_done:
    pop ds
    pop bp
    retf 4
empty:
    cwd
    jmp SHORT query_done
QUERY_BACKING_RECORD_SIZE ENDP

QUERY_CALLBACK_U32 PROC FAR
    push bp
    mov bp, sp
    mov al, 0
    call DWORD PTR [bp+6]
    jnc callback_done
    sub ax, ax
    cwd
callback_done:
    pop bp
    retf 4
QUERY_CALLBACK_U32 ENDP
END

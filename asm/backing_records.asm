.MODEL MEDIUM
.DATA
PUBLIC BACKING_DRIVER, BACKING_POSITION, BACKING_FREE, BACKING_FULL
BACKING_DRIVER DD 0
BACKING_POSITION DD 0
BACKING_FREE DD 0
BACKING_FULL DW 0
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
    mov ax, WORD PTR BACKING_DRIVER
    or ax, WORD PTR BACKING_DRIVER+2
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
    and ax, 01ffh
    call WRITE_BACKING_HEADER
    add dx, 4
    adc ax, 0
    add WORD PTR BACKING_FREE, dx
    adc WORD PTR BACKING_FREE+2, ax
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
    mov ax, WORD PTR BACKING_DRIVER
    or ax, WORD PTR BACKING_DRIVER+2
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

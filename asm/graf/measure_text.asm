.MODEL MEDIUM
.DATA
EXTRN CURRENT_CONTEXT:WORD
measure_text_callback DW 0
.CODE
EXTRN MEASURE_COUNTED_TEXT:NEAR
EXTRN SUBTRACT_PORT_ORIGIN:NEAR
EXTRN SECTRECT:NEAR
PUBLIC MEASURE_CHARACTER, PUSH_PORT_CLIP, POP_PORT_CLIP, MEASURE_TEXT

MEASURE_CHARACTER PROC FAR
    push bp
    mov bp, sp
    lea ax, [bp+6]
    push ds
    push ax
    mov ax, 1
    push ax
    push cs
    call MEASURE_COUNTED_TEXT
    pop bp
    retf 2
MEASURE_CHARACTER ENDP

; Save the old clip rectangle on the caller's stack and install its intersection.
PUSH_PORT_CLIP PROC FAR
    mov bx, CURRENT_CONTEXT
    add bx, 20h
    mov cx, 4
    pop ax
    pop es
    pop dx
    pop ds
save_clip:
    sub bx, 2
    push WORD PTR ss:[bx]
    loop save_clip
    push ds
    push dx
    push es
    push ax
    push ss
    pop ds

    push bp
    mov bp, sp
    add sp, -8
    push di
    mov di, bx
    les bx, DWORD PTR [bp+6]
    mov ax, es
    or ax, bx
    jnz intersect_clip
    mov cx, 4
    push ds
    pop es
    rep stosw
    jmp SHORT restore_result
intersect_clip:
    lea ax, [bp-8]
    push ds
    push ax
    push es
    push bx
    add ax, 4
    push ds
    push ax
    add bx, 4
    push es
    push bx
    push cs
    call SUBTRACT_PORT_ORIGIN
    push cs
    call SUBTRACT_PORT_ORIGIN
    push ds
    push di
    push ds
    push di
    push ds
    push ax
    push cs
    call SECTRECT
restore_result:
    les ax, DWORD PTR [bp+6]
    mov dx, es
    pop di
    mov sp, bp
    pop bp
    retf 4
PUSH_PORT_CLIP ENDP

; Consume the four saved words, preserving the caller's far return address.
POP_PORT_CLIP PROC FAR
    pop ax
    pop es
    mov bx, CURRENT_CONTEXT
    add bx, 18h
    mov cx, 4
restore_clip:
    pop WORD PTR [bx]
    add bx, 2
    loop restore_clip
    push es
    push ax
    retf
POP_PORT_CLIP ENDP

MEASURE_TEXT PROC FAR
    jmp WORD PTR measure_text_callback
MEASURE_TEXT ENDP
END

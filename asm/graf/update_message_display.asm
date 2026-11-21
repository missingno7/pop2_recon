.MODEL MEDIUM
.DATA
EXTRN CURRENT_CONTEXT:WORD
EXTRN MESSAGE_DISPLAY_STATE:WORD
EXTRN MESSAGE_CONTEXT:WORD
EXTRN MESSAGE_RECORD:BYTE
.CODE
EXTRN MEASURE_TEXT:NEAR
PUBLIC UPDATE_MESSAGE_DISPLAY

UPDATE_MESSAGE_DISPLAY PROC FAR
    mov ax, MESSAGE_DISPLAY_STATE
    test ax, ax
    jl decrease_only
    jz swap_context
    neg ax
swap_context:
    dec ax
    mov MESSAGE_DISPLAY_STATE, ax
    sub ax, ax
    xchg ax, CURRENT_CONTEXT
    push ax
    mov ax, OFFSET MESSAGE_CONTEXT
    push ax
    mov ax, OFFSET MESSAGE_RECORD
    push ax
    add ax, 4
    push ds
    push ax
    push ds
    push ax
    mov ax, 0
    push ax
    push cs
    call MEASURE_TEXT
    pop CURRENT_CONTEXT
    retf
decrease_only:
    dec ax
    mov MESSAGE_DISPLAY_STATE, ax
    retf
UPDATE_MESSAGE_DISPLAY ENDP
END

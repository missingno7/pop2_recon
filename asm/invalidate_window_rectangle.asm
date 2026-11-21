.MODEL MEDIUM
.DATA
EXTRN SCREEN_CONTEXT:WORD
EXTRN CURRENT_CONTEXT:WORD
EXTRN WINDOW_COLOR:WORD
EXTRN WINDOW_HEAD:WORD
.CODE
EXTRN SECTRECT:NEAR
EXTRN SET_RECORD_RANGE:NEAR
EXTRN UNIONRECT:NEAR
EXTRN SUBTRACT_RECTANGLE_STRIP:NEAR
EXTRN EMPTYRECT:NEAR
EXTRN TRIM_PENDING_WINDOW_UPDATES:NEAR
PUBLIC INVALIDATE_WINDOW_RECTANGLE

INVALIDATE_WINDOW_RECTANGLE PROC FAR
    push bp
    mov bp, sp
    add sp, -8
    push di
    push si
    lea si, [bp-8]
    push ds
    push si
    mov ax, SCREEN_CONTEXT
    add ax, 10h
    push ds
    push ax
    push ds
    push WORD PTR [bp+8]
    push cs
    call SECTRECT
    cmp WORD PTR [bp+6], 0
    jne start_windows
    mov ax, SCREEN_CONTEXT
    xchg CURRENT_CONTEXT, ax
    push ax
    push ds
    push si
    push WINDOW_COLOR
    push cs
    call SET_RECORD_RANGE
    pop CURRENT_CONTEXT
start_windows:
    mov cx, WINDOW_HEAD
inspect_window:
    jcxz invalidate_done
    mov di, cx
    lea ax, [bp-8]
    lea si, [di+48h]
    mov cx, [di+46h]
    jcxz clip_window
    push ds
    push si
    push ds
    push si
    push ds
    push ax
    push cs
    call UNIONRECT
clip_window:
    push ds
    push si
    push ds
    push ax
    lea ax, [di+3eh]
    push ds
    push ax
    push cs
    call SECTRECT
    mov [di+46h], ax
    lea ax, [bp-8]
    push ds
    push ax
    push ds
    push ax
    push ds
    push si
    push cs
    call SUBTRACT_RECTANGLE_STRIP
    push ds
    push ax
    push cs
    call EMPTYRECT
    xchg cx, ax
    jcxz next_window
    push WORD PTR [bp+8]
    push WORD PTR [di+2ch]
    push cs
    call TRIM_PENDING_WINDOW_UPDATES
    jmp SHORT invalidate_done
next_window:
    mov cx, [di+2ch]
    cmp di, [bp+6]
    jne inspect_window
invalidate_done:
    pop si
    pop di
    mov sp, bp
    pop bp
    retf 4
INVALIDATE_WINDOW_RECTANGLE ENDP
END

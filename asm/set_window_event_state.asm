.MODEL MEDIUM
.DATA
EXTRN WINDOW_HEAD:WORD
.CODE
EXTRN INVALIDATE_WINDOW_RECTANGLE:NEAR
PUBLIC SET_WINDOW_EVENT_STATE

SET_WINDOW_EVENT_STATE PROC FAR
    push bp
    mov bp, sp
    push di
    mov di, [bp+8]
    mov cx, [bp+6]
    test cx, cx
    xchg [di+34h], cx
    jcxz check_previous_state
    jne state_done
    jmp SHORT notify_state_change
check_previous_state:
    jz state_done
notify_state_change:
    push di
    mov ax, 2
    push ax
    sub ax, ax
    push ax
    call DWORD PTR [di+2eh]
    mov bx, WINDOW_HEAD
    cmp bx, di
    je state_done
find_previous_window:
    cmp di, [bx+2ch]
    je invalidate_window
    mov bx, [bx+2ch]
    jmp SHORT find_previous_window
invalidate_window:
    lea ax, [di+3eh]
    push ax
    push bx
    push cs
    call INVALIDATE_WINDOW_RECTANGLE
state_done:
    pop di
    pop bp
    retf 4
SET_WINDOW_EVENT_STATE ENDP
END

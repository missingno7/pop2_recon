.MODEL MEDIUM
.DATA
EXTRN WINDOW_LIST:BYTE
EXTRN WINDOW_HEAD:WORD
EXTRN SCREEN_CONTEXT:WORD
EXTRN CURRENT_CONTEXT:WORD
.CODE
EXTRN SECTRECT:NEAR
EXTRN UNIONRECT:NEAR
EXTRN INVALIDATE_WINDOW_RECTANGLE:NEAR
EXTRN UPDATE_MESSAGE_DISPLAY:NEAR
EXTRN PUSH_PORT_CLIP:NEAR
EXTRN POP_PORT_CLIP:NEAR
EXTRN STROKE_PORT_RECT:NEAR
PUBLIC PAINT_PENDING_WINDOW, BRING_WINDOW_FORWARD

PAINT_PENDING_WINDOW PROC FAR
    pop ax
    pop es
    pop dx
    mov bx, dx
    add bx, 20h
    mov cx, 4
save_window_clip:
    sub bx, 2
    push WORD PTR [bx]
    loop save_window_clip
    push dx
    push es
    push ax
    push bp
    mov bp, sp
    push di
    push cs
    call UPDATE_MESSAGE_DISPLAY
    mov ax, SCREEN_CONTEXT
    xchg CURRENT_CONTEXT, ax
    push ax
    mov di, [bp+6]
    mov cx, [di+46h]
    jcxz window_paint_done
    lea ax, [di+48h]
    push ds
    push ax
    push cs
    call PUSH_PORT_CLIP
    sub ax, ax
    mov [di+46h], ax
    push di
    mov dx, 2
    push dx
    push ax
    call DWORD PTR [di+2eh]
    push cs
    call POP_PORT_CLIP
    mov CURRENT_CONTEXT, di
    lea ax, [di+18h]
    push ds
    push ax
    push ds
    push ax
    lea ax, [di+48h]
    push ds
    push ax
    push cs
    call SECTRECT
    lea ax, [di+10h]
    push ds
    push ax
    push cs
    call STROKE_PORT_RECT
window_paint_done:
    pop CURRENT_CONTEXT
    pop di
    pop bp
    retf 2
PAINT_PENDING_WINDOW ENDP

BRING_WINDOW_FORWARD PROC FAR
    push bp
    mov bp, sp
    add sp, -12h
    push di
    push si
    mov WORD PTR [bp-2], 0
    mov di, [bp+6]
    mov si, OFFSET WINDOW_LIST
inspect_preceding_window:
    cmp di, [si+2ch]
    je relink_window
    mov si, [si+2ch]
    lea ax, [bp-12h]
    push ds
    push ax
    lea ax, [di+3eh]
    push ds
    push ax
    lea ax, [si+3eh]
    push ds
    push ax
    push cs
    call SECTRECT
    xchg cx, ax
    jcxz inspect_preceding_window
    push di
    push si
    lea di, [bp-0ah]
    lea si, [bp-12h]
    mov cx, 1
    xchg [bp-2], cx
    jcxz copy_first_overlap
    push ds
    push di
    push ds
    push di
    push ds
    push si
    push cs
    call UNIONRECT
    jmp SHORT restore_windows
copy_first_overlap:
    push ds
    pop es
    mov cx, 4
    rep movsw
restore_windows:
    pop si
    pop di
    jmp SHORT inspect_preceding_window
relink_window:
    mov ax, [di+2ch]
    mov [si+2ch], ax
    mov ax, di
    xchg WINDOW_HEAD, ax
    mov [di+2ch], ax
    mov cx, [bp-2]
    jcxz window_front_done
    lea ax, [bp-0ah]
    push ax
    push di
    push cs
    call INVALIDATE_WINDOW_RECTANGLE
window_front_done:
    pop si
    pop di
    mov sp, bp
    pop bp
    retf 2
BRING_WINDOW_FORWARD ENDP
END

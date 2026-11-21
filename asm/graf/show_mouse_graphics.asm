.MODEL MEDIUM
.DATA
EXTRN CURSOR_SHOW_COUNT:WORD
EXTRN CURSOR_PENDING:WORD
EXTRN CURSOR_PORT:BYTE
EXTRN CURRENT_CONTEXT:WORD
EXTRN CURSOR_X:WORD
EXTRN CURSOR_Y:WORD
EXTRN CURSOR_HOT_X:WORD
EXTRN CURSOR_HOT_Y:WORD
EXTRN CURSOR_Y0:WORD
EXTRN CURSOR_X0:WORD
EXTRN CURSOR_Y1:WORD
EXTRN CURSOR_X1:WORD
EXTRN CURSOR_BACKGROUND:BYTE
EXTRN CURSOR_MASK_HANDLE:WORD
EXTRN CURSOR_IMAGE_HANDLE:WORD
cursor_plane_mask DW 0fh
.CODE
EXTRN UPDATE_MESSAGE_DISPLAY:NEAR
EXTRN BLIT_RECT:NEAR
EXTRN RESOLVE_HANDLE_TO_FAR_PAYLOAD:NEAR
EXTRN DRAW_MASK:NEAR
PUBLIC SHOW_MOUSE_GRAPHICS

SHOW_MOUSE_GRAPHICS PROC FAR
    mov cx, CURSOR_SHOW_COUNT
    jcxz mouse_show_done
    cmp cx, -1
    jl increment_show_count
    je repaint_cursor
    dec cx
    mov CURSOR_SHOW_COUNT, cx
    jne mouse_show_done
    xchg CURSOR_PENDING, cx
    jcxz mouse_show_done
    push cs
    call UPDATE_MESSAGE_DISPLAY
    jmp SHORT repaint_cursor
increment_show_count:
    inc CURSOR_SHOW_COUNT
mouse_show_done:
    retf
repaint_cursor:
    mov bx, OFFSET CURSOR_PORT
    mov ax, bx
    xchg CURRENT_CONTEXT, ax
    push ax
    mov ax, CURSOR_X
    sub ax, CURSOR_HOT_X
    add ax, [bx+6]
    mov CURSOR_X0, ax
    add ax, 10h
    mov CURSOR_X1, ax
    mov ax, CURSOR_Y
    sub ax, CURSOR_HOT_Y
    add ax, [bx+4]
    mov CURSOR_Y0, ax
    add ax, 10h
    mov CURSOR_Y1, ax
    mov ax, OFFSET CURSOR_BACKGROUND
    push ax
    push bx
    add ax, 4
    push ds
    push ax
    push ds
    push ax
    mov ax, 0
    push ax
    push cs
    call BLIT_RECT
    push CURSOR_MASK_HANDLE
    push cs
    call RESOLVE_HANDLE_TO_FAR_PAYLOAD
    push dx
    push ax
    les ax, DWORD PTR CURSOR_Y0
    push es
    push ax
    mov ax, 1
    push ax
    mov ax, -1
    push ax
    push cs
    call DRAW_MASK
    push CURSOR_IMAGE_HANDLE
    push cs
    call RESOLVE_HANDLE_TO_FAR_PAYLOAD
    push dx
    push ax
    les ax, DWORD PTR CURSOR_Y0
    push es
    push ax
    mov ax, 3
    push ax
    push cursor_plane_mask
    push cs
    call DRAW_MASK
    pop CURRENT_CONTEXT
    jmp increment_show_count
SHOW_MOUSE_GRAPHICS ENDP
END

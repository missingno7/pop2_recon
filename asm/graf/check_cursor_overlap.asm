.MODEL MEDIUM
.DATA
EXTRN CURSOR_SHOW_COUNT:WORD
EXTRN CURSOR_PORT:WORD
EXTRN CURSOR_PORT_SEGMENT:WORD
EXTRN CURSOR_NEAR_CORNER:DWORD
EXTRN CURSOR_FAR_CORNER:DWORD
cursor_port_origin DD 0
.CODE
EXTRN UPDATE_MESSAGE_DISPLAY:NEAR
PUBLIC CHECK_CURSOR_OVERLAP

CHECK_CURSOR_OVERLAP PROC NEAR
    cmp CURSOR_SHOW_COUNT, 0
    jge check_port
    dec CURSOR_SHOW_COUNT
    ret
check_port:
    push ax
    push bx
    push cx
    push dx
    push es
    cmp ax, CURSOR_PORT
    jne increment_cursor_count
    mov ax, es
    cmp ax, CURSOR_PORT_SEGMENT
    jne increment_cursor_count
    les dx, cursor_port_origin
    mov cx, es
    les ax, CURSOR_FAR_CORNER
    sub ax, dx
    cmp ax, [bp-8]
    jle increment_cursor_count
    mov ax, es
    sub ax, cx
    cmp ax, [bp-6]
    jle increment_cursor_count
    les ax, CURSOR_NEAR_CORNER
    sub ax, dx
    cmp ax, [bp-4]
    jge increment_cursor_count
    mov ax, es
    sub ax, cx
    cmp ax, [bp-2]
    jge increment_cursor_count
    push cs
    call UPDATE_MESSAGE_DISPLAY
    jmp SHORT overlap_done
increment_cursor_count:
    inc CURSOR_SHOW_COUNT
overlap_done:
    pop es
    pop dx
    pop cx
    pop bx
    pop ax
    ret
CHECK_CURSOR_OVERLAP ENDP
END

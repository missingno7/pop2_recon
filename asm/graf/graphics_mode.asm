.MODEL MEDIUM
.DATA
mode_snapshot DW 0, 320, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
EXTRN DEFAULT_PORT:WORD
EXTRN CURRENT_CONTEXT:WORD
EXTRN CURRENT_PORT_STORAGE:WORD
EXTRN CURRENT_GRAPHICS_FONT:WORD
EXTRN SAVED_BIOS_EQUIPMENT:WORD
EXTRN SAVED_BIOS_MODE:BYTE
EXTRN MODE_TABLE:WORD
.CODE
EXTRN SET_RECORD_RANGE:NEAR
EXTRN SHUTDOWN_MOUSE_GRAPHICS:NEAR
EXTRN FREE_NEAR_HEAP:FAR
EXTRN DISPOSE_HANDLE:NEAR
PUBLIC SHUTDOWN_GRAPHICS_MODE, SELECT_GRAPHICS_MODE, CHECK_GRAPHICS_MODE

SHUTDOWN_GRAPHICS_MODE PROC FAR
    sub cx, cx
    xchg cx, mode_snapshot
    jcxz shutdown_done
    mov ax, OFFSET DEFAULT_PORT
    mov CURRENT_CONTEXT, ax
    add ax, 4
    push ds
    push ax
    mov ax, 0
    push ax
    push cs
    call SET_RECORD_RANGE
    push cs
    call SHUTDOWN_MOUSE_GRAPHICS
    push cs
    call WORD PTR [mode_snapshot+12]
    sub ax, ax
    xchg ax, CURRENT_PORT_STORAGE
    push ax
    call FREE_NEAR_HEAP
    add sp, 2
    push CURRENT_GRAPHICS_FONT
    push cs
    call DISPOSE_HANDLE
    sub ax, ax
    mov es, ax
    mov ax, SAVED_BIOS_EQUIPMENT
    mov es:[410h], ax
    sub ah, ah
    mov al, SAVED_BIOS_MODE
    int 10h
shutdown_done:
    retf
SHUTDOWN_GRAPHICS_MODE ENDP

SELECT_GRAPHICS_MODE PROC FAR
    push bp
    mov bp, sp
    push di
    mov di, [bp+6]
    test di, di
    jz select_default
    call CHECK_GRAPHICS_MODE
    jc selected
    sub di, di
selected:
    xchg ax, di
    pop di
    pop bp
    retf 2
select_default:
    mov di, MODE_TABLE
try_mode:
    call CHECK_GRAPHICS_MODE
    jc selected
    dec di
    jnz try_mode
    jmp SHORT selected
SELECT_GRAPHICS_MODE ENDP

CHECK_GRAPHICS_MODE PROC NEAR
    cmp di, MODE_TABLE
    ja unavailable
    mov bx, di
    shl bx, 1
    mov bx, MODE_TABLE[bx]
    test bx, bx
    jz unavailable
    mov ax, [mode_snapshot+2]
    cmp ax, cs:[bx+2]
    jne unavailable
    push cs
    call WORD PTR cs:[bx+8]
    shr ax, 1
check_done:
    ret
unavailable:
    clc
    jmp SHORT check_done
CHECK_GRAPHICS_MODE ENDP
END

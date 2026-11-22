.MODEL MEDIUM
.DATA
EXTRN CURRENT_CONTEXT:WORD
EXTRN CURRENT_GRAPHICS_FONT:WORD
EXTRN DEFAULT_PORT:WORD
EXTRN MODE_TABLE:WORD
EXTRN SAVED_BIOS_EQUIPMENT:WORD
EXTRN SAVED_BIOS_MODE:BYTE
EXTRN CURRENT_PORT_STORAGE:WORD
mode_snapshot DW 0, 320, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
.CODE
EXTRN HGETFONT:NEAR
EXTRN INIT_PORT:NEAR
EXTRN STROKE_PORT_RECT:NEAR
EXTRN INITIALIZE_MOUSE_GRAPHICS:NEAR
EXTRN SET_RECORD_RANGE:NEAR
EXTRN SHUTDOWN_MOUSE_GRAPHICS:NEAR
EXTRN FREE_NEAR_HEAP:FAR
EXTRN DISPOSE_HANDLE:NEAR

PUBLIC COPY_CURRENT_PORT_ORIGIN

COPY_CURRENT_PORT_ORIGIN PROC FAR
    push bp
    mov bp, sp
    push di
    push si
    mov cx, 8
    mov si, CURRENT_CONTEXT
    add si, 22h
    les di, DWORD PTR [bp+6]
    rep movsb
    pop si
    pop di
    pop bp
    retf 4
COPY_CURRENT_PORT_ORIGIN ENDP

PUBLIC INITIALIZE_GRAPHICS_MODE

; Save the BIOS state, initialize the adapter and install its dispatch table.
INITIALIZE_GRAPHICS_MODE PROC FAR
    push bp
    mov bp, sp
    push di
    push si
    sub ax, ax
    mov es, ax
    mov ax, es:[410h]
    mov SAVED_BIOS_EQUIPMENT, ax
    and ax, 0ffcfh
    or ax, 10h
    mov es:[410h], ax
    mov al, es:[449h]
    mov SAVED_BIOS_MODE, al
    mov si, [bp+6]
    shl si, 1
    mov si, MODE_TABLE[si]
    push cs
    call WORD PTR cs:[si+0ah]
    push ds
    push cs
    pop ds
    mov cx, 15
    mov di, OFFSET mode_snapshot
    push ss
    pop es
    rep movsw
    pop ds

    ; Install the default font, port and clip before starting the mouse.
    push cx
    push cs
    call HGETFONT
    mov CURRENT_GRAPHICS_FONT, ax
    mov ax, OFFSET DEFAULT_PORT
    push ax
    add ax, 4
    push ds
    push ax
    push cs
    call INIT_PORT
    mov CURRENT_CONTEXT, ax
    add ax, 4
    push ds
    push ax
    push cs
    call STROKE_PORT_RECT
    push cs
    call INITIALIZE_MOUSE_GRAPHICS
    pop si
    pop di
    pop bp
    retf 2
INITIALIZE_GRAPHICS_MODE ENDP

PUBLIC SHUTDOWN_GRAPHICS_MODE, SELECT_GRAPHICS_MODE, CHECK_GRAPHICS_MODE

SHUTDOWN_GRAPHICS_MODE PROC FAR
    sub cx, cx
    xchg cx, mode_snapshot
    jcxz shutdown_done_4f10
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
shutdown_done_4f10:
    retf
SHUTDOWN_GRAPHICS_MODE ENDP

SELECT_GRAPHICS_MODE PROC FAR
    push bp
    mov bp, sp
    push di
    mov di, [bp+6]
    test di, di
    jz select_default_4f10
    call CHECK_GRAPHICS_MODE
    jc selected_4f10
    sub di, di
selected_4f10:
    xchg ax, di
    pop di
    pop bp
    retf 2
select_default_4f10:
    mov di, MODE_TABLE
try_mode_4f10:
    call CHECK_GRAPHICS_MODE
    jc selected_4f10
    dec di
    jnz try_mode_4f10
    jmp SHORT selected_4f10
SELECT_GRAPHICS_MODE ENDP

CHECK_GRAPHICS_MODE PROC NEAR
    cmp di, MODE_TABLE
    ja unavailable_4f10
    mov bx, di
    shl bx, 1
    mov bx, MODE_TABLE[bx]
    test bx, bx
    jz unavailable_4f10
    mov ax, [mode_snapshot+2]
    cmp ax, cs:[bx+2]
    jne unavailable_4f10
    push cs
    call WORD PTR cs:[bx+8]
    shr ax, 1
check_done_4f10:
    ret
unavailable_4f10:
    clc
    jmp SHORT check_done_4f10
CHECK_GRAPHICS_MODE ENDP
END

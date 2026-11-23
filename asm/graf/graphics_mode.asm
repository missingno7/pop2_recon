.MODEL MEDIUM
.DATA
EXTRN Graf:WORD
EXTRN MODE_TABLE:WORD
EXTRN SAVED_BIOS_EQUIPMENT:WORD
EXTRN SAVED_BIOS_MODE:BYTE
; Default mode record. Initialization replaces all fifteen words.
PUBLIC SCREEN_WIDTH, PIXELS_PER_BYTE
mode_snapshot DW 0
SCREEN_WIDTH DW 320
    DW 0
PIXELS_PER_BYTE LABEL BYTE
    DW 0, 0, 0, 0
PUBLIC RELEASE_BUFFER_CALLBACK
RELEASE_BUFFER_CALLBACK DW 0
mode_reserved DW 0
PUBLIC ALLOCATE_BUFFER_CALLBACK
ALLOCATE_BUFFER_CALLBACK DW 0
PUBLIC MEASURE_TEXT_CALLBACK
MEASURE_TEXT_CALLBACK DW 0
PUBLIC AXIS_SEGMENT_CALLBACK
AXIS_SEGMENT_CALLBACK DW 0
PUBLIC PAINT_FOUR_BIT_CALLBACK
PAINT_FOUR_BIT_CALLBACK DW 0
PUBLIC STROKE_RECTANGLE_CALLBACK
STROKE_RECTANGLE_CALLBACK DW 0
PUBLIC PAINT_PLAIN_CALLBACK
PAINT_PLAIN_CALLBACK DW 0

; INIT_PORT copies the complete 2Ch-byte record before applying a caller rectangle.
PUBLIC DEFAULT_PORT, DEFAULT_PORT_SEGMENT, DEFAULT_PORT_ORIGIN
PUBLIC DEFAULT_PORT_LIMITS, DEFAULT_PORT_MOUSE_BOUNDS, DEFAULT_PORT_MOUSE_MAX_X
DEFAULT_PORT DW 0
DEFAULT_PORT_SEGMENT DW 0
DEFAULT_PORT_ORIGIN LABEL DWORD
    DW 0, 0
DEFAULT_PORT_LIMITS LABEL DWORD
    DW 0, 0
default_port_pitch DW 0
default_port_row_table DW 0
DEFAULT_PORT_MOUSE_BOUNDS LABEL WORD
    DW 0, 0
default_port_mouse_max_y DW 0
DEFAULT_PORT_MOUSE_MAX_X DW 0
default_port_clip DW 0, 0, 0, 0
default_port_attributes DW 0
default_port_origin_transform DW 0, 0, 0, 0fh
CURRENT_GRAPHICS_FONT DW 0

.CODE
EXTRN HGETFONT:NEAR
EXTRN INIT_PORT:NEAR
EXTRN STROKE_PORT_RECT:NEAR
EXTRN INITIALIZE_MOUSE_GRAPHICS:NEAR
EXTRN SET_RECORD_RANGE:NEAR
EXTRN SHUTDOWN_MOUSE_GRAPHICS:NEAR
EXTRN __nfree:FAR
EXTRN DISPOSE_HANDLE:NEAR

PUBLIC COPY_CURRENT_PORT_ORIGIN

COPY_CURRENT_PORT_ORIGIN PROC FAR
    push bp
    mov bp, sp
    push di
    push si
    mov cx, 8
    mov si, Graf
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
    mov Graf, ax
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
    mov Graf, ax
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
    xchg ax, default_port_row_table
    push ax
    call __nfree
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

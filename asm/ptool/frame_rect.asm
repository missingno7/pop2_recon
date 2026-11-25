.MODEL MEDIUM
.DATA
EXTRN Graf:WORD
.CODE
EXTRN STROKE_RECTANGLE:NEAR

PUBLIC ERASERECT

ERASERECT PROC FAR
    push bp
    mov bp, sp
    les ax, DWORD PTR [bp+6]
    push es
    push ax
    mov ax, 0
    push ax
    mov bx, Graf
    push WORD PTR [bx+20h]
    push cs
    call STROKE_RECTANGLE
    pop bp
    retf 4
ERASERECT ENDP
PUBLIC alignment_4dab
alignment_4dab DB 0  ; Link-address alignment between routines.

PUBLIC FRAMERECT

draw_strip MACRO
    push ds
    push di
    push WORD PTR [bp-4]
    push WORD PTR [bp-2]
    push cs
    call STROKE_RECTANGLE
ENDM

FRAMERECT PROC FAR
    push bp
    mov bp, sp
    add sp, -12
    push di
    push si
    mov bx, Graf
    mov ax, [bx+26h]
    mov [bp-4], ax
    mov ax, [bx+28h]
    mov [bp-2], ax
    push ds
    mov cx, 4
    lea di, [bp-12]
    lds si, DWORD PTR [bp+6]
    push ss
    pop es
    rep movsw
    pop ds
    lea di, [bp-12]
    les si, DWORD PTR [bp+6]
    mov ax, es:[si]
    inc ax
    cmp ax, [di+4]
    jg done_4dac
    push ax
    mov [di+4], ax
    je bottom_edge_4dac
    push es
    draw_strip
    pop es

bottom_edge_4dac:
    mov ax, es:[si+4]
    mov [di+4], ax
    dec ax
    mov [di], ax
    push es
    draw_strip
    pop es
    pop WORD PTR [di]
    dec WORD PTR [di+4]
    mov ax, es:[si+2]
    inc ax
    cmp ax, [di+6]
    jg done_4dac
    je right_edge_4dac
    mov [di+6], ax
    push es
    draw_strip
    pop es

right_edge_4dac:
    mov ax, es:[si+6]
    mov [di+6], ax
    dec ax
    mov [di+2], ax
    draw_strip
done_4dac:
    xchg ax, si
    mov dx, [bp+8]
    pop si
    pop di
    mov sp, bp
    pop bp
    retf 4
FRAMERECT ENDP
END

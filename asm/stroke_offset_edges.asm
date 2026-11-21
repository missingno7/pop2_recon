.MODEL MEDIUM
.CODE
EXTRN STROKE_RECTANGLE:NEAR
PUBLIC STROKE_OFFSET_EDGES

STROKE_OFFSET_EDGES PROC FAR
    push bp
    mov bp, sp
    add sp, -8
    push di
    push si
    mov si, [bp+14]
    lea di, [bp-8]
    mov ax, [si+6]
    mov [di+2], ax
    add ax, [bp+12]
    mov [di+6], ax
    mov ax, [si]
    add ax, [bp+10]
    mov [di], ax
    mov ax, [si+4]
    add ax, [bp+10]
    mov [di+4], ax
    push ds
    push di
    push WORD PTR [bp+8]
    push WORD PTR [bp+6]
    push cs
    call STROKE_RECTANGLE
    mov ax, [si+2]
    mov cx, [bp+12]
    add ax, cx
    mov [di+2], ax
    sub [di+6], cx
    mov ax, [si+4]
    mov [di], ax
    push ds
    push di
    push WORD PTR [bp+8]
    push WORD PTR [bp+6]
    push cs
    call STROKE_RECTANGLE
    xchg si, ax
    pop si
    pop di
    mov sp, bp
    pop bp
    retf 10
STROKE_OFFSET_EDGES ENDP
END

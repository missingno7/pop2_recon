.MODEL MEDIUM
.DATA
EXTRN Graf:WORD
.CODE
EXTRN DRAW_COUNTED_TEXT_IN_RECT:NEAR
EXTRN MEASURE_CHARACTER:NEAR
EXTRN H2P:NEAR

PUBLIC SETORIGIN, SET_RELATIVE_PEN_STATE
PUBLIC STRINGRECT, MEASURE_NUL_TERMINATED_TEXT

SETORIGIN PROC FAR
    push bp
    mov bp, sp
    les dx, DWORD PTR [bp+6]
    mov cx, es
    mov bx, Graf
    sub cx, [bx+12h]
    sub dx, [bx+10h]
    add [bx+12h], cx
    add [bx+10h], dx
    add [bx+16h], cx
    add [bx+14h], dx
    add [bx+6], cx
    add [bx+4], dx
    add [bx+0ah], cx
    add [bx+8], dx
    pop bp
    retf 4
SETORIGIN ENDP

SET_RELATIVE_PEN_STATE PROC FAR
    push bp
    mov bp, sp
    push di
    push si
    push ds
    mov cx, 8
    mov di, Graf
    add di, 22h
    push ds
    pop es
    lds si, DWORD PTR [bp+6]
    rep movsb
    pop ds
    pop si
    pop di
    pop bp
    retf 4
SET_RELATIVE_PEN_STATE ENDP

STRINGRECT PROC FAR
    push bp
    mov bp, sp
    push di
    les ax, DWORD PTR [bp+0eh]
    push es
    push ax
    push WORD PTR [bp+0ch]
    push WORD PTR [bp+0ah]
    les di, DWORD PTR [bp+6]
    push es
    push di
    sub ax, ax
    mov cx, 0ffffh
    repne scasb
    not cx
    dec cx
    push cx
    push cs
    call DRAW_COUNTED_TEXT_IN_RECT
    pop di
    pop bp
    retf 12
STRINGRECT ENDP

MEASURE_NUL_TERMINATED_TEXT PROC FAR
    push bp
    mov bp, sp
    push di
    les di, DWORD PTR [bp+6]
    push es
    push di
    sub ax, ax
    mov cx, 0ffffh
    repne scasb
    not cx
    dec cx
    push cx
    push cs
    call NEAR PTR MEASURE_COUNTED_TEXT
    pop di
    pop bp
    retf 4
MEASURE_NUL_TERMINATED_TEXT ENDP
PUBLIC alignment_5397
alignment_5397 DB 0  ; Link-address alignment between routines.

PUBLIC FIT_TEXT_PREFIX

FIT_TEXT_PREFIX PROC FAR
    push bp
    mov bp, sp
    add sp, -4
    push di
    push si
    sub di, di
    mov [bp-2], di
    mov [bp-4], di
    les si, DWORD PTR [bp+12]
next_char_5398:
    cmp di, [bp+10]
    je count_read_5398
    push es
    push WORD PTR es:[si]
    push cs
    call MEASURE_CHARACTER
    pop es
    add ax, [bp-4]
    mov [bp-4], ax
    cmp ax, [bp+8]
    ja exceeds_width_5398
    inc di
    lods BYTE PTR es:[si]
    cmp al, 0dh
    je count_read_5398
    cmp al, '-'
    je save_break_5398
    cmp WORD PTR [bp+6], 0
    jg before_next_word_5398
    cmp al, ' '
    je save_break_5398
    cmp BYTE PTR es:[si], ' '
    je save_break_5398
    jmp SHORT next_char_5398
before_next_word_5398:
    cmp BYTE PTR es:[si], ' '
    jne next_char_5398
    cmp al, ' '
    je next_char_5398
save_break_5398:
    mov [bp-2], di
    jmp SHORT next_char_5398
exceeds_width_5398:
    mov ax, [bp-2]
    test ax, ax
    jnz done_5398
count_read_5398:
    xchg ax, di
done_5398:
    pop si
    pop di
    mov sp, bp
    pop bp
    retf 10
FIT_TEXT_PREFIX ENDP

PUBLIC MEASURE_COUNTED_TEXT

MEASURE_COUNTED_TEXT PROC FAR
    push bp
    mov bp, sp
    push di
    push si
    push ds
    mov di, Graf
    push WORD PTR [di+2ah]
    push cs
    call H2P
    xchg ax, bx
    mov ds, dx
    sub ax, ax
    mov cx, [bp+6]
    jcxz done_5400
    cwd
    les si, DWORD PTR [bp+8]
glyph_5400:
    lods BYTE PTR es:[si]
    cmp al, [bx+1]
    ja next_glyph_5400
    sub al, [bx]
    jb next_glyph_5400
    mov di, ax
    shl di, 1
    mov di, [bx+di+0ah]
    add di, bx
    add dx, [di+2]
    add dx, [bx+8]
next_glyph_5400:
    loop glyph_5400
    xchg ax, dx
done_5400:
    pop ds
    pop si
    pop di
    pop bp
    retf 6
MEASURE_COUNTED_TEXT ENDP
END

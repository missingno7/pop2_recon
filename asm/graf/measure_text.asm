.MODEL MEDIUM
.DATA
EXTRN Graf:WORD
EXTRN MEASURE_TEXT_CALLBACK:WORD
.CODE
EXTRN MEASURE_COUNTED_TEXT:NEAR
EXTRN SUBTRACT_PORT_ORIGIN:NEAR
EXTRN SECTRECT:NEAR
EXTRN H2P:NEAR
EXTRN PAINT4BITSHAPE:NEAR

PUBLIC MEASURE_CHARACTER, CLIPRECT, CLIPRESTORE, COPYBITS

MEASURE_CHARACTER PROC FAR
    push bp
    mov bp, sp
    lea ax, [bp+6]
    push ds
    push ax
    mov ax, 1
    push ax
    push cs
    call MEASURE_COUNTED_TEXT
    pop bp
    retf 2
MEASURE_CHARACTER ENDP

; Save the old clip rectangle on the caller's stack and install its intersection.
CLIPRECT PROC FAR
    mov bx, Graf
    add bx, 20h
    mov cx, 4
    pop ax
    pop es
    pop dx
    pop ds
save_clip_4c3e:
    sub bx, 2
    push WORD PTR ss:[bx]
    loop save_clip_4c3e
    push ds
    push dx
    push es
    push ax
    push ss
    pop ds

    push bp
    mov bp, sp
    add sp, -8
    push di
    mov di, bx
    les bx, DWORD PTR [bp+6]
    mov ax, es
    or ax, bx
    jnz intersect_clip_4c3e
    mov cx, 4
    push ds
    pop es
    rep stosw
    jmp SHORT restore_result_4c3e
intersect_clip_4c3e:
    lea ax, [bp-8]
    push ds
    push ax
    push es
    push bx
    add ax, 4
    push ds
    push ax
    add bx, 4
    push es
    push bx
    push cs
    call SUBTRACT_PORT_ORIGIN
    push cs
    call SUBTRACT_PORT_ORIGIN
    push ds
    push di
    push ds
    push di
    push ds
    push ax
    push cs
    call SECTRECT
restore_result_4c3e:
    les ax, DWORD PTR [bp+6]
    mov dx, es
    pop di
    mov sp, bp
    pop bp
    retf 4
CLIPRECT ENDP

; Consume the four saved words, preserving the caller's far return address.
CLIPRESTORE PROC FAR
    pop ax
    pop es
    mov bx, Graf
    add bx, 18h
    mov cx, 4
restore_clip_4c3e:
    pop WORD PTR [bx]
    add bx, 2
    loop restore_clip_4c3e
    push es
    push ax
    retf
CLIPRESTORE ENDP

COPYBITS PROC FAR
    jmp WORD PTR measure_text_callback
COPYBITS ENDP

PUBLIC DRAWSTRING

DRAWSTRING PROC FAR
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
    call NEAR PTR DRAW_COUNTED_TEXT
    pop di
    pop bp
    retf 4
DRAWSTRING ENDP
PUBLIC alignment_4cef
alignment_4cef DB 0  ; Link-address alignment between routines.

PUBLIC DRAW_COUNTED_TEXT

DRAW_COUNTED_TEXT PROC FAR
    push bp
    mov bp, sp
    add sp, -8
    push di
    push si
    mov di, Graf
    les si, DWORD PTR [di+22h]
    mov [bp-6], es
    push WORD PTR [di+2ah]
    push cs
    call H2P
    mov [bp-4], ax
    mov [bp-2], dx
    xchg ax, si
    mov es, dx
    sub ax, es:[si+2]
    mov [bp-8], ax
    les si, DWORD PTR [bp+8]
next_glyph_4cf0:
    sub WORD PTR [bp+6], 1
    jb done_4cf0
    lods BYTE PTR es:[si]
    push es
    push si
    les si, DWORD PTR [bp-4]
    cmp al, es:[si+1]
    ja advance_4cf0
    sub al, es:[si]
    jb advance_4cf0
    sub ah, ah
    shl ax, 1
    xchg ax, bx
    mov bx, es:[bx+si+0ah]
    add bx, si
    push es
    push bx
    push WORD PTR [di+24h]
    push WORD PTR [bp-8]
    push WORD PTR [di+26h]
    push WORD PTR [di+28h]
    mov ax, es:[bx+2]
    add ax, es:[si+8]
    add [di+24h], ax
    push cs
    call PAINT4BITSHAPE
advance_4cf0:
    pop si
    pop es
    jmp SHORT next_glyph_4cf0
done_4cf0:
    mov ax, [di+24h]
    sub ax, [bp-6]
    pop si
    pop di
    mov sp, bp
    pop bp
    retf 6
DRAW_COUNTED_TEXT ENDP
END

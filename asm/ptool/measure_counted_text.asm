.MODEL MEDIUM
.DATA
EXTRN CURRENT_CONTEXT:WORD
.CODE
EXTRN RESOLVE_HANDLE_TO_FAR_PAYLOAD:NEAR
PUBLIC MEASURE_COUNTED_TEXT

MEASURE_COUNTED_TEXT PROC FAR
    push bp
    mov bp, sp
    push di
    push si
    push ds
    mov di, CURRENT_CONTEXT
    push WORD PTR [di+2ah]
    push cs
    call RESOLVE_HANDLE_TO_FAR_PAYLOAD
    xchg ax, bx
    mov ds, dx
    sub ax, ax
    mov cx, [bp+6]
    jcxz done
    cwd
    les si, DWORD PTR [bp+8]
glyph:
    lods BYTE PTR es:[si]
    cmp al, [bx+1]
    ja next_glyph
    sub al, [bx]
    jb next_glyph
    mov di, ax
    shl di, 1
    mov di, [bx+di+0ah]
    add di, bx
    add dx, [di+2]
    add dx, [bx+8]
next_glyph:
    loop glyph
    xchg ax, dx
done:
    pop ds
    pop si
    pop di
    pop bp
    retf 6
MEASURE_COUNTED_TEXT ENDP
END

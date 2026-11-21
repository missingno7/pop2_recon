.MODEL MEDIUM
.DATA
EXTRN CURRENT_CONTEXT:WORD
.CODE
EXTRN RESOLVE_HANDLE_TO_FAR_PAYLOAD:NEAR
EXTRN PAINT_FOUR_BIT:NEAR
PUBLIC DRAW_COUNTED_TEXT

DRAW_COUNTED_TEXT PROC FAR
    push bp
    mov bp, sp
    add sp, -8
    push di
    push si
    mov di, CURRENT_CONTEXT
    les si, DWORD PTR [di+22h]
    mov [bp-6], es
    push WORD PTR [di+2ah]
    push cs
    call RESOLVE_HANDLE_TO_FAR_PAYLOAD
    mov [bp-4], ax
    mov [bp-2], dx
    xchg ax, si
    mov es, dx
    sub ax, es:[si+2]
    mov [bp-8], ax
    les si, DWORD PTR [bp+8]
next_glyph:
    sub WORD PTR [bp+6], 1
    jb done
    lods BYTE PTR es:[si]
    push es
    push si
    les si, DWORD PTR [bp-4]
    cmp al, es:[si+1]
    ja advance
    sub al, es:[si]
    jb advance
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
    call PAINT_FOUR_BIT
advance:
    pop si
    pop es
    jmp SHORT next_glyph
done:
    mov ax, [di+24h]
    sub ax, [bp-6]
    pop si
    pop di
    mov sp, bp
    pop bp
    retf 6
DRAW_COUNTED_TEXT ENDP
END

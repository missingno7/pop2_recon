.MODEL MEDIUM
.CODE
PUBLIC ROUND_BYTES_TO_PARAGRAPHS

; DX:AX is an unsigned byte count. Return its rounded-up paragraph count in AX.
ROUND_BYTES_TO_PARAGRAPHS PROC NEAR
    cmp dx, 0fh
    ja saturated
    mov bx, ax
    mov cl, 4
    shr ax, cl
    ror dx, cl
    test bl, 0fh
    je add_high_word
    inc ax
add_high_word:
    add ax, dx
    jae done
saturated:
    mov ax, 0ffffh
done:
    ret
ROUND_BYTES_TO_PARAGRAPHS ENDP
END

.MODEL MEDIUM
.CODE
PUBLIC RESAMPLE_SOUND

RESAMPLE_SOUND PROC FAR
    push bp
    mov bp, sp
    add sp, -2
    push di
    push si
    push ds
    mov ax, [bp+8]
    sub dx, dx
    mov cx, [bp+6]
    div cx
    mov [bp-1], al
    sub ax, ax
    div cx
    shl al, 1
    adc ah, 0
    mov [bp-2], ah
    mov bx, 100h
    sub dx, dx
    lds si, DWORD PTR [bp+10]
    les di, DWORD PTR [bp+14]
fetch_sample:
    mov al, [si]
    mov cx, [bp-2]
    cmp bx, cx
    jbe emit_sample
accumulate:
    mul cl
    sub bx, cx
    add dx, ax
    inc si
    jmp SHORT fetch_sample
emit_sample:
    test bl, bl
    jz copy_sample
    mul bl
    add ax, dx
    xchg al, ah
    shl ah, 1
    adc al, 0
    jnc store_weighted
    mov al, 0ffh
store_weighted:
    stosb
    dec WORD PTR [bp+8]
    jz done
    mov al, [si]
    sub cx, bx
    mov bx, 100h
    sub dx, dx
    test ch, ch
    jz next_fraction
copy_sample:
    stosb
    dec WORD PTR [bp+8]
    jz done
    dec ch
    jnz copy_sample
next_fraction:
    test cl, cl
    jnz accumulate
    inc si
    jmp SHORT fetch_sample
done:
    les ax, DWORD PTR [bp+14]
    mov dx, es
    pop ds
    pop si
    pop di
    mov sp, bp
    pop bp
    retf 12
RESAMPLE_SOUND ENDP
END

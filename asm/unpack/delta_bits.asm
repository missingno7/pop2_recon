.MODEL MEDIUM
.CODE
PUBLIC UNPACK_DELTA_BITS

UNPACK_DELTA_BITS PROC FAR
    push bp
    mov bp, sp
    push di
    push si
    push ds
    push bp
    lds si, DWORD PTR [bp+6]
    les di, DWORD PTR [bp+10]
    lodsw
    add ax, di
    xchg ax, bp

    mov ah, 80h
    sub bl, bl
    lodsb
    stosb
next_sample:
    cmp di, bp
    jae done
    sub cx, cx
    add ch, bl
    jne read_bits
    mov ch, 3
read_bits:
    dec ch
    jl interpret_bits
    shl cl, 1
    test [si], ah
    jz advance_bit
    inc cl
advance_bit:
    ror ah, 1
    adc si, 0
    jmp SHORT read_bits

interpret_bits:
    test bl, bl
    jz narrow_code
    cmp cl, dl
    je reset_width
    jb store_delta
    or cl, dh
store_delta:
    add al, cl
    stosb
    jmp SHORT next_sample
    EVEN
reset_width:
    sub bl, bl
    jmp SHORT next_sample
narrow_code:
    cmp cl, 7
    jne select_width
    add al, 80h
    stosb
    jmp SHORT next_sample
select_width:
    mov bl, 2
    add bl, cl
    mov dx, 0fc02h
    shl dx, cl
    jmp SHORT next_sample

    EVEN
done:
    pop bp
    pop ds
    mov ax, [bp+10]
    mov dx, [bp+12]
    pop si
    pop di
    mov sp, bp
    pop bp
    retf 8
UNPACK_DELTA_BITS ENDP
END

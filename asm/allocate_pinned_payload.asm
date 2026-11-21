.MODEL MEDIUM
.CODE
EXTRN PREPARE_BACKING_ALLOCATION:NEAR
EXTRN ALLOCATE_BACKING_SPACE_GUARDED:NEAR
PUBLIC ALLOCATE_PINNED_PAYLOAD

ALLOCATE_PINNED_PAYLOAD PROC FAR
    push bp
    mov bp, sp
    push si
    push ds
    les ax, DWORD PTR [bp+6]
    push es
    push ax
    push cs
    call PREPARE_BACKING_ALLOCATION
    les ax, DWORD PTR [bp+6]
    mov dx, es
    call ALLOCATE_BACKING_SPACE_GUARDED
    jnc empty
    or WORD PTR [si], 8000h
    add si, 6
    xchg ax, si
    mov dx, ds
done:
    pop ds
    pop si
    pop bp
    retf 4
empty:
    sub ax, ax
    cwd
    jmp SHORT done
ALLOCATE_PINNED_PAYLOAD ENDP
END

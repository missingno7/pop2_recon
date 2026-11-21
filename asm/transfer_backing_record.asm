.MODEL MEDIUM
.DATA
EXTRN BACKING_DRIVER_SEGMENT:WORD
EXTRN BACKING_DRIVER:DWORD
.CODE
EXTRN SELECT_BACKING_POSITION:NEAR
PUBLIC TRANSFER_BACKING_RECORD

TRANSFER_BACKING_RECORD PROC FAR
    push bp
    mov bp, sp
    push si
    push ds
    mov ax, WORD PTR ss:[BACKING_DRIVER]
    or ax, ss:[BACKING_DRIVER_SEGMENT]
    jz done
    cmp WORD PTR [bp+6], 0
    jl read_data
    mov al, 5
    lds si, DWORD PTR [bp+12]
    les dx, DWORD PTR [bp+16]
    jmp SHORT seek
read_data:
    mov al, 4
    lds si, DWORD PTR [bp+16]
    les dx, DWORD PTR [bp+12]
seek:
    push ax
    mov cx, es
    call SELECT_BACKING_POSITION
    pop ax
    les dx, DWORD PTR [bp+8]
    mov cx, es
    call DWORD PTR ss:[BACKING_DRIVER]
done:
    pop ds
    pop si
    pop bp
    retf 14
TRANSFER_BACKING_RECORD ENDP
END

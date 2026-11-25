.MODEL MEDIUM
.CODE
EXTRN FIND_RESOURCE_ENTRY:NEAR
EXTRN QUERY_RESOURCE_PAYLOAD:NEAR
EXTRN READ_RESOURCE_PAYLOAD:NEAR
EXTRN H2P:NEAR
EXTRN MOVE_HUGE_BYTES:NEAR
EXTRN __nmalloc:FAR
EXTRN __nfree:FAR
PUBLIC NGETRSRC

NGETRSRC PROC FAR
    push bp
    mov bp, sp
    add sp, -4
    push di
    push si
    push ds
    mov WORD PTR [bp-2], 0
    les ax, DWORD PTR [bp+8]
    mov dx, es
    mov cx, [bp+6]
    call FIND_RESOURCE_ENTRY
    jnc load_done
    call QUERY_RESOURCE_PAYLOAD
    test cx, cx
    jne load_done
    mov [bp-4], dx
    push ax
    push ds
    mov bx, [si-2]
    push WORD PTR ss:[bx]
    push bx
    or WORD PTR ss:[bx], 8000h
    push dx
    push ss
    pop ds
    call __nmalloc
    add sp, 2
    mov [bp-2], ax
    pop bx
    pop WORD PTR ss:[bx]
    pop ds
    pop cx
    test ax, ax
    jz load_done
    jcxz load_file
    push ss
    push ax
    push cx
    push cs
    call H2P
    push dx
    push ax
    sub ax, ax
    push ax
    push WORD PTR [bp-4]
    push cs
    call MOVE_HUGE_BYTES
load_done:
    mov ax, [bp-2]
    pop ds
    pop si
    pop di
    mov sp, bp
    pop bp
    retf 6
load_file:
    mov bx, [bp-2]
    push ss
    pop es
    mov dx, [bp-4]
    sub cx, cx
    call READ_RESOURCE_PAYLOAD
    test ax, ax
    jz load_done
    jl load_file
    push ss
    pop ds
    sub ax, ax
    xchg ax, [bp-2]
    push ax
    call __nfree
    add sp, 2
    jmp SHORT load_done
NGETRSRC ENDP
END

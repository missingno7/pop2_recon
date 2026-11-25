.MODEL MEDIUM
.DATA
EXTRN HEAP_STATE:DWORD
.CODE
EXTRN HLOCK:NEAR
EXTRN GET_BLOCK_PAYLOAD_SIZE:NEAR
EXTRN PREPARE_BACKING_ALLOCATION:NEAR
EXTRN FIND_MOVABLE_PAYLOAD_SPACE:NEAR
EXTRN MOVE_HUGE_BYTES:NEAR
EXTRN HSETSTATE:NEAR
PUBLIC RELOCATE_HANDLE_PAYLOAD

RELOCATE_HANDLE_PAYLOAD PROC FAR
    push bp
    mov bp, sp
    add sp, -8
    push di
    push si
    push ds
    mov di, [bp+6]
    test di, di
    jnz inspect_payload
finish:
    xchg ax, di
    pop ds
    pop si
    pop di
    mov sp, bp
    pop bp
    retf 2
inspect_payload:
    push di
    push WORD PTR ss:[di]
    push di
    push cs
    call HLOCK
    xchg ax, si
    mov ds, dx
    or dx, si
    jz restore
    sub si, 6
    mov [bp-4], si
    mov [bp-2], ds
    call GET_BLOCK_PAYLOAD_SIZE
    mov [bp-8], ax
    mov [bp-6], dx
    push dx
    push ax
    push cs
    call PREPARE_BACKING_ALLOCATION
    cmp dx, [bp-6]
    ja check_position
    jb restore
    cmp ax, [bp-8]
    jb restore
check_position:
    lds si, DWORD PTR ss:[HEAP_STATE]
    lds si, DWORD PTR [si+4]
    mov dx, ds
    cmp dx, [bp-2]
    ja restore
    jb allocate_payload
    cmp si, [bp-4]
    ja restore
allocate_payload:
    les ax, DWORD PTR [bp-8]
    mov dx, es
    call FIND_MOVABLE_PAYLOAD_SPACE
    jnc restore
    les bx, DWORD PTR [bp-4]
    mov cx, es:[bx]
    and WORD PTR es:[bx], 3fffh
    and cx, 0ffc0h
    and WORD PTR [si], 3fh
    or [si], cx
    mov [si+4], di
    and WORD PTR ss:[di], 0fff0h
    or ss:[di], si
    mov ss:[di+2], ds
    add si, 6
    push ds
    push si
    add bx, 6
    push es
    push bx
    push dx
    push ax
    push cs
    call MOVE_HUGE_BYTES
restore:
    push cs
    call HSETSTATE
    jmp NEAR PTR finish
RELOCATE_HANDLE_PAYLOAD ENDP
END

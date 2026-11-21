.MODEL MEDIUM
.DATA
handle_scratch_source DW 0
handle_scratch_saved DW 0
.CODE
EXTRN MATERIALIZE_HANDLE:NEAR
EXTRN FIND_MOVABLE_PAYLOAD_SPACE:NEAR
EXTRN GET_BLOCK_PAYLOAD_SIZE:NEAR
EXTRN MOVE_HUGE_BYTES:NEAR
PUBLIC RESIZE_HANDLE
EXTRN ALLOCATE_BACKING_SPACE_GUARDED:NEAR
EXTRN RELEASE_HANDLE_STORAGE:NEAR
EXTRN FREE_NEAR_HEAP:FAR
EXTRN ALLOCATE_NEAR_HEAP:FAR
PUBLIC ALLOCATE_PAYLOAD_HANDLE, ALLOCATE_EMPTY_HANDLE

ALLOCATE_PAYLOAD_HANDLE PROC FAR
    push bp
    mov bp, sp
    push si
    push ds
    call ALLOCATE_EMPTY_HANDLE
    les bp, DWORD PTR [bp+6]
    xchg ax, bp
    jnc alloc_done
    mov dx, es
    call ALLOCATE_BACKING_SPACE_GUARDED
    jnc alloc_failed
    or WORD PTR [si], 0c000h
    mov [si+4], bp
    or si, 01f0h
    mov [bp], si
    mov [bp+2], ds
alloc_done:
    xchg ax, bp
    pop ds
    pop si
    pop bp
    retf 4
alloc_failed:
    push ss
    pop ds
    push bp
    call FREE_NEAR_HEAP
    add sp, 2
    sub bp, bp
    jmp SHORT alloc_done
ALLOCATE_PAYLOAD_HANDLE ENDP

ALLOCATE_EMPTY_HANDLE PROC NEAR
    push ds
    push ss
    pop ds
    mov ax, handle_scratch_source
    mov handle_scratch_saved, ax
    mov ax, 4
    push ax
    call ALLOCATE_NEAR_HEAP
    add sp, 2
    sub dx, dx
    cmp dx, ax
    je empty_done
    mov bx, ax
    mov [bx], dx
    mov [bx+2], dx
empty_done:
    pop ds
    ret
ALLOCATE_EMPTY_HANDLE ENDP

PUBLIC MARK_FIXED, MARK_PURGEABLE

MARK_FIXED PROC FAR
    push bp
    mov bp, sp
    mov bp, [bp+6]
    test bp, bp
    jz fixed_done
    and WORD PTR [bp], 0bfffh
fixed_done:
    xchg ax, bp
    pop bp
    retf 2
MARK_FIXED ENDP

MARK_PURGEABLE PROC FAR
    push bp
    mov bp, sp
    mov bp, [bp+6]
    test bp, bp
    jz purgeable_done
    or WORD PTR [bp], 4000h
purgeable_done:
    xchg ax, bp
    pop bp
    retf 2
MARK_PURGEABLE ENDP

PUBLIC REPLACE_HANDLE_PAYLOAD

REPLACE_HANDLE_PAYLOAD PROC FAR
    push bp
    mov bp, sp
    push di
    push si
    push ds
    mov di, [bp+10]
    test di, di
    jz replace_status
    push di
    push cs
    call RELEASE_HANDLE_STORAGE
    test ax, ax
    jnz replace_done
    les ax, DWORD PTR [bp+6]
    mov dx, es
    call ALLOCATE_BACKING_SPACE_GUARDED
    jnc replace_status
    or WORD PTR [si], 0c000h
    mov [si+4], di
    and WORD PTR ss:[di], 0fdf0h
    or si, 01f0h
    or ss:[di], si
    mov ss:[di+2], ds
    stc
replace_status:
    cmc
    sbb ax, ax
    neg ax
replace_done:
    pop ds
    pop si
    pop di
    pop bp
    retf 6
REPLACE_HANDLE_PAYLOAD ENDP

RESIZE_HANDLE PROC FAR
    push bp
    mov bp, sp
    push di
    push si
    push ds
    mov di, [bp+10]
    push di
    push cs
    call MATERIALIZE_HANDLE
    or ax, dx
    jz resize_missing
    lds si, DWORD PTR ss:[di]
    and si, 0fh
    les ax, DWORD PTR [bp+6]
    mov dx, es
    call FIND_MOVABLE_PAYLOAD_SPACE
    jc resize_status
    test WORD PTR ss:[di], 8000h
    jnz resize_status
    or WORD PTR ss:[di], 8000h
    push ds
    push si
    les ax, DWORD PTR [bp+6]
    mov dx, es
    call ALLOCATE_BACKING_SPACE_GUARDED
    sbb cx, cx
    mov bx, si
    push ds
    pop es
    pop si
    pop ds
    and WORD PTR ss:[di], 7fffh
    jcxz resize_status
    mov ax, [si]
    and WORD PTR [si], 3fffh
    and ax, 0ffc0h
    and WORD PTR es:[bx], 3fh
    or es:[bx], ax
    mov es:[bx+4], di
    and WORD PTR ss:[di], 0fff0h
    or ss:[di], bx
    mov ss:[di+2], es
    add bx, 6
    push es
    push bx
    call GET_BLOCK_PAYLOAD_SIZE
    add si, 6
    push ds
    push si
    push dx
    push ax
    push cs
    call MOVE_HUGE_BYTES
    stc
resize_status:
    cmc
    sbb ax, ax
    neg ax
resize_done:
    pop ds
    pop si
    pop di
    pop bp
    retf 6
resize_missing:
    push di
    les ax, DWORD PTR [bp+6]
    push es
    push ax
    push cs
    call NEAR PTR REPLACE_HANDLE_PAYLOAD
    jmp SHORT resize_done
RESIZE_HANDLE ENDP


PUBLIC RESTORE_HANDLE_FLAGS

RESTORE_HANDLE_FLAGS PROC FAR
    push bp
    mov bp, sp
    mov ax, [bp+6]
    mov bp, [bp+8]
    test bp, bp
    jz done
    and ax, 0e000h
    and WORD PTR [bp], 1fffh
    or [bp], ax
done:
    xchg ax, bp
    pop bp
    retf 4
RESTORE_HANDLE_FLAGS ENDP
END

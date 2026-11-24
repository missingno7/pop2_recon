GAMELIB_TEXT SEGMENT BYTE PUBLIC 'CODE'
GAMELIB_TEXT ENDS

DGROUP GROUP _DATA
_DATA SEGMENT WORD PUBLIC 'DATA'
ASSUME DS:DGROUP
EXTRN BACKING_DRIVER:DWORD
EXTRN BACKING_POSITION:DWORD
EXTRN BACKING_FREE:DWORD
EXTRN BACKING_FULL:WORD
PUBLIC ALLOCATION_HANDLER
ALLOCATION_HANDLER DD 0
_DATA ENDS

GAMELIB_TEXT SEGMENT BYTE PUBLIC 'CODE'
ASSUME CS:GAMELIB_TEXT, DS:DGROUP, SS:DGROUP
PUBLIC ALLOCATE_BACKING_RECORD, READ_BACKING_HEADER, WRITE_BACKING_HEADER
PUBLIC SELECT_BACKING_POSITION, SELECT_BACKING_EXTENDED

ALLOCATE_BACKING_RECORD PROC FAR
    push bp
    mov bp, sp
    add sp, -2
    push di
    push si
    push ds
    push ss
    pop ds
    mov ax, WORD PTR BACKING_DRIVER
    or ax, WORD PTR BACKING_DRIVER+2
    jz allocation_failed
    cmp WORD PTR [bp+8], 0fe00h
    jbe size_ready
    mov WORD PTR [bp+6], 0ffffh
    mov WORD PTR [bp+8], 1ffh
size_ready:
    mov WORD PTR [bp-2], 2
seek_next_candidate:
    les dx, BACKING_POSITION
    mov cx, es
    mov ax, 3
seek_candidate:
    call BACKING_DRIVER
    call READ_BACKING_HEADER
    test ax, 0c000h
    jz free_record
    js skip_record
    jmp extend_backing
skip_record:
    and ax, 1ffh
    xchg cx, ax
    mov ax, 103h
    jmp SHORT seek_candidate
allocation_failed:
    sub ax, ax
    cwd
    jmp allocation_done
free_record:
    and ax, 1ffh
    mov di, ax
    mov si, dx
    xchg cx, ax
    call SELECT_BACKING_EXTENDED
    call READ_BACKING_HEADER
    push ax
    push dx
    mov dx, -4
    mov cx, -1
    sub dx, si
    sbb cx, di
    call SELECT_BACKING_EXTENDED
    pop dx
    pop ax
    test ax, 0c000h
    xchg di, ax
    xchg dx, si
    jne test_free_size
    add si, dx
    adc di, ax
    mov dx, -4
    mov cx, -1
    sub si, dx
    sbb di, cx
    call SELECT_BACKING_EXTENDED
    xchg di, ax
    mov dx, si
    call WRITE_BACKING_HEADER
test_free_size:
    cmp ax, [bp+8]
    ja split_free_record
    jb skip_record
    cmp dx, [bp+6]
    jb skip_record
split_free_record:
    mov di, ax
    mov si, dx
    sub dx, [bp+6]
    sbb ax, [bp+8]
    sub dx, 4
    jnc write_remainder
    sbb ax, 0
    jc mark_allocated
write_remainder:
    push ax
    push dx
    les dx, DWORD PTR [bp+6]
    mov cx, es
    call SELECT_BACKING_EXTENDED
    pop dx
    pop ax
    call WRITE_BACKING_HEADER
    sub si, dx
    sbb di, ax
    mov dx, -4
    mov cx, -1
    add si, dx
    adc di, cx
    sub dx, si
    sbb cx, di
    call SELECT_BACKING_EXTENDED
mark_allocated:
    mov dx, -4
    mov cx, -1
    call SELECT_BACKING_EXTENDED
    mov WORD PTR BACKING_POSITION, dx
    mov WORD PTR BACKING_POSITION+2, cx
    mov dx, si
    mov ax, di
    sub si, [bp+6]
    mov cl, 9
    shl si, cl
    and si, 600h
    or ax, si
    or ax, 8000h
    call WRITE_BACKING_HEADER
    and ax, 1ffh
    add dx, 4
    adc ax, 0
    sub WORD PTR BACKING_FREE, dx
    sbb WORD PTR BACKING_FREE+2, ax
    les ax, BACKING_POSITION
    mov dx, es
    add ax, 4
    adc dx, 0
allocation_done:
    pop ds
    pop si
    pop di
    mov sp, bp
    pop bp
    retf 4
extend_backing:
    cmp BACKING_FULL, 0
    jne restart_search
    sub cx, cx
    sub dx, dx
    call SELECT_BACKING_EXTENDED
    add dx, [bp+6]
    adc cx, [bp+8]
    cmp cx, 200h
    ja mark_full
    jb write_end_marker
    cmp dx, 4
    jae mark_full
write_end_marker:
    call SELECT_BACKING_POSITION
    jc mark_full
    mov ax, 4000h
    sub dx, dx
    call WRITE_BACKING_HEADER
    jc mark_full
    mov dx, -8
    mov cx, -1
    sub dx, [bp+6]
    sbb cx, [bp+8]
    call SELECT_BACKING_EXTENDED
    mov WORD PTR BACKING_POSITION, dx
    mov WORD PTR BACKING_POSITION+2, cx
    les dx, DWORD PTR [bp+6]
    mov ax, es
    call WRITE_BACKING_HEADER
    add dx, 4
    adc ax, 0
    add WORD PTR BACKING_FREE, dx
    adc WORD PTR BACKING_FREE+2, ax
    jmp seek_next_candidate
give_up:
    jmp allocation_failed
mark_full:
    mov BACKING_FULL, 1
restart_search:
    dec WORD PTR [bp-2]
    jz give_up
    les ax, DWORD PTR [bp+6]
    mov dx, es
    cmp dx, WORD PTR BACKING_FREE+2
    ja give_up
    jb seek_from_start
    cmp ax, WORD PTR BACKING_FREE
    ja give_up
seek_from_start:
    sub ax, ax
    mov WORD PTR BACKING_POSITION, ax
    mov WORD PTR BACKING_POSITION+2, ax
    jmp seek_next_candidate
ALLOCATE_BACKING_RECORD ENDP

READ_BACKING_HEADER PROC NEAR
    push si
    push ds
    push ss
    pop ds
    sub cx, cx
    mov dx, 4
    sub sp, dx
    mov si, sp
    mov al, 4
    call DWORD PTR ss:[BACKING_DRIVER]
    pop dx
    pop ax
    pop ds
    pop si
    ret
READ_BACKING_HEADER ENDP

WRITE_BACKING_HEADER PROC NEAR
    push si
    push ds
    push ss
    pop ds
    push ax
    push dx
    sub cx, cx
    mov dx, 4
    mov si, sp
    mov al, 5
    call DWORD PTR ss:[BACKING_DRIVER]
    pop dx
    pop ax
    pop ds
    pop si
    ret
WRITE_BACKING_HEADER ENDP

SELECT_BACKING_POSITION PROC NEAR
    mov ax, 3
    call DWORD PTR ss:[BACKING_DRIVER]
    ret
SELECT_BACKING_POSITION ENDP

SELECT_BACKING_EXTENDED PROC NEAR
    mov ax, 103h
    call DWORD PTR ss:[BACKING_DRIVER]
    ret
SELECT_BACKING_EXTENDED ENDP
GAMELIB_TEXT ENDS
END

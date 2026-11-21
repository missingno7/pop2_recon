.MODEL MEDIUM
.DATA
EXTRN BACKING_DRIVER:DWORD
.CODE
PUBLIC READ_BACKING_HEADER, WRITE_BACKING_HEADER
PUBLIC SELECT_BACKING_POSITION, SELECT_BACKING_EXTENDED

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
END

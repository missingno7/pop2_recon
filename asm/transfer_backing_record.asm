.MODEL MEDIUM
.DATA
EXTRN BACKING_DRIVER_SEGMENT:WORD
EXTRN BACKING_DRIVER:DWORD
.CODE
EXTRN SELECT_BACKING_POSITION:NEAR
PUBLIC TRANSFER_BACKING_RECORD


EXTRN READ_BACKING_HEADER:NEAR
EXTRN WRITE_BACKING_HEADER:NEAR
PUBLIC INITIALIZE_BACKING_DRIVER
INITIALIZE_BACKING_DRIVER PROC FAR
    push bp
    mov bp, sp
    add sp, -8
    push di
    sub ax, ax
    mov [bp-8], ax
    mov [bp-6], ax
    mov cx, 7
    push ds
    pop es
    mov di, OFFSET BACKING_DRIVER
    rep stosw
    les ax, DWORD PTR [bp+6]
    mov dx, es
    or dx, ax
    jz backing_ready
    mov al, 1
    call DWORD PTR [bp+6]
    mov [bp-8], ax
    mov [bp-6], dx
    jc backing_ready
    les ax, DWORD PTR [bp+6]
    mov WORD PTR BACKING_DRIVER, ax
    mov BACKING_DRIVER_SEGMENT, es
    mov ax, 4000h
    sub dx, dx
    call WRITE_BACKING_HEADER
    jc close_failed_driver
    sub cx, cx
    sub dx, dx
    call SELECT_BACKING_POSITION
    jc close_failed_driver
    call READ_BACKING_HEADER
    jc close_failed_driver
    cmp ax, 4000h
    jne close_failed_driver
    test dx, dx
    jz backing_ready
close_failed_driver:
    mov al, 2
    call BACKING_DRIVER
    sub ax, ax
    mov WORD PTR BACKING_DRIVER, ax
    mov BACKING_DRIVER_SEGMENT, ax
    mov [bp-8], ax
    mov [bp-6], ax
backing_ready:
    les ax, DWORD PTR [bp-8]
    mov dx, es
    pop di
    mov sp, bp
    pop bp
    retf 4
INITIALIZE_BACKING_DRIVER ENDP

PUBLIC SHUTDOWN_BACKING_DRIVER
SHUTDOWN_BACKING_DRIVER PROC FAR
    mov ax, WORD PTR BACKING_DRIVER
    or ax, BACKING_DRIVER_SEGMENT
    jz driver_inactive
    mov al, 2
    call BACKING_DRIVER
    sub ax, ax
    mov WORD PTR BACKING_DRIVER, ax
    mov BACKING_DRIVER_SEGMENT, ax
driver_inactive:
    retf
SHUTDOWN_BACKING_DRIVER ENDP

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

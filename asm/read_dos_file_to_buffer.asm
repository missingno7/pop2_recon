.MODEL MEDIUM
EXTRN DOS_DGROUP:BYTE
.DATA
file_buffer DD 0
file_handle DW 0
file_length DW 0
.CODE
EXTRN ALLOCATE_PINNED_PAYLOAD:NEAR
EXTRN CLEAR_PAYLOAD_HEADER:NEAR
PUBLIC READ_DOS_FILE_TO_BUFFER, RELEASE_DRIVER_PAYLOAD

READ_DOS_FILE_TO_BUFFER PROC FAR
    push bp
    mov bp, sp
    push ds
    mov ax, SEG DOS_DGROUP
    mov ds, ax
    xor ax, ax
    mov WORD PTR file_buffer, ax
    mov WORD PTR [file_buffer+2], ax
    lds dx, DWORD PTR [bp+6]
    mov ax, 3d00h
    int 21h
    mov file_handle, ax
    mov bx, file_handle
    mov ax, 4202h
    xor cx, cx
    xor dx, dx
    int 21h
    mov file_length, ax
    mov bx, file_handle
    mov ax, 4200h
    xor cx, cx
    xor dx, dx
    int 21h
    xor dx, dx
    mov ax, file_length
    or ax, ax
    jz save_file_buffer
    push dx
    push ax
    push cs
    call ALLOCATE_PINNED_PAYLOAD
save_file_buffer:
    mov WORD PTR file_buffer, ax
    mov WORD PTR [file_buffer+2], dx
    mov cx, ax
    or cx, dx
    jz close_file
    mov cx, file_length
    mov bx, file_handle
    push ds
    mov ds, dx
    mov dx, ax
    mov ah, 3fh
    int 21h
    pop ds
close_file:
    mov ax, SEG DOS_DGROUP
    mov ds, ax
    mov ah, 3eh
    mov bx, file_handle
    int 21h
    mov ax, WORD PTR file_buffer
    mov dx, WORD PTR [file_buffer+2]
    pop ds
    pop bp
    retf
READ_DOS_FILE_TO_BUFFER ENDP

RELEASE_DRIVER_PAYLOAD PROC FAR
    push bp
    mov bp, sp
    push es
    les bx, DWORD PTR [bp+6]
    push es
    push bx
    push cs
    call CLEAR_PAYLOAD_HEADER
    pop es
    pop bp
    retf
RELEASE_DRIVER_PAYLOAD ENDP
END

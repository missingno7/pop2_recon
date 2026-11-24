.MODEL MEDIUM
EXTRN DOS_DGROUP:BYTE
.DATA
; Unused driver entry points, still the failing stub.
    DD INVALID_DRIVER_STUB, INVALID_DRIVER_STUB
driver_allocation DD 0
; The file reader shares the entry, handle and low header words.
driver_entry DD 0
driver_file DW 0
driver_header DD 0
.CODE
EXTRN ALLOCATE_PINNED_PAYLOAD:NEAR
EXTRN CLEAR_PAYLOAD_HEADER:NEAR
EXTRN CPU_PROBE:FAR
PUBLIC INVALID_DRIVER_STUB, _load_sound_driver

INVALID_DRIVER_STUB PROC FAR
    mov ax, -1
    retf
INVALID_DRIVER_STUB ENDP

_load_sound_driver PROC FAR
    push bp
    mov bp, sp
    push ds
    mov ax, SEG DOS_DGROUP
    mov ds, ax
    xor ax, ax
    mov WORD PTR driver_allocation, ax
    mov WORD PTR [driver_allocation+2], ax
    mov WORD PTR driver_header, ax
    mov WORD PTR [driver_header+2], ax
    lds dx, DWORD PTR [bp+6]
    mov ax, 3d00h
    int 21h
    mov driver_file, ax
    jnc get_driver_header
    xor ax, ax
    mov driver_file, ax
    jmp invalid_driver
get_driver_header:
    xor dx, dx
    mov ax, 0c4h
    or ax, ax
    jz store_driver_header
    push dx
    push ax
    push cs
    call ALLOCATE_PINNED_PAYLOAD
store_driver_header:
    mov WORD PTR driver_header, ax
    mov WORD PTR [driver_header+2], dx
    mov cx, ax
    or cx, dx
    jne read_header
header_failed:
    jmp invalid_driver
read_header:
    mov cx, 0c4h
    mov bx, driver_file
    push ds
    mov ds, dx
    mov dx, ax
    mov ah, 3fh
    int 21h
    pop ds
    jc header_failed
    les bx, driver_header
    mov ax, es:[bx+90h]
    cmp ax, 9876h
    jne header_failed
    mov ax, es:[bx+92h]
    cmp ax, 1234h
    jne header_failed
    call CPU_PROBE
    cmp ax, es:[bx+96h]
    jl invalid_driver
    push bx
    mov bx, driver_file
    mov ax, 4200h
    xor cx, cx
    mov dx, 100h
    int 21h
    pop bx
    jc invalid_driver
    push es
    push bx
    xor dx, dx
    mov ax, es:[bx+9ah]
    or ax, ax
    jz store_driver_allocation
    add ax, 20h
    push dx
    push ax
    push cs
    call ALLOCATE_PINNED_PAYLOAD
store_driver_allocation:
    pop bx
    pop es
    mov WORD PTR driver_allocation, ax
    mov WORD PTR [driver_allocation+2], dx
    mov cx, ax
    or cx, dx
    jz invalid_driver
    mov cl, 4
    shr ax, cl
    add dx, ax
    mov ax, 100h
    sub dx, 0eh
    mov WORD PTR driver_entry, ax
    mov WORD PTR [driver_entry+2], dx
    push es
    push bx
    mov es, dx
    mov bx, ax
    mov ax, WORD PTR driver_allocation
    mov es:[bx-4], ax
    mov ax, WORD PTR [driver_allocation+2]
    mov es:[bx-2], ax
    mov ax, es
    mov dx, bx
    pop bx
    pop es
    mov cx, es:[bx+9ah]
    mov bx, driver_file
    mov ds, ax
    mov ah, 3fh
    int 21h
    jnc close_driver_file
invalid_driver:
    mov ax, SEG DOS_DGROUP
    mov ds, ax
    mov ax, OFFSET INVALID_DRIVER_STUB
    mov WORD PTR driver_entry, ax
    mov WORD PTR [driver_entry+2], cs
close_driver_file:
    mov ax, SEG DOS_DGROUP
    mov ds, ax
    mov bx, driver_file
    or bx, bx
    jz free_driver_header
    mov ah, 3eh
    int 21h
free_driver_header:
    mov ax, WORD PTR driver_header
    or ax, WORD PTR [driver_header+2]
    jz return_driver_entry
    mov ax, WORD PTR [driver_header+2]
    push ax
    mov ax, WORD PTR driver_header
    push ax
    push cs
    call CLEAR_PAYLOAD_HEADER
return_driver_entry:
    mov ax, WORD PTR driver_entry
    mov dx, WORD PTR [driver_entry+2]
    pop ds
    pop bp
    retf
_load_sound_driver ENDP

PUBLIC RELEASE_LOADED_DRIVER

; Only an entry at PSP offset 100h has a preceding allocation pointer.
RELEASE_LOADED_DRIVER PROC FAR
    push bp
    mov bp, sp
    push ds
    lds bx, DWORD PTR [bp+6]
    cmp bx, 100h
    jne driver_release_done
    lds bx, DWORD PTR [bx-4]
    cmp bx, -1
    je driver_release_done
    push ds
    push bx
    push cs
    call CLEAR_PAYLOAD_HEADER
driver_release_done:
    pop ds
    pop bp
    retf
RELEASE_LOADED_DRIVER ENDP

PUBLIC _read_dos_file_to_buffer, _release_driver_payload

_read_dos_file_to_buffer PROC FAR
    push bp
    mov bp, sp
    push ds
    mov ax, SEG DOS_DGROUP
    mov ds, ax
    xor ax, ax
    mov WORD PTR driver_entry, ax
    mov WORD PTR [driver_entry+2], ax
    lds dx, DWORD PTR [bp+6]
    mov ax, 3d00h
    int 21h
    mov driver_file, ax
    mov bx, driver_file
    mov ax, 4202h
    xor cx, cx
    xor dx, dx
    int 21h
    mov driver_header, ax
    mov bx, driver_file
    mov ax, 4200h
    xor cx, cx
    xor dx, dx
    int 21h
    xor dx, dx
    mov ax, driver_header
    or ax, ax
    jz save_file_buffer
    push dx
    push ax
    push cs
    call ALLOCATE_PINNED_PAYLOAD
save_file_buffer:
    mov WORD PTR driver_entry, ax
    mov WORD PTR [driver_entry+2], dx
    mov cx, ax
    or cx, dx
    jz close_file
    mov cx, driver_header
    mov bx, driver_file
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
    mov bx, driver_file
    int 21h
    mov ax, WORD PTR driver_entry
    mov dx, WORD PTR [driver_entry+2]
    pop ds
    pop bp
    retf
_read_dos_file_to_buffer ENDP

_release_driver_payload PROC FAR
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
_release_driver_payload ENDP
END

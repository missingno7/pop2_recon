; Shared packed-pixel routines and adapter setup for two DOS video modes.
; The CGA and Hercules descriptors use the same raster operations.
.MODEL MEDIUM
.DATA
EXTRN DEFAULT_PORT:BYTE
EXTRN mode_bytes_per_row:WORD
EXTRN mode_scanline_offsets:WORD

PUBLIC screen_bits, Graf, screen_row_origin, screen_x_origin
screen_bits DD packed_pattern_bank
Graf DW 0
screen_row_origin DW 0
screen_x_origin DW 0
.CODE
EXTRN check_cursor_overlap:NEAR
EXTRN show_mouse_graphics:NEAR
EXTRN __nmalloc:FAR
hercules_config_port EQU 3BFh
mono_mode_port EQU 3B8h
mono_crtc_port EQU 3B4h
mono_status_port EQU 3BAh
color_crtc_port EQU 3D4h

PUBLIC cga_mode_descriptor
PUBLIC hercules_mode_descriptor
PUBLIC packed_pattern_bank
PUBLIC copy_packed_rectangle
PUBLIC copy_rop_forward
PUBLIC probe_cga
PUBLIC initialize_cga
PUBLIC cga_surface_geometry
PUBLIC draw_packed_line
PUBLIC scale_line_clip_delta
PUBLIC classify_line_endpoint
PUBLIC line_rop_dispatch
PUBLIC draw_packed_bitmap
PUBLIC bitmap_rop_dispatch
PUBLIC cga_mode_exit
PUBLIC fill_packed_rectangle
PUBLIC fill_rop_dispatch
PUBLIC composite_packed_bitmap
PUBLIC transparent_pair_masks
PUBLIC probe_hercules
PUBLIC initialize_hercules
PUBLIC hercules_surface_geometry
PUBLIC hercules_mode_exit
PUBLIC probe_crtc_register
PUBLIC probe_video_ram
PUBLIC mode_boundary_fill

; Mode, width, color count, format, lifecycle hooks and drawing slots.
cga_mode_descriptor         DW 1, 140h, 4, 104h
                            DW OFFSET probe_cga, OFFSET initialize_cga, OFFSET cga_mode_exit, 0
                            DW 0, 0, OFFSET copy_packed_rectangle, OFFSET draw_packed_line
                            DW OFFSET draw_packed_bitmap, OFFSET fill_packed_rectangle, OFFSET composite_packed_bitmap

; Mode, width, color count, format, lifecycle hooks and drawing slots.
hercules_mode_descriptor    DW 2, 140h, 4, 104h
                            DW OFFSET probe_hercules, OFFSET initialize_hercules, OFFSET hercules_mode_exit, 0
                            DW 0, 0, OFFSET copy_packed_rectangle, OFFSET draw_packed_line
                            DW OFFSET draw_packed_bitmap, OFFSET fill_packed_rectangle, OFFSET composite_packed_bitmap

; Reserved words followed by the shared packed-pixel patterns.
packed_pattern_bank DW 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
                            DW 0, 0CC33h, 0CC33h, 0C03h, 0C030h, 5555h, 5555h, 0FFFFh
                            DW 0FFFFh, 5555h, 5555h, 0AAAAh, 0AAAAh, 0, 0, 0FFFFh
                            DW 0FFFFh, 0FFFFh, 0FFFFh

; Clip the source and destination together, then choose the copy direction
; and raster operation. Both scanline tables contain offsets into packed memory.
copy_packed_rectangle PROC FAR
    push bp
    mov bp, sp
    add sp,-2Ch
    push di
    push si
    mov bx, WORD PTR [bp+10h]
    les si, DWORD PTR [bp+8]
    sub dx, dx
    lods WORD PTR es:[si]
    sub ax, WORD PTR [bx+4]
    jge loc_3a8a
    xchg dx, ax

loc_3a8a:
    mov WORD PTR [bp-1Eh], ax
    sub cx, cx
    lods WORD PTR es:[si]
    sub ax, WORD PTR [bx+6]
    jge loc_3a97
    xchg cx, ax

loc_3a97:
    mov WORD PTR [bp-1Ch], ax
    sub di, di
    lods WORD PTR es:[si]
    sub ax, WORD PTR [bx+8]
    jle loc_3aa4
    xchg di, ax

loc_3aa4:
    mov WORD PTR [bp-1Ah], di
    sub di, di
    lods WORD PTR es:[si]
    sub ax, WORD PTR [bx+0Ah]
    jle loc_3ab1
    xchg di, ax

loc_3ab1:
    mov WORD PTR [bp-18h], di
    mov bx, WORD PTR [bp+12h]
    les si, DWORD PTR [bp+0Ch]
    lods WORD PTR es:[si]
    sub ax, dx
    sub ax, WORD PTR [bx+4]
    jge loc_3ac8
    sub WORD PTR [bp-1Eh], ax
    sub ax, ax

loc_3ac8:
    xchg dx, ax
    lods WORD PTR es:[si]
    sub ax, cx
    sub ax, WORD PTR [bx+6]
    jge loc_3ad7
    sub WORD PTR [bp-1Ch], ax
    sub ax, ax

loc_3ad7:
    xchg cx, ax
    lods WORD PTR es:[si]
    sub ax, WORD PTR [bp-1Ah]
    cmp ax, WORD PTR [bx+8]
    jle loc_3ae5
    mov ax, WORD PTR [bx+8]

loc_3ae5:
    sub ax, WORD PTR [bx+4]
    mov WORD PTR [bp-12h], ax
    lods WORD PTR es:[si]
    sub ax, WORD PTR [bp-18h]
    cmp ax, WORD PTR [bx+0Ah]
    jle loc_3af8
    mov ax, WORD PTR [bx+0Ah]

loc_3af8:
    sub ax, WORD PTR [bx+6]
    mov WORD PTR [bp-10h], ax
    cmp bx, WORD PTR [Graf]
    jne loc_3b32
    mov ax, WORD PTR [bx+18h]
    sub ax, dx
    jle loc_3b10
    add dx, ax
    add WORD PTR [bp-1Eh], ax

loc_3b10:
    mov ax, WORD PTR [bx+1Ah]
    sub ax, cx
    jle loc_3b1c
    add cx, ax
    add WORD PTR [bp-1Ch], ax

loc_3b1c:
    mov ax, WORD PTR [bx+1Ch]
    sub ax, WORD PTR [bp-12h]
    jge loc_3b27
    add WORD PTR [bp-12h], ax

loc_3b27:
    mov ax, WORD PTR [bx+1Eh]
    sub ax, WORD PTR [bp-10h]
    jge loc_3b32
    add WORD PTR [bp-10h], ax

loc_3b32:
    mov WORD PTR [bp-14h], cx
    mov WORD PTR [bp-16h], dx
    mov ax, WORD PTR [bp-12h]
    sub ax, dx
    mov WORD PTR [bp-28h], ax
    jle loc_3b69
    sub cx, WORD PTR [bp-1Ch]
    mov dx, 3
    and cl, dl
    mov ch, cl
    neg cl
    and cl, dl
    jne loc_3b54
    mov cl, 4

loc_3b54:
    shl cx, 1
    mov WORD PTR [bp-24h], cx
    mov ax, 0FFFFh
    mov di, WORD PTR [bp-10h]
    mov cx, WORD PTR [bp-14h]
    sub di, cx
    mov WORD PTR [bp-26h], di
    jg loc_3b71

loc_3b69:
    pop si
    pop di
    mov sp, bp
    pop bp
    retf 0Eh

loc_3b71:
    and cl, dl
    jne loc_3b78
    sub di, 4

loc_3b78:
    shr al, cl
    shr al, cl
    neg cx
    and cx, dx
    sub di, cx
    jle loc_3b97
    mov cx, di
    shr di, 1
    shr di, 1
    neg cx
    and cl, dl
    je loc_3ba5
    shl cl, 1
    shl ah, cl
    inc di
    jmp SHORT loc_3ba5

loc_3b97:
    mov cx, di
    neg cx
    and cx, dx
    shl cl, 1
    shl ah, cl
    and al, ah
    sub di, di

loc_3ba5:
    mov WORD PTR [bp-0Ah], di
    mov WORD PTR [bp-20h], ax
    les ax, DWORD PTR [bp-28h]
    add ax, WORD PTR [bp-1Eh]
    mov WORD PTR [bp-1Ah], ax
    mov ax, es
    add ax, WORD PTR [bp-1Ch]
    mov WORD PTR [bp-18h], ax
    mov si, bp
    les ax, DWORD PTR [bx]
    lea bp, [si-0Eh]
    call check_cursor_overlap
    mov di, WORD PTR [si+10h]
    les ax, DWORD PTR [di]
    lea bp, [si-16h]
    call check_cursor_overlap
    mov bp, si
    mov WORD PTR [bp-22h], OFFSET copy_rop_forward
    mov WORD PTR [bp-2Ch], 1
    mov dx, WORD PTR [bp-1Ch]
    mov di, dx
    cmp ax, WORD PTR [bx]
    mov ax, WORD PTR [bp-14h]
    jne loc_3c2f
    mov cx, es
    cmp cx, WORD PTR [bx+2]
    jne loc_3c2f
    mov si, WORD PTR [bp-1Eh]
    cmp si, WORD PTR [bp-16h]
    jge loc_3c05
    mov cx, WORD PTR [bp-28h]
    dec cx
    add WORD PTR [bp-16h], cx
    add WORD PTR [bp-1Eh], cx
    neg WORD PTR [bp-2Ch]

loc_3c05:
    cmp dx, ax
    jg loc_3c2f
    mov cx, WORD PTR [bp-26h]
    dec cx
    add ax, cx
    add dx, cx
    mov di, dx
    neg di
    std
    mov cx, WORD PTR [bp-24h]
    test ch, ch
    je loc_3c22
    xchg cl, ch
    mov WORD PTR [bp-24h], cx

loc_3c22:
    mov cx, WORD PTR [bp-20h]
    xchg cl, ch
    mov WORD PTR [bp-20h], cx
    mov WORD PTR [bp-22h], OFFSET copy_rop_reverse

loc_3c2f:
    and di, 3
    shl di, 1
    mov WORD PTR [bp-0Ch], di
    mov cl, 2
    shr ax, cl
    shr dx, cl
    les cx, DWORD PTR [bx]
    add ax, cx
    mov WORD PTR [bp-14h], ax
    mov ax, WORD PTR [bx+0Eh]
    mov WORD PTR [bp-0Eh], ax
    mov bx, WORD PTR [bp+10h]
    mov ax, WORD PTR [bx+0Eh]
    mov WORD PTR [bp-2Ah], ax
    lds cx, DWORD PTR [bx]
    add dx, cx
    mov WORD PTR [bp-1Ch], dx
    mov cx, WORD PTR [bp-24h]
    mov bx, WORD PTR [bp+6]
    and bx, 7
    mov dl, BYTE PTR cs:copy_rop_masks[bx]
    and bx, 3
    shl bx, 1
    add bx, WORD PTR [bp-22h]
    mov ax, WORD PTR cs:[bx]
    mov WORD PTR [bp-22h], ax
    jmp SHORT copy_prepare_scanline

copy_forward_replace:
    xchg al, ah
    rol ax, cl
    and al, dh
    not dh
    and dh, BYTE PTR es:[di]
    or al, dh
    stosb
    xchg cl, ch
    rol ah, cl
    xchg cl, ch
    dec bx
    jl copy_next_scanline
    je loc_3cc1
    test ch, ch
    jne loc_3cb0
    xchg bx, cx
    test dl, dl
    jne loc_3ca6
    shr cx, 1
    rep movsw
    xchg bx, cx
    jae loc_3cc1
    movsb
    jmp SHORT loc_3cc1

loc_3ca6:
    lodsb
    xor al, dl
    stosb
    loop loc_3ca6
    xchg bx, cx
    jmp SHORT loc_3cc1

loc_3cb0:
    lodsb
    xor al, dl
    xchg al, ah
    rol ax, cl
    stosb
    xchg cl, ch
    rol ah, cl
    xchg cl, ch
    dec bx
    jg loc_3cb0

loc_3cc1:
    lodsb
    xor al, dl
    xchg al, ah
    rol ax, cl
    mov dh, BYTE PTR [bp-1Fh]
    and al, dh
    not dh
    and dh, BYTE PTR es:[di]
    or al, dh
    stosb

copy_next_scanline:
    dec WORD PTR [bp-28h]
    je copy_restore_cursor
    mov ax, WORD PTR [bp-2Ch]
    add WORD PTR [bp-16h], ax
    add WORD PTR [bp-1Eh], ax

copy_prepare_scanline:
    mov di, WORD PTR [bp-16h]
    shl di, 1
    add di, WORD PTR [bp-0Eh]
    mov di, WORD PTR ss:[di]
    add di, WORD PTR [bp-14h]
    mov si, WORD PTR [bp-1Eh]
    shl si, 1
    add si, WORD PTR [bp-2Ah]
    mov si, WORD PTR ss:[si]
    add si, WORD PTR [bp-1Ch]
    mov bx, WORD PTR [bp-0Ah]
    mov dh, BYTE PTR [bp-20h]
    sub ah, ah
    lodsb
    xor al, dl
    cmp cl, BYTE PTR [bp-0Ch]
    ja loc_3d14
    mov ah, al
    lodsb
    xor al, dl

loc_3d14:
    jmp WORD PTR [bp-22h]

copy_restore_cursor:
    cld
    push ss
    pop ds
    push cs
    call show_mouse_graphics
    push cs
    call show_mouse_graphics
    jmp loc_3b69

copy_forward_and:
    shl ax, cl
    and ah, dh
    not ah
    and BYTE PTR es:[di], ah
    inc di
    xchg cl, ch
    shl ax, cl
    xchg cl, ch
    dec bx
    jl copy_next_scanline
    je loc_3d63
    test ch, ch
    jne loc_3d4f
    xchg bx, cx

loc_3d40:
    lodsb
    xor al, dl
    not al
    and BYTE PTR es:[di], al
    inc di
    loop loc_3d40
    xchg bx, cx
    jmp SHORT loc_3d63

loc_3d4f:
    lodsb
    xor al, dl
    shl ax, cl
    not ah
    and BYTE PTR es:[di], ah
    inc di
    xchg cl, ch
    shl ax, cl
    xchg cl, ch
    dec bx
    jg loc_3d4f

loc_3d63:
    lodsb
    xor al, dl
    shl ax, cl
    and ah, BYTE PTR [bp-1Fh]
    not ah
    and BYTE PTR es:[di], ah
    jmp copy_next_scanline

copy_forward_or:
    shl ax, cl
    and ah, dh
    or BYTE PTR es:[di], ah
    inc di
    xchg cl, ch
    shl ax, cl
    xchg cl, ch
    dec bx
    jl loc_3db6
    je loc_3dab
    test ch, ch
    jne loc_3d99
    xchg bx, cx

loc_3d8c:
    lodsb
    xor al, dl
    or BYTE PTR es:[di], al
    inc di
    loop loc_3d8c
    xchg bx, cx
    jmp SHORT loc_3dab

loc_3d99:
    lodsb
    xor al, dl
    shl ax, cl
    or BYTE PTR es:[di], ah
    inc di
    xchg cl, ch
    shl ax, cl
    xchg cl, ch
    dec bx
    jg loc_3d99

loc_3dab:
    lodsb
    xor al, dl
    shl ax, cl
    and ah, BYTE PTR [bp-1Fh]
    or BYTE PTR es:[di], ah

loc_3db6:
    jmp copy_next_scanline

copy_forward_xor:
    shl ax, cl
    and ah, dh
    xor BYTE PTR es:[di], ah
    inc di
    xchg cl, ch
    shl ax, cl
    xchg cl, ch
    dec bx
    jl loc_3dfc
    je loc_3df1
    test ch, ch
    jne loc_3ddf
    xchg bx, cx

loc_3dd2:
    lodsb
    xor al, dl
    xor BYTE PTR es:[di], al
    inc di
    loop loc_3dd2
    xchg bx, cx
    jmp SHORT loc_3df1

loc_3ddf:
    lodsb
    xor al, dl
    shl ax, cl
    xor BYTE PTR es:[di], ah
    inc di
    xchg cl, ch
    shl ax, cl
    xchg cl, ch
    dec bx
    jg loc_3ddf

loc_3df1:
    lodsb
    xor al, dl
    shl ax, cl
    and ah, BYTE PTR [bp-1Fh]
    xor BYTE PTR es:[di], ah

loc_3dfc:
    jmp copy_next_scanline

copy_reverse_replace:
    xchg al, ah
    ror ax, cl
    and al, dh
    not dh
    and dh, BYTE PTR es:[di]
    or al, dh
    stosb
    xchg cl, ch
    ror ah, cl
    xchg cl, ch
    dec bx
    jl loc_3e60
    je loc_3e4c
    test ch, ch
    jne loc_3e3b
    xchg bx, cx
    test dl, dl
    jne loc_3e31
    dec di
    dec si
    shr cx, 1
    rep movsw
    inc di
    inc si
    xchg bx, cx
    jae loc_3e4c
    movsb
    jmp SHORT loc_3e4c

loc_3e31:
    lodsb
    xor al, dl
    stosb
    loop loc_3e31
    xchg bx, cx
    jmp SHORT loc_3e4c

loc_3e3b:
    lodsb
    xor al, dl
    xchg al, ah
    ror ax, cl
    stosb
    xchg cl, ch
    ror ah, cl
    xchg cl, ch
    dec bx
    jg loc_3e3b

loc_3e4c:
    lodsb
    xor al, dl
    xchg al, ah
    ror ax, cl
    mov dh, BYTE PTR [bp-1Fh]
    and al, dh
    not dh
    and dh, BYTE PTR es:[di]
    or al, dh
    stosb

loc_3e60:
    jmp copy_next_scanline

copy_reverse_and:
    ror ax, cl
    and ah, dh
    not ah
    and BYTE PTR es:[di], ah
    dec di
    xchg cl, ch
    ror ax, cl
    xchg cl, ch
    dec bx
    jl loc_3eae
    je loc_3ea1
    test cl, cl
    jne loc_3e8d
    xchg bx, cx

loc_3e7e:
    lodsb
    xor al, dl
    not al
    and BYTE PTR es:[di], al
    dec di
    loop loc_3e7e
    xchg bx, cx
    jmp SHORT loc_3ea1

loc_3e8d:
    lodsb
    xor al, dl
    ror ax, cl
    not ah
    and BYTE PTR es:[di], ah
    dec di
    xchg cl, ch
    ror ax, cl
    xchg cl, ch
    dec bx
    jg loc_3e8d

loc_3ea1:
    lodsb
    xor al, dl
    ror ax, cl
    and ah, BYTE PTR [bp-1Fh]
    not ah
    and BYTE PTR es:[di], ah

loc_3eae:
    jmp copy_next_scanline

copy_reverse_or:
    ror ax, cl
    and ah, dh
    or BYTE PTR es:[di], ah
    dec di
    xchg cl, ch
    ror ax, cl
    xchg cl, ch
    dec bx
    jl loc_3ef4
    je loc_3ee9
    test cl, cl
    jne loc_3ed7
    xchg bx, cx

loc_3eca:
    lodsb
    xor al, dl
    or BYTE PTR es:[di], al
    dec di
    loop loc_3eca
    xchg bx, cx
    jmp SHORT loc_3ee9

loc_3ed7:
    lodsb
    xor al, dl
    ror ax, cl
    or BYTE PTR es:[di], ah
    dec di
    xchg cl, ch
    ror ax, cl
    xchg cl, ch
    dec bx
    jg loc_3ed7

loc_3ee9:
    lodsb
    xor al, dl
    ror ax, cl
    and ah, BYTE PTR [bp-1Fh]
    or BYTE PTR es:[di], ah

loc_3ef4:
    jmp copy_next_scanline

copy_reverse_xor:
    ror ax, cl
    and ah, dh
    xor BYTE PTR es:[di], ah
    dec di
    xchg cl, ch
    ror ax, cl
    xchg cl, ch
    dec bx
    jl loc_3f3a
    je loc_3f2f
    test cl, cl
    jne loc_3f1d
    xchg bx, cx

loc_3f10:
    lodsb
    xor al, dl
    xor BYTE PTR es:[di], al
    dec di
    loop loc_3f10
    xchg bx, cx
    jmp SHORT loc_3f2f

loc_3f1d:
    lodsb
    xor al, dl
    ror ax, cl
    xor BYTE PTR es:[di], ah
    dec di
    xchg cl, ch
    ror ax, cl
    xchg cl, ch
    dec bx
    jg loc_3f1d

loc_3f2f:
    lodsb
    xor al, dl
    ror ax, cl
    and ah, BYTE PTR [bp-1Fh]
    xor BYTE PTR es:[di], ah

loc_3f3a:
    jmp copy_next_scanline
copy_packed_rectangle ENDP

copy_rop_forward            DW OFFSET copy_forward_replace, OFFSET copy_forward_and, OFFSET copy_forward_or
                            DW OFFSET copy_forward_xor
copy_rop_reverse            DW OFFSET copy_reverse_replace, OFFSET copy_reverse_and, OFFSET copy_reverse_or
                            DW OFFSET copy_reverse_xor
copy_rop_masks DB 0, 0, 0, 0, 0FFh, 0FFh, 0FFh, 0FFh

; Disable Hercules graphics before probing the color adapter RAM and CRTC.
probe_cga PROC FAR
    push di
    mov dx, hercules_config_port
    mov al, 0
    out dx, al
    mov ax, 0B800h
    mov cx, 0C0h
    mov di, 3F3Fh
    call probe_video_ram
    jne loc_3f75
    mov dx, color_crtc_port
    call probe_crtc_register
    mov ax, 1
    je loc_3f77

loc_3f75:
    sub ax, ax

loc_3f77:
    pop di
    retf
probe_cga ENDP

; Install the surface geometry and build the two-bank scanline offset table.
initialize_cga PROC FAR
    push di
    push si
    push ds
    mov cx, 0Eh
    mov si, OFFSET cga_surface_geometry
    mov di, OFFSET DEFAULT_PORT
    push ds
    pop es
    push cs
    pop ds
    rep movsb
    pop ds
    mov ax, WORD PTR [DEFAULT_PORT+8]
    shl ax, 1
    push ax
    call __nmalloc
    add sp, 2
    mov WORD PTR [mode_scanline_offsets], ax
    test ax, ax
    je loc_3fc4
    xchg di, ax
    sub si, si
    mov cx, WORD PTR [DEFAULT_PORT+8]
    push ds
    pop es

loc_3faa:
    mov ax, si
    shr ax, 1
    mul WORD PTR [mode_bytes_per_row]
    test si, 1
    je loc_3fbb
    add ax, 2000h

loc_3fbb:
    stosw
    inc si
    loop loc_3faa
    mov ax, 4
    int 10h

loc_3fc4:
    pop si
    pop di
    retf
initialize_cga ENDP

cga_surface_geometry        DW 0, 0B800h, 0, 0, 0C8h, 140h, 50h

; Classify and clip the endpoints before selecting a packed-pixel writer.
draw_packed_line PROC FAR
    push bp
    mov bp, sp
    add sp,-20h
    push di
    push si
    mov bx, WORD PTR [Graf]
    mov ax, WORD PTR [bx+28h]
    and ax, 0Fh
    mov cl, 2
    shl ax, cl
    add ax, WORD PTR [screen_bits]
    mov WORD PTR [bp-0Ch], ax
    mov di, WORD PTR [bx+26h]
    and di, 3
    mov al, BYTE PTR cs:line_rop_masks[di]
    mov BYTE PTR [bp-20h], al
    and di, 3
    shl di, 1
    mov ax, WORD PTR cs:line_rop_dispatch[di]
    mov WORD PTR [bp-16h], ax
    mov ax, WORD PTR [bx+6]
    mov cx, WORD PTR [bp+8]
    sub WORD PTR [bp+8], ax
    xchg cx, WORD PTR [bx+24h]
    sub cx, ax
    mov WORD PTR [bp-1Ch], cx
    add ax, WORD PTR [screen_x_origin]
    and ax, 3
    shl ax, 1
    mov WORD PTR [bp-18h], ax
    mov ax, WORD PTR [bx+4]
    mov dx, WORD PTR [bp+6]
    sub WORD PTR [bp+6], ax
    xchg dx, WORD PTR [bx+22h]
    sub dx, ax
    mov WORD PTR [bp-1Eh], dx
    les dx, DWORD PTR [bp+6]
    mov cx, es
    call classify_line_endpoint
    mov BYTE PTR [bp-9], al

loc_4046:
    les dx, DWORD PTR [bp-1Eh]
    mov cx, es
    call classify_line_endpoint
    mov BYTE PTR [bp-0Ah], al
    mov ah, BYTE PTR [bp-9]
    test ah, al
    jne loc_405f
    or ah, al
    jne loc_4067
    jmp loc_40f5

loc_405f:
    pop si
    pop di
    mov sp, bp
    pop bp
    retf 4

loc_4067:
    cmp al, 0
    jne loc_407d
    xchg al, BYTE PTR [bp-9]
    mov BYTE PTR [bp-0Ah], al
    xchg cx, WORD PTR [bp+8]
    mov WORD PTR [bp-1Ch], cx
    xchg dx, WORD PTR [bp+6]
    mov WORD PTR [bp-1Eh], dx

loc_407d:
    test al, 1
    je loc_409b
    mov di, WORD PTR [bp+8]
    sub di, cx
    mov si, WORD PTR [bx+1Ah]
    mov WORD PTR [bp-1Ch], si
    sub si, cx
    mov ax, WORD PTR [bp+6]
    sub ax, dx
    call scale_line_clip_delta
    add WORD PTR [bp-1Eh], cx
    jmp SHORT loc_4046

loc_409b:
    test al, 2
    je loc_40b9
    mov di, WORD PTR [bp+6]
    sub di, dx
    mov si, WORD PTR [bx+18h]
    mov WORD PTR [bp-1Eh], si
    sub si, dx
    mov ax, WORD PTR [bp+8]
    sub ax, cx
    call scale_line_clip_delta
    add WORD PTR [bp-1Ch], cx
    jmp SHORT loc_4046

loc_40b9:
    test al, 4
    je loc_40d9
    mov di, WORD PTR [bp+8]
    sub di, cx
    mov si, WORD PTR [bx+1Eh]
    dec si
    mov WORD PTR [bp-1Ch], si
    sub si, cx
    mov ax, WORD PTR [bp+6]
    sub ax, dx
    call scale_line_clip_delta
    add WORD PTR [bp-1Eh], cx
    jmp loc_4046

loc_40d9:
    mov di, WORD PTR [bp+6]
    sub di, dx
    mov si, WORD PTR [bx+1Ch]
    dec si
    mov WORD PTR [bp-1Eh], si
    sub si, dx
    mov ax, WORD PTR [bp+8]
    sub ax, cx
    call scale_line_clip_delta
    add WORD PTR [bp-1Ch], cx
    jmp loc_4046

loc_40f5:
    cmp cx, WORD PTR [bp+8]
    jle loc_4106
    xchg cx, WORD PTR [bp+8]
    mov WORD PTR [bp-1Ch], cx
    xchg dx, WORD PTR [bp+6]
    mov WORD PTR [bp-1Eh], dx

loc_4106:
    les di, DWORD PTR [bp+6]
    mov WORD PTR [bp-6], cx
    mov WORD PTR [bp-2], es
    inc WORD PTR [bp-2]
    mov ax, dx
    cmp ax, di
    jle loc_4119
    xchg di, ax

loc_4119:
    inc di
    mov WORD PTR [bp-4], di
    mov WORD PTR [bp-8], ax
    mov ax, dx
    add ax, WORD PTR [bx+4]
    add ax, WORD PTR [screen_row_origin]
    mov WORD PTR [bp-1Ah], ax
    mov si, 1
    mov ax, WORD PTR [bp+6]
    sub ax, dx
    jge loc_413a
    neg ax
    neg si

loc_413a:
    mov WORD PTR [bp-10h], ax
    mov dx, si
    mov si, 0FFFFh
    mov di, WORD PTR [bp+8]
    sub di, cx
    je loc_4153
    mov si, di
    cmp si, ax
    jae loc_4153
    mov si, ax
    neg si

loc_4153:
    sar si, 1
    mov WORD PTR [bp-0Eh], di
    add di, ax
    inc di
    xchg bx, di
    les ax, DWORD PTR [di]
    call check_cursor_overlap
    mov di, WORD PTR [di+0Eh]
    mov WORD PTR [bp-14h], di
    mov di, cx
    shr di, 1
    shr di, 1
    add ax, di
    mov WORD PTR [bp-12h], ax
    mov ax, 0C003h
    and cl, al
    ror ah, cl
    ror ah, cl
    mov cl, BYTE PTR [bp-18h]
    mov ds, WORD PTR [screen_bits+2]
    jmp SHORT loc_4194

line_replace:
    and ch, al
    not al
    and al, BYTE PTR es:[di]
    or al, ch
    stosb
    sub al, al
    dec bx
    jle loc_41d8

loc_4194:
    sub al, al
    mov di, WORD PTR [bp-1Ah]
    and di, 3
    add di, WORD PTR [bp-0Ch]
    mov ch, BYTE PTR [di]
    xor ch, BYTE PTR [bp-20h]
    ror ch, cl
    mov di, WORD PTR [bp-1Eh]
    shl di, 1
    add di, WORD PTR [bp-14h]
    mov di, WORD PTR ss:[di]
    add di, WORD PTR [bp-12h]

loc_41b4:
    or al, ah
    test si, si
    jl loc_41cc
    sub si, WORD PTR [bp-10h]
    ror ah, 1
    ror ah, 1
    jb loc_41c6
    dec bx
    jne loc_41b4

loc_41c6:
    inc WORD PTR [bp-12h]
    jmp WORD PTR [bp-16h]

loc_41cc:
    add WORD PTR [bp-1Eh], dx
    add WORD PTR [bp-1Ah], dx
    add si, WORD PTR [bp-0Eh]
    jmp WORD PTR [bp-16h]

loc_41d8:
    push ss
    pop ds
    push cs
    call show_mouse_graphics
    jmp loc_405f

line_and:
    and al, ch
    not al
    and BYTE PTR es:[di], al
    dec bx
    jg loc_4194
    jmp SHORT loc_41d8

line_or:
    and al, ch
    or BYTE PTR es:[di], al
    dec bx
    jg loc_4194
    jmp SHORT loc_41d8

line_xor:
    and al, ch
    xor BYTE PTR es:[di], al
    dec bx
    jg loc_4194
    jmp SHORT loc_41d8
draw_packed_line ENDP

; Scale a signed intersection delta, with a half-step correction.
scale_line_clip_delta PROC NEAR
    imul si
    idiv di
    mov cx, ax
    cwd
    test si, si
    jns loc_420e
    neg si

loc_420e:
    idiv si
    sar ax, 1
    sub cx, ax
    ret
scale_line_clip_delta ENDP

; Return the four clipping outcode bits for CX:DX against the rectangle in BX.
classify_line_endpoint PROC NEAR
    sub al, al
    cmp cx, WORD PTR [bx+1Ah]
    jge loc_421e
    or al, 1

loc_421e:
    cmp dx, WORD PTR [bx+18h]
    jge loc_4225
    or al, 2

loc_4225:
    cmp cx, WORD PTR [bx+1Eh]
    jl loc_422c
    or al, 4

loc_422c:
    cmp dx, WORD PTR [bx+1Ch]
    jl loc_4233
    or al, 8

loc_4233:
    ret
classify_line_endpoint ENDP

line_rop_dispatch           DW OFFSET line_replace, OFFSET line_and, OFFSET line_or
                            DW OFFSET line_xor
line_rop_masks DB 0, 0, 0, 0, 0FFh, 0FFh, 0FFh, 0FFh

; Clip a bitmap against the current port and dispatch by raster operation.
draw_packed_bitmap PROC FAR
    push bp
    mov bp, sp
    add sp,-2Ah
    push di
    push si
    mov bx, WORD PTR [Graf]
    les si, DWORD PTR [bp+0Eh]
    mov ax, WORD PTR [bp+0Ch]
    mov di, WORD PTR [bx+6]
    sub ax, di
    add di, WORD PTR [screen_x_origin]
    mov dx, 3
    and di, dx
    shl di, 1
    mov WORD PTR [bp-22h], di
    sub cx, cx
    mov di, ax
    sub di, WORD PTR [bx+1Ah]
    jge loc_4277
    mov ax, WORD PTR [bx+1Ah]
    sub cx, di

loc_4277:
    mov WORD PTR [bp-6], ax
    mov WORD PTR [bp-10h], cx
    mov di, ax
    add di, WORD PTR es:[si+2]
    sub di, cx
    cmp di, WORD PTR [bx+1Eh]
    jle loc_428d
    mov di, WORD PTR [bx+1Eh]

loc_428d:
    mov WORD PTR [bp-2], di
    sub di, ax
    mov WORD PTR [bp-0Ch], di
    jle loc_42d0
    mov ax, WORD PTR [bp+0Ah]
    sub ax, WORD PTR [bx+4]
    sub cx, cx
    mov di, ax
    sub di, WORD PTR [bx+18h]
    jge loc_42ab
    mov ax, WORD PTR [bx+18h]
    sub cx, di

loc_42ab:
    mov WORD PTR [bp-8], ax
    mov WORD PTR [bp-1Ch], cx
    mov di, ax
    add di, WORD PTR es:[si]
    sub di, cx
    cmp di, WORD PTR [bx+1Ch]
    jle loc_42c0
    mov di, WORD PTR [bx+1Ch]

loc_42c0:
    mov WORD PTR [bp-4], di
    add cx, WORD PTR [bp+0Ah]
    mov WORD PTR [bp-24h], cx
    sub di, ax
    mov WORD PTR [bp-0Eh], di
    jg loc_42dd

loc_42d0:
    les ax, DWORD PTR [bp+0Eh]
    mov dx, es
    pop si
    pop di
    mov sp, bp
    pop bp
    retf 0Ch

loc_42dd:
    mov ax, WORD PTR [bp-6]
    sub ax, WORD PTR [bp-10h]
    and al, dl
    mov ah, al
    neg al
    and al, dl
    jne loc_42ef
    mov al, 4

loc_42ef:
    shl ax, 1
    mov WORD PTR [bp-28h], ax
    les ax, DWORD PTR [bx]
    call check_cursor_overlap
    mov di, WORD PTR [bp-6]
    mov cx, di
    shr di, 1
    shr di, 1
    add ax, di
    mov WORD PTR [bp-16h], ax
    mov WORD PTR [bp-14h], es
    mov ax, WORD PTR [bx+0Eh]
    mov WORD PTR [bp-18h], ax
    mov al, 0FFh
    mov di, WORD PTR [bp-0Ch]
    neg cx
    and cx, dx
    jne loc_431d
    mov cl, 4

loc_431d:
    sub di, cx
    jle loc_4334
    mov cx, di
    shr di, 1
    shr di, 1
    neg cx
    and cx, dx
    je loc_4340
    inc di
    shl al, cl
    shl al, cl
    jmp SHORT loc_4340

loc_4334:
    mov cx, di
    neg cx
    and cx, dx
    shl cl, 1
    shl al, cl
    sub di, di

loc_4340:
    mov WORD PTR [bp-0Ah], di
    mov BYTE PTR [bp-29h], al
    mov ax, WORD PTR [bp+6]
    shl ax, 1
    shl ax, 1
    and ax, 3Ch
    les di, DWORD PTR [screen_bits]
    add ax, di
    mov WORD PTR [bp-20h], ax
    mov WORD PTR [bp-1Eh], es
    mov bx, WORD PTR [bp+8]
    and bx, 7
    mov al, BYTE PTR cs:bitmap_rop_masks[bx]
    mov BYTE PTR [bp-2Ah], al
    and bl, dl
    shl bx, 1
    mov ax, WORD PTR cs:bitmap_rop_dispatch[bx]
    mov WORD PTR [bp+8], ax
    lds si, DWORD PTR [bp+0Eh]
    mov di, WORD PTR [bp-10h]
    mov cx, di
    and cx, dx
    shr di, 1
    shr di, 1
    mov ax, WORD PTR [si+4]
    mov WORD PTR [bp-26h], ax
    mul WORD PTR [bp-1Ch]
    push bx
    mov bx, ax
    and ax, 0Fh
    shr dx, 1
    rcr bx, 1
    shr dx, 1
    rcr bx, 1
    shr dx, 1
    rcr bx, 1
    shr dx, 1
    rcr bx, 1
    mov dx, ds
    add dx, bx
    mov WORD PTR [bp-1Ah], dx
    mov ds, dx
    pop bx
    add ax, 6
    add si, ax
    add si, di
    mov WORD PTR [bp-1Ch], si
    shl cx, 1
    mov WORD PTR [bp-10h], cx
    mov al, 0FFh
    shr al, cl
    mov BYTE PTR [bp-12h], al
    jmp SHORT loc_43f5

loc_43c5:
    mov dl, al
    mov al, ah
    not al
    and al, BYTE PTR es:[di]
    and ah, dh
    or al, ah
    stosb
    mov al, dl
    xchg cl, ch
    shl ax, cl
    xchg cl, ch
    lodsb

bitmap_replace:
    shl ax, cl
    dec bx
    jge loc_43c5
    and ah, BYTE PTR [bp-29h]
    mov al, ah
    not al
    and al, BYTE PTR es:[di]
    and ah, dh
    or al, ah
    stosb

loc_43f0:
    dec WORD PTR [bp-0Eh]
    je loc_4452

loc_43f5:
    mov ax, WORD PTR [bp-24h]
    inc WORD PTR [bp-24h]
    and ax, 3
    les di, DWORD PTR [bp-20h]
    add di, ax
    mov dh, BYTE PTR es:[di]
    mov cl, BYTE PTR [bp-22h]
    rol dh, cl
    xor dh, BYTE PTR [bp-2Ah]
    les di, DWORD PTR [bp-16h]
    mov si, WORD PTR [bp-8]
    inc WORD PTR [bp-8]
    shl si, 1
    add si, WORD PTR [bp-18h]
    add di, WORD PTR ss:[si]
    lds si, DWORD PTR [bp-1Ch]
    mov cx, WORD PTR [bp-26h]
    add cx, WORD PTR [bp-1Ch]
    mov ax, cx
    and cx, 0Fh
    mov WORD PTR [bp-1Ch], cx
    mov cl, 4
    shr ax, cl
    mov cx, ds
    add ax, cx
    mov WORD PTR [bp-1Ah], ax
    mov bx, WORD PTR [bp-0Ah]
    mov cx, WORD PTR [bp-28h]
    sub ah, ah
    lodsb
    and al, BYTE PTR [bp-12h]
    cmp cl, BYTE PTR [bp-10h]
    ja loc_444f
    mov ah, al
    lodsb

loc_444f:
    jmp WORD PTR [bp+8]

loc_4452:
    push ss
    pop ds
    push cs
    call show_mouse_graphics
    jmp loc_42d0

loc_445b:
    and ah, dh
    xor BYTE PTR es:[di], ah
    inc di
    xchg cl, ch
    shl ax, cl
    xchg cl, ch
    lodsb

bitmap_xor:
    shl ax, cl
    dec bx
    jge loc_445b
    and ah, BYTE PTR [bp-29h]
    and ah, dh
    xor BYTE PTR es:[di], ah
    jmp loc_43f0

loc_4478:
    and ah, dh
    or BYTE PTR es:[di], ah
    inc di
    xchg cl, ch
    shl ax, cl
    xchg cl, ch
    lodsb

bitmap_or:
    shl ax, cl
    dec bx
    jge loc_4478
    and ah, BYTE PTR [bp-29h]
    and ah, dh
    or BYTE PTR es:[di], ah
    jmp loc_43f0

loc_4495:
    and ah, dh
    not ah
    and BYTE PTR es:[di], ah
    inc di
    xchg cl, ch
    shl ax, cl
    xchg cl, ch
    lodsb

bitmap_and:
    shl ax, cl
    dec bx
    jge loc_4495
    and ah, BYTE PTR [bp-29h]
    and ah, dh
    not ah
    and BYTE PTR es:[di], ah
    jmp loc_43f0
draw_packed_bitmap ENDP

bitmap_rop_dispatch         DW OFFSET bitmap_replace, OFFSET bitmap_and, OFFSET bitmap_or
                            DW OFFSET bitmap_xor
bitmap_rop_masks DB 0, 0, 0, 0, 0FFh, 0FFh, 0FFh, 0FFh

; The CGA descriptor has an empty adapter-specific exit hook.
cga_mode_exit PROC FAR
    retf
cga_mode_exit ENDP

; Clip the rectangle, then apply the selected raster operation across its rows.
fill_packed_rectangle PROC FAR
    push bp
    mov bp, sp
    add sp,-1Ah
    push di
    push si
    mov di, WORD PTR [Graf]
    les si, DWORD PTR [bp+0Ah]
    mov ax, WORD PTR es:[si+2]
    mov bx, WORD PTR [di+6]
    sub ax, bx
    mov dx, 3
    add bx, WORD PTR [screen_x_origin]
    and bx, dx
    shl bx, 1
    mov WORD PTR [bp-16h], bx
    cmp ax, WORD PTR [di+1Ah]
    jge loc_44f5
    mov ax, WORD PTR [di+1Ah]

loc_44f5:
    mov WORD PTR [bp-6], ax
    mov cx, ax
    shr ax, 1
    shr ax, 1
    mov WORD PTR [bp-10h], ax
    mov bx, WORD PTR es:[si+6]
    sub bx, WORD PTR [di+6]
    cmp bx, WORD PTR [di+1Eh]
    jle loc_4510
    mov bx, WORD PTR [di+1Eh]

loc_4510:
    mov WORD PTR [bp-2], bx
    sub bx, cx
    jle loc_453d
    and cx, dx
    jne loc_451e
    sub bx, 4

loc_451e:
    mov ax, 0FFFFh
    shr al, cl
    shr al, cl
    neg cx
    and cx, dx
    sub bx, cx
    jl loc_454a
    mov cx, bx
    shr bx, 1
    shr bx, 1
    and cl, dl
    shl cl, 1
    shr ah, cl
    not ah
    jmp SHORT loc_455a

loc_453d:
    les ax, DWORD PTR [bp+0Ah]
    mov dx, es
    pop si
    pop di
    mov sp, bp
    pop bp
    retf 8

loc_454a:
    mov cx, bx
    neg cx
    and cl, dl
    shl cl, 1
    shl ah, cl
    and al, ah
    sub ah, ah
    sub bx, bx

loc_455a:
    mov WORD PTR [bp-0Ah], bx
    not ax
    mov WORD PTR [bp-0Ch], ax
    mov ax, WORD PTR es:[si]
    mov bx, ax
    sub bx, WORD PTR [di+4]
    mov cx, bx
    sub cx, WORD PTR [di+18h]
    jge loc_4576
    sub ax, cx
    mov bx, WORD PTR [di+18h]

loc_4576:
    mov WORD PTR [bp-8], bx
    add ax, WORD PTR [screen_row_origin]
    mov WORD PTR [bp-18h], ax
    mov si, WORD PTR es:[si+4]
    sub si, WORD PTR [di+4]
    cmp si, WORD PTR [di+1Ch]
    jle loc_458f
    mov si, WORD PTR [di+1Ch]

loc_458f:
    mov WORD PTR [bp-4], si
    sub si, bx
    jle loc_453d
    les ax, DWORD PTR [di]
    call check_cursor_overlap
    add WORD PTR [bp-10h], ax
    mov WORD PTR [bp-0Eh], es
    mov bx, WORD PTR [di+0Eh]
    mov ax, WORD PTR [bp+6]
    shl ax, 1
    shl ax, 1
    and ax, 3Ch
    les di, DWORD PTR [screen_bits]
    add ax, di
    mov WORD PTR [bp-14h], ax
    mov WORD PTR [bp-12h], es
    mov di, WORD PTR [bp+8]
    and di, 7
    mov al, BYTE PTR cs:fill_rop_masks[di]
    mov BYTE PTR [bp-1Ah], al
    and di, dx
    shl di, 1
    mov dx, WORD PTR cs:fill_rop_dispatch[di]
    jmp SHORT loc_45fb

fill_replace:
    and BYTE PTR es:[di], al
    not al
    and al, ah
    or BYTE PTR es:[di], al
    inc di
    mov al, ah
    shr cx, 1
    rep stosw
    jae loc_45e7
    stosb

loc_45e7:
    mov al, BYTE PTR [bp-0Bh]
    cmp al, 0FFh
    je loc_45f8
    and BYTE PTR es:[di], al
    not al
    and al, ah
    or BYTE PTR es:[di], al

loc_45f8:
    dec si
    je loc_462b

loc_45fb:
    mov ax, WORD PTR [bp-18h]
    inc WORD PTR [bp-18h]
    and ax, 3
    les di, DWORD PTR [bp-14h]
    add di, ax
    mov ah, BYTE PTR es:[di]
    mov cl, BYTE PTR [bp-16h]
    rol ah, cl
    xor ah, BYTE PTR [bp-1Ah]
    mov al, BYTE PTR [bp-0Ch]
    mov di, WORD PTR [bp-8]
    inc WORD PTR [bp-8]
    shl di, 1
    mov di, WORD PTR [bx+di]
    les cx, DWORD PTR [bp-10h]
    add di, cx
    mov cx, WORD PTR [bp-0Ah]
    jmp dx

loc_462b:
    push cs
    call show_mouse_graphics
    jmp loc_453d

fill_xor:
    not al
    and al, ah
    xor BYTE PTR es:[di], al
    inc di
    jcxz loc_4642

loc_463c:
    xor BYTE PTR es:[di], ah
    inc di
    loop loc_463c

loc_4642:
    mov al, BYTE PTR [bp-0Bh]
    xor al, 0FFh
    je loc_45f8
    and al, ah
    xor BYTE PTR es:[di], al
    jmp SHORT loc_45f8

fill_or:
    not al
    and al, ah
    or BYTE PTR es:[di], al
    inc di
    jcxz loc_4660

loc_465a:
    or BYTE PTR es:[di], ah
    inc di
    loop loc_465a

loc_4660:
    mov al, BYTE PTR [bp-0Bh]
    xor al, 0FFh
    je loc_45f8
    and al, ah
    or BYTE PTR es:[di], al
    jmp SHORT loc_45f8

fill_and:
    or al, ah
    and BYTE PTR es:[di], al
    inc di
    jcxz loc_467c

loc_4676:
    and BYTE PTR es:[di], ah
    inc di
    loop loc_4676

loc_467c:
    mov al, BYTE PTR [bp-0Bh]
    cmp al, 0FFh
    je loc_4688
    or al, ah
    and BYTE PTR es:[di], al

loc_4688:
    jmp loc_45f8
fill_packed_rectangle ENDP

fill_rop_dispatch           DW OFFSET fill_replace, OFFSET fill_and, OFFSET fill_or
                            DW OFFSET fill_xor
fill_rop_masks DB 0, 0, 0, 0, 0FFh, 0FFh, 0FFh, 0FFh

; Composite a packed source, using CS lookup masks for transparent pixel pairs.
composite_packed_bitmap PROC FAR
    push bp
    mov bp, sp
    add sp,-1Ch
    push di
    push si
    mov bx, WORD PTR [Graf]
    les si, DWORD PTR [bp+0Ch]
    mov ax, WORD PTR [bp+0Ah]
    sub ax, WORD PTR [bx+6]
    sub cx, cx
    mov di, ax
    sub di, WORD PTR [bx+1Ah]
    jge loc_46be
    mov ax, WORD PTR [bx+1Ah]
    sub cx, di

loc_46be:
    mov WORD PTR [bp-6], ax
    mov di, ax
    add di, WORD PTR es:[si+2]
    sub di, cx
    cmp di, WORD PTR [bx+1Eh]
    jle loc_46d1
    mov di, WORD PTR [bx+1Eh]

loc_46d1:
    mov WORD PTR [bp-2], di
    sub di, ax
    mov WORD PTR [bp-1Ah], di
    jle loc_470b
    mov ax, WORD PTR [bp+8]
    sub ax, WORD PTR [bx+4]
    sub dx, dx
    mov di, ax
    sub di, WORD PTR [bx+18h]
    jge loc_46ef
    mov ax, WORD PTR [bx+18h]
    sub dx, di

loc_46ef:
    mov WORD PTR [bp-8], ax
    mov WORD PTR [bp-14h], dx
    mov di, ax
    add di, WORD PTR es:[si]
    sub di, dx
    cmp di, WORD PTR [bx+1Ch]
    jle loc_4704
    mov di, WORD PTR [bx+1Ch]

loc_4704:
    mov WORD PTR [bp-4], di
    sub di, ax
    jg loc_4718

loc_470b:
    les ax, DWORD PTR [bp+0Ch]
    mov dx, es
    pop si
    pop di
    mov sp, bp
    pop bp
    retf 0Ah

loc_4718:
    mov WORD PTR [bp-1Ch], di
    les ax, DWORD PTR [bx]
    call check_cursor_overlap
    mov si, WORD PTR [bp-6]
    shr si, 1
    shr si, 1
    add ax, si
    mov WORD PTR [bp-0Eh], ax
    mov ax, WORD PTR [bx+0Eh]
    mov WORD PTR [bp-10h], ax
    lds si, DWORD PTR [bp+0Ch]
    mov ax, WORD PTR [si+4]
    mov WORD PTR [bp-16h], ax
    mul WORD PTR [bp-14h]
    mov bx, ax
    and ax, 0Fh
    shr dx, 1
    rcr bx, 1
    shr dx, 1
    rcr bx, 1
    shr dx, 1
    rcr bx, 1
    shr dx, 1
    rcr bx, 1
    mov dx, ds
    add dx, bx
    mov ds, dx
    add ax, 6
    add si, ax
    mov ax, cx
    mov dx, 3
    and cx, dx
    shr ax, 1
    shr ax, 1
    add si, ax
    mov WORD PTR [bp-14h], si
    mov ax, WORD PTR [bp-6]
    sub ax, cx
    and al, dl
    mov ah, al
    neg al
    and al, dl
    jne loc_477f
    mov al, 4

loc_477f:
    shl ax, 1
    shl cx, 1
    mov WORD PTR [bp-18h], ax
    mov WORD PTR [bp-0Ch], cx
    mov ax, 0FFFFh
    mov di, WORD PTR [bp-1Ah]
    mov cx, WORD PTR [bp-6]
    and cl, dl
    jne loc_4799
    sub di, 4

loc_4799:
    shr al, cl
    shr al, cl
    neg cx
    and cx, dx
    sub di, cx
    jle loc_47b8
    mov cx, di
    shr di, 1
    shr di, 1
    neg cx
    and cl, dl
    je loc_47c6
    shl cl, 1
    shl ah, cl
    inc di
    jmp SHORT loc_47c6

loc_47b8:
    mov cx, di
    neg cx
    and cx, dx
    shl cl, 1
    shl ah, cl
    and al, ah
    sub di, di

loc_47c6:
    mov WORD PTR [bp-0Ah], di
    mov BYTE PTR [bp-12h], al
    mov BYTE PTR [bp-11h], ah
    mov cx, WORD PTR [bp-18h]
    mov bx, WORD PTR [bp+6]
    cmp bx, 8
    je loc_47fc
    cmp bx, 0Ah
    je loc_47f7
    and bx, 7
    mov dl, BYTE PTR cs:composite_rop_masks[bx]
    and bl, 3
    shl bx, 1
    mov ax, WORD PTR cs:composite_rop_dispatch[bx]
    mov WORD PTR [bp+6], ax
    jmp composite_prepare_scanline

loc_47f7:
    mov ax, OFFSET composite_transparent_replace
    jmp SHORT loc_47ff

loc_47fc:
    mov ax, OFFSET composite_transparent_blend

loc_47ff:
    mov WORD PTR [bp+6], ax
    sub dl, dl
    jmp SHORT composite_prepare_scanline

composite_replace:
    xchg al, ah
    and al, dh
    not dh
    and dh, BYTE PTR es:[di]
    or al, dh
    stosb
    xchg cl, ch
    rol ah, cl
    xchg cl, ch
    dec bx
    jl composite_next_scanline
    je loc_484d
    test ch, ch
    jne loc_483c
    xchg bx, cx
    test dl, dl
    jne loc_4832
    shr cx, 1
    rep movsw
    xchg bx, cx
    jae loc_484d
    movsb
    jmp SHORT loc_484d

loc_4832:
    lodsb
    xor al, dl
    stosb
    loop loc_4832
    xchg bx, cx
    jmp SHORT loc_484d

loc_483c:
    lodsb
    xor al, dl
    xchg al, ah
    rol ax, cl
    stosb
    xchg cl, ch
    rol ah, cl
    xchg cl, ch
    dec bx
    jg loc_483c

loc_484d:
    lodsb
    xor al, dl
    xchg al, ah
    rol ax, cl
    mov dh, BYTE PTR [bp-11h]
    and al, dh
    not dh
    and dh, BYTE PTR es:[di]
    or al, dh
    stosb

composite_next_scanline:
    dec WORD PTR [bp-1Ch]
    je loc_48b0
    mov bx, WORD PTR [bp-16h]
    add bx, WORD PTR [bp-14h]
    mov ax, bx
    and bx, 0Fh
    mov WORD PTR [bp-14h], bx
    shr ax, 1
    shr ax, 1
    shr ax, 1
    shr ax, 1
    mov bx, ds
    add ax, bx
    mov ds, ax

composite_prepare_scanline:
    mov di, WORD PTR [bp-8]
    inc WORD PTR [bp-8]
    shl di, 1
    add di, WORD PTR [bp-10h]
    mov di, WORD PTR ss:[di]
    add di, WORD PTR [bp-0Eh]
    mov bx, WORD PTR [bp-0Ah]
    mov dh, BYTE PTR [bp-12h]
    mov si, WORD PTR [bp-14h]
    sub ah, ah
    lodsb
    xor al, dl
    cmp cl, BYTE PTR [bp-0Ch]
    ja loc_48ab
    mov ah, al
    lodsb
    xor al, dl

loc_48ab:
    shl ax, cl
    jmp WORD PTR [bp+6]

loc_48b0:
    push ss
    pop ds
    push cs
    call show_mouse_graphics
    jmp loc_470b

composite_and:
    and ah, dh
    not ah
    and BYTE PTR es:[di], ah
    inc di
    xchg cl, ch
    shl ax, cl
    xchg cl, ch
    dec bx
    jl composite_next_scanline
    je loc_48f5
    test ch, ch
    jne loc_48e1
    xchg bx, cx

loc_48d2:
    lodsb
    xor al, dl
    not al
    and BYTE PTR es:[di], al
    inc di
    loop loc_48d2
    xchg bx, cx
    jmp SHORT loc_48f5

loc_48e1:
    lodsb
    xor al, dl
    shl ax, cl
    not ah
    and BYTE PTR es:[di], ah
    inc di
    xchg cl, ch
    shl ax, cl
    xchg cl, ch
    dec bx
    jg loc_48e1

loc_48f5:
    lodsb
    xor al, dl
    shl ax, cl
    and ah, BYTE PTR [bp-11h]
    not ah
    and BYTE PTR es:[di], ah
    jmp composite_next_scanline

composite_or:
    and ah, dh
    or BYTE PTR es:[di], ah
    inc di
    xchg cl, ch
    shl ax, cl
    xchg cl, ch
    dec bx
    jl loc_4946
    je loc_493b
    test ch, ch
    jne loc_4929
    xchg bx, cx

loc_491c:
    lodsb
    xor al, dl
    or BYTE PTR es:[di], al
    inc di
    loop loc_491c
    xchg bx, cx
    jmp SHORT loc_493b

loc_4929:
    lodsb
    xor al, dl
    shl ax, cl
    or BYTE PTR es:[di], ah
    inc di
    xchg cl, ch
    shl ax, cl
    xchg cl, ch
    dec bx
    jg loc_4929

loc_493b:
    lodsb
    xor al, dl
    shl ax, cl
    and ah, BYTE PTR [bp-11h]
    or BYTE PTR es:[di], ah

loc_4946:
    jmp composite_next_scanline

composite_xor:
    and ah, dh
    xor BYTE PTR es:[di], ah
    inc di
    xchg cl, ch
    shl ax, cl
    xchg cl, ch
    dec bx
    jl loc_498a
    je loc_497f
    test ch, ch
    jne loc_496d
    xchg bx, cx

loc_4960:
    lodsb
    xor al, dl
    xor BYTE PTR es:[di], al
    inc di
    loop loc_4960
    xchg bx, cx
    jmp SHORT loc_497f

loc_496d:
    lodsb
    xor al, dl
    shl ax, cl
    xor BYTE PTR es:[di], ah
    inc di
    xchg cl, ch
    shl ax, cl
    xchg cl, ch
    dec bx
    jg loc_496d

loc_497f:
    lodsb
    xor al, dl
    shl ax, cl
    and ah, BYTE PTR [bp-11h]
    xor BYTE PTR es:[di], ah

loc_498a:
    jmp composite_next_scanline

composite_transparent_blend:
    xchg al, ah
    and al, dh
    mov dx, bx
    mov bx, OFFSET transparent_pair_masks
    xlat cs:BYTE PTR [bx]
    and BYTE PTR es:[di], al
    inc di
    xchg cl, ch
    rol ah, cl
    xchg cl, ch
    dec dx
    jl loc_49db
    je loc_49ce
    test ch, ch
    jne loc_49ba
    xchg cx, dx

loc_49ad:
    lodsb
    xlat cs:BYTE PTR [bx]
    and BYTE PTR es:[di], al
    inc di
    loop loc_49ad
    xchg cx, dx
    jmp SHORT loc_49ce

loc_49ba:
    lodsb
    xchg al, ah
    rol ax, cl
    xlat cs:BYTE PTR [bx]
    and BYTE PTR es:[di], al
    inc di
    xchg cl, ch
    rol ah, cl
    xchg cl, ch
    dec dx
    jg loc_49ba

loc_49ce:
    lodsb
    xchg al, ah
    rol ax, cl
    and al, BYTE PTR [bp-11h]
    xlat cs:BYTE PTR [bx]
    and BYTE PTR es:[di], al

loc_49db:
    sub dl, dl
    jmp composite_next_scanline

composite_transparent_replace:
    xchg al, ah
    and al, dh
    mov dh, BYTE PTR [bp-11h]
    push bp
    mov bp, bx
    mov bx, OFFSET transparent_pair_masks
    mov dl, al
    xlat cs:BYTE PTR [bx]
    and al, BYTE PTR es:[di]
    or al, dl
    stosb
    xchg cl, ch
    rol ah, cl
    xchg cl, ch
    dec bp
    jl loc_4a42
    je loc_4a31
    test ch, ch
    jne loc_4a19
    xchg cx, bp

loc_4a08:
    lodsb
    mov dl, al
    xlat cs:BYTE PTR [bx]
    and al, BYTE PTR es:[di]
    or al, dl
    stosb
    loop loc_4a08
    xchg cx, bp
    jmp SHORT loc_4a31

loc_4a19:
    lodsb
    xchg al, ah
    rol ax, cl
    mov dl, al
    xlat cs:BYTE PTR [bx]
    and al, BYTE PTR es:[di]
    or al, dl
    stosb
    xchg cl, ch
    rol ah, cl
    xchg cl, ch
    dec bp
    jg loc_4a19

loc_4a31:
    lodsb
    xchg al, ah
    rol ax, cl
    and al, dh
    mov dl, al
    xlat cs:BYTE PTR [bx]
    and al, BYTE PTR es:[di]
    or al, dl
    stosb

loc_4a42:
    pop bp
    sub dl, dl
    jmp composite_next_scanline
composite_packed_bitmap ENDP

; Preserve the destination pair wherever its source pair is zero.
transparent_pair_masks LABEL BYTE
pixel_index = 0
REPT 256
    preserve_mask = 0
    pair_mask = 3
    REPT 4
        IF (pixel_index AND pair_mask) EQ 0
            preserve_mask = preserve_mask OR pair_mask
        ENDIF
        pair_mask = pair_mask SHL 2
    ENDM
    DB preserve_mask
    pixel_index = pixel_index + 1
ENDM

composite_rop_dispatch      DW OFFSET composite_replace, OFFSET composite_and, OFFSET composite_or
                            DW OFFSET composite_xor
composite_rop_masks DB 0, 0, 0, 0, 0FFh, 0FFh, 0FFh, 0FFh

; Probe the monochrome CRTC and wait for the Hercules status bit to change.
probe_hercules PROC FAR
    mov dx, mono_crtc_port
    call probe_crtc_register
    jne loc_4b76
    mov dx, mono_status_port
    in al, dx
    mov ah, al
    mov cx, 8000h

loc_4b69:
    in al, dx
    and ax, 8080h
    cmp al, ah
    loope loc_4b69
    mov ax, 1
    jne loc_4b78

loc_4b76:
    sub ax, ax

loc_4b78:
    retf
probe_hercules ENDP

; Build the four-bank scanline table and program the monochrome adapter.
initialize_hercules PROC FAR
    push di
    push si
    push ds
    mov cx, 0Eh
    mov si, OFFSET hercules_surface_geometry
    mov di, OFFSET DEFAULT_PORT
    push ds
    pop es
    push cs
    pop ds
    rep movsb
    pop ds
    mov ax, WORD PTR [DEFAULT_PORT+8]
    shl ax, 1
    push ax
    call __nmalloc
    add sp, 2
    mov WORD PTR [mode_scanline_offsets], ax
    test ax, ax
    jne loc_4ba4

loc_4ba1:
    pop si
    pop di
    retf

loc_4ba4:
    xchg di, ax
    sub si, si
    mov cx, WORD PTR [DEFAULT_PORT+8]

loc_4bab:
    mov ax, si
    shr ax, 1
    shr ax, 1
    mul WORD PTR [mode_bytes_per_row]
    mov WORD PTR [di], ax
    mov ax, si
    and ax, 3
    mov dx, 2000h
    mul dx
    add ax, WORD PTR [di]
    stosw
    inc si
    loop loc_4bab
    mov es, cx
    mov BYTE PTR es:[449h], 6
    mov dx, hercules_config_port
    mov al, 1
    out dx, al
    mov dx, mono_mode_port
    mov al, 2
    out dx, al
    mov dx, mono_crtc_port
    sub di, di

loc_4be0:
    mov ax, di
    mov ah, BYTE PTR cs:hercules_crtc_registers[di]
    out dx, ax
    inc di
    cmp di, 0Ch
    jb loc_4be0
    mov dx, mono_mode_port
    mov al, 0Ah
    out dx, al
    jmp SHORT loc_4ba1
initialize_hercules ENDP

hercules_surface_geometry   DW 0, 0B000h, 0, 0, 15Ch, 168h, 5Ah
hercules_crtc_registers     DB 35h, 2Dh, 2Eh, 7, 5Bh, 2, 57h, 57h, 2, 3, 0, 0

; Disable Hercules graphics through its configuration port.
hercules_mode_exit PROC FAR
    mov dx, hercules_config_port
    mov al, 0
    out dx, al
    retf
hercules_mode_exit ENDP

; Write and read back register 0Fh, restoring its previous value.
probe_crtc_register PROC NEAR
    mov ax, 550Fh
    out dx, al
    inc dx
    in al, dx
    xchg al, ah
    out dx, al
    mov cx, 100h

loc_4c23:
    loop loc_4c23
    in al, dx
    cmp al, 55h
    xchg al, ah
    out dx, al
    ret
probe_crtc_register ENDP

; Write and compare a decreasing byte pattern in the supplied video RAM window.
probe_video_ram PROC NEAR
    mov es, ax
    push cx
    push di

loc_4c30:
    mov al, cl
    stosb
    loop loc_4c30
    pop di
    pop cx

loc_4c37:
    mov al, cl
    scasb
    loope loc_4c37
    ret
probe_video_ram ENDP

mode_boundary_fill DB 0

END

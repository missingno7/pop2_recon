; DOS VGA adapter hooks and scanline raster operations.
.MODEL MEDIUM
.DATA
EXTRN Graf:WORD
EXTRN default_port:BYTE
EXTRN surface_row_stride:WORD
EXTRN surface_scanline_offsets:WORD
.CODE
EXTRN show_mouse_graphics:NEAR
EXTRN check_cursor_overlap:NEAR
EXTRN __nmalloc:FAR

PUBLIC copy_vga_rectangle
PUBLIC probe_vga
PUBLIC initialize_vga
PUBLIC draw_vga_line
PUBLIC scale_vga_line_clip
PUBLIC classify_vga_line_endpoint
PUBLIC draw_vga_bitmap
PUBLIC vga_mode_exit
PUBLIC fill_vga_rectangle
PUBLIC composite_vga_bitmap
PUBLIC vga_mode_descriptor
PUBLIC vga_palette
PUBLIC vga_copy_dispatch
PUBLIC vga_surface_geometry
PUBLIC vga_line_dispatch
PUBLIC vga_bitmap_dispatch
PUBLIC vga_fill_dispatch
PUBLIC vga_composite_dispatch

vga_mode_descriptor LABEL WORD
    DW 5, 140h, 100h, 101h
    DW OFFSET probe_vga, OFFSET initialize_vga, OFFSET vga_mode_exit, 0, 0, 0
    DW OFFSET copy_vga_rectangle, OFFSET draw_vga_line, OFFSET draw_vga_bitmap, OFFSET fill_vga_rectangle, OFFSET composite_vga_bitmap

vga_palette LABEL BYTE
    DB 0, 0, 0, 0, 0, 2Ah, 0, 2Ah, 0, 0, 2Ah, 2Ah
    DB 2Ah, 0, 0, 2Ah, 0, 2Ah, 2Ah, 15h, 0, 2Ah, 2Ah, 2Ah
    DB 15h, 15h, 15h, 15h, 15h, 3Fh, 15h, 3Fh, 15h, 15h, 3Fh, 3Fh
    DB 3Fh, 15h, 15h, 0, 0, 0, 3Fh, 3Fh, 15h, 3Fh, 3Fh, 3Fh

; Clip both rectangles, then copy scanlines with the selected raster operation.
copy_vga_rectangle PROC FAR
    push bp
    mov bp, sp
    add sp, -1Ch
    push di
    push si
    mov bx, WORD PTR [bp + 10h]
    mov ax, WORD PTR [bx + 0Ch]
    mov WORD PTR [bp - 1Ch], ax
    les si, DWORD PTR [bp + 8]
    sub dx, dx
    lods WORD PTR es:[si]
    sub ax, WORD PTR [bx + 4]
    jge vga_66d4
    xchg dx, ax

vga_66d4:
    mov WORD PTR [bp - 14h], ax
    sub cx, cx
    lods WORD PTR es:[si]
    sub ax, WORD PTR [bx + 6]
    jge vga_66e1
    xchg cx, ax

vga_66e1:
    mov WORD PTR [bp - 12h], ax
    sub di, di
    lods WORD PTR es:[si]
    sub ax, WORD PTR [bx + 8]
    jle vga_66ee
    xchg di, ax

vga_66ee:
    mov WORD PTR [bp - 10h], di
    sub di, di
    lods WORD PTR es:[si]
    sub ax, WORD PTR [bx + 0Ah]
    jle vga_66fb
    xchg di, ax

vga_66fb:
    mov WORD PTR [bp - 0Eh], di
    mov bx, WORD PTR [bp + 12h]
    mov ax, WORD PTR [bx + 0Ch]
    mov WORD PTR [bp - 4], ax
    les si, DWORD PTR [bp + 0Ch]
    lods WORD PTR es:[si]
    sub ax, dx
    sub ax, WORD PTR [bx + 4]
    jge vga_6718
    sub WORD PTR [bp - 14h], ax
    sub ax, ax

vga_6718:
    mov WORD PTR [bp - 0Ch], ax
    lods WORD PTR es:[si]
    sub ax, cx
    sub ax, WORD PTR [bx + 6]
    jge vga_6729
    sub WORD PTR [bp - 12h], ax
    sub ax, ax

vga_6729:
    mov WORD PTR [bp - 0Ah], ax
    lods WORD PTR es:[si]
    sub ax, WORD PTR [bp - 10h]
    cmp ax, WORD PTR [bx + 8]
    jle vga_6739
    mov ax, WORD PTR [bx + 8]

vga_6739:
    sub ax, WORD PTR [bx + 4]
    xchg dx, ax
    lods WORD PTR es:[si]
    sub ax, WORD PTR [bp - 0Eh]
    cmp ax, WORD PTR [bx + 0Ah]
    jle vga_674a
    mov ax, WORD PTR [bx + 0Ah]

vga_674a:
    sub ax, WORD PTR [bx + 6]
    xchg cx, ax
    cmp bx, WORD PTR [Graf]
    jne vga_6780
    mov ax, WORD PTR [bx + 18h]
    sub ax, WORD PTR [bp - 0Ch]
    jle vga_6762
    add WORD PTR [bp - 0Ch], ax
    add WORD PTR [bp - 14h], ax

vga_6762:
    mov ax, WORD PTR [bx + 1Ah]
    sub ax, WORD PTR [bp - 0Ah]
    jle vga_6770
    add WORD PTR [bp - 0Ah], ax
    add WORD PTR [bp - 12h], ax

vga_6770:
    cmp dx, WORD PTR [bx + 1Ch]
    jle vga_6778
    mov dx, WORD PTR [bx + 1Ch]

vga_6778:
    cmp cx, WORD PTR [bx + 1Eh]
    jle vga_6780
    mov cx, WORD PTR [bx + 1Eh]

vga_6780:
    mov WORD PTR [bp - 8], dx
    mov WORD PTR [bp - 6], cx
    sub dx, WORD PTR [bp - 0Ch]
    jle vga_6790
    sub cx, WORD PTR [bp - 0Ah]
    jg vga_6798

vga_6790:
    pop si
    pop di
    mov sp, bp
    pop bp
    retf 0Eh

vga_6798:
    mov WORD PTR [bp - 16h], cx
    mov WORD PTR [bp - 18h], dx
    add cx, WORD PTR [bp - 12h]
    mov WORD PTR [bp - 0Eh], cx
    add dx, WORD PTR [bp - 14h]
    mov WORD PTR [bp - 10h], dx
    mov si, bp
    mov di, WORD PTR [si + 10h]
    les ax, DWORD PTR [di]
    lea bp, [si - 0Ch]
    call check_cursor_overlap
    les ax, DWORD PTR [bx]
    lea bp, [si - 4]
    call check_cursor_overlap
    mov bp, si
    mov ax, WORD PTR [bp - 0Ah]
    mov dx, WORD PTR [bp - 12h]
    les cx, DWORD PTR [di]
    mov di, WORD PTR [bp - 0Ch]
    mov si, WORD PTR [bp - 14h]
    cmp cx, WORD PTR [bx]
    jne vga_67f9
    mov cx, es
    cmp cx, WORD PTR [bx + 2]
    jne vga_67f9
    cmp di, si
    jle vga_67ec
    mov cx, WORD PTR [bp - 18h]
    dec cx
    add di, cx
    add si, cx
    neg WORD PTR [bp - 4]
    neg WORD PTR [bp - 1Ch]

vga_67ec:
    cmp ax, dx
    jle vga_67f9
    mov cx, WORD PTR [bp - 16h]
    dec cx
    add ax, cx
    add dx, cx
    std

vga_67f9:
    les cx, DWORD PTR [bx]
    add ax, cx
    shl di, 1
    add di, WORD PTR [bx + 0Eh]
    add ax, WORD PTR [di]
    mov WORD PTR [bp - 2], ax
    mov bx, WORD PTR [bp + 10h]
    lds cx, DWORD PTR [bx]
    add dx, cx
    shl si, 1
    add si, WORD PTR ss:[bx + 0Eh]
    add dx, WORD PTR ss:[si]
    mov WORD PTR [bp - 1Ah], dx
    mov bx, WORD PTR [bp + 6]
    and bx, 7
    mov dl, BYTE PTR cs:[bx + vga_copy_dispatch_xor_masks]
    shl bx, 1
    and bl, 7
    mov bx, WORD PTR cs:[bx + vga_copy_dispatch]
    jne vga_6865
    mov bx, OFFSET vga_684d
    pushf
    pop ax
    test ax, 400h
    je vga_6865
    mov bx, OFFSET vga_6840
    jmp SHORT vga_6865

vga_6840:
    dec di
    dec si
    shr cx, 1
    rep movsw
    jae vga_6854
    inc di
    inc si
    movsb
    jmp SHORT vga_6854

vga_684d:
    shr cx, 1
    rep movsw
    jae vga_6854
    movsb

vga_6854:
    dec WORD PTR [bp - 18h]
    je vga_6870
    mov ax, WORD PTR [bp - 4]
    add WORD PTR [bp - 2], ax
    mov ax, WORD PTR [bp - 1Ch]
    add WORD PTR [bp - 1Ah], ax

vga_6865:
    mov cx, WORD PTR [bp - 16h]
    mov di, WORD PTR [bp - 2]
    mov si, WORD PTR [bp - 1Ah]
    jmp bx

vga_6870:
    cld
    push ss
    pop ds
    push cs
    call show_mouse_graphics
    push cs
    call show_mouse_graphics
    jmp vga_6790

vga_687e:
    lodsb
    xor al, dl
    stosb
    loop vga_687e
    jmp SHORT vga_6854

vga_6886:
    lodsb
    xor al, dl
    and al, BYTE PTR es:[di]
    stosb
    loop vga_6886
    jmp SHORT vga_6854

vga_6891:
    lodsb
    xor al, dl
    or al, BYTE PTR es:[di]
    stosb
    loop vga_6891
    jmp SHORT vga_6854

vga_689c:
    lodsb
    xor al, dl
    xor al, BYTE PTR es:[di]
    stosb
    loop vga_689c
    jmp SHORT vga_6854
copy_vga_rectangle ENDP

vga_copy_dispatch LABEL WORD
    DW OFFSET vga_687e, OFFSET vga_6886, OFFSET vga_6891, OFFSET vga_689c
vga_copy_dispatch_xor_masks LABEL BYTE
    DB 0, 0, 0, 0, 0FFh, 0FFh, 0FFh, 0FFh

; Check that this adapter and mode are available.
probe_vga PROC FAR
    mov ax, 1A00h
    int 10h
    cmp al, 1Ah
    jne vga_68df
    xchg bx, ax
    mov bx, 1
    mov cx, 2

vga_68c7:
    cmp al, 7
    je vga_68e1
    cmp al, 8
    je vga_68e1
    cmp al, 0Ah
    je vga_68e1
    cmp al, 0Bh
    je vga_68e1
    cmp al, 0Ch
    xchg al, ah
    loopne vga_68c7
    je vga_68e1

vga_68df:
    sub bx, bx

vga_68e1:
    xchg bx, ax
    retf
probe_vga ENDP

; Set the video mode and build the scanline offset table for the default port.
initialize_vga PROC FAR
    push di
    push si
    push ds
    mov cx, 0Eh
    mov si, OFFSET vga_surface_geometry
    mov di, OFFSET default_port
    push ds
    pop es
    push cs
    pop ds
    rep movsb
    pop ds
    mov ax, WORD PTR [default_port+8]
    shl ax, 1
    push ax
    call __nmalloc
    add sp, 2
    mov WORD PTR [surface_scanline_offsets], ax
    test ax, ax
    je vga_692f
    xchg di, ax
    sub ax, ax
    mov cx, WORD PTR [default_port+8]
    push ds
    pop es

vga_6914:
    stosw
    add ax, WORD PTR [surface_row_stride]
    loop vga_6914
    mov ax, 13h
    int 10h
    mov ax, 1012h
    sub bx, bx
    mov cx, 10h
    push cs
    pop es
    mov dx, OFFSET vga_palette
    int 10h

vga_692f:
    pop si
    pop di
    retf
initialize_vga ENDP

vga_surface_geometry LABEL WORD
    DW 0, 0A000h, 0, 0, 0C8h, 140h, 140h

; Clip to the current port and draw a line in its selected raster operation.
draw_vga_line PROC FAR
    push bp
    mov bp, sp
    add sp, -14h
    push di
    push si
    mov bx, WORD PTR [Graf]
    mov ax, WORD PTR [bx + 6]
    mov cx, WORD PTR [bp + 8]
    sub WORD PTR [bp + 8], ax
    xchg WORD PTR [bx + 24h], cx
    sub cx, ax
    mov WORD PTR [bp - 12h], cx
    mov ax, WORD PTR [bx + 4]
    mov dx, WORD PTR [bp + 6]
    sub WORD PTR [bp + 6], ax
    xchg WORD PTR [bx + 22h], dx
    sub dx, ax
    mov WORD PTR [bp - 14h], dx
    les dx, DWORD PTR [bp + 6]
    mov cx, es
    call classify_vga_line_endpoint
    mov BYTE PTR [bp - 9], al

vga_6979:
    les dx, DWORD PTR [bp - 14h]
    mov cx, es
    call classify_vga_line_endpoint
    mov BYTE PTR [bp - 0Ah], al
    mov ah, BYTE PTR [bp - 9]
    test ah, al
    jne vga_6992
    or ah, al
    jne vga_699a
    jmp vga_6a28

vga_6992:
    pop si
    pop di
    mov sp, bp
    pop bp
    retf 4

vga_699a:
    cmp al, 0
    jne vga_69b0
    xchg BYTE PTR [bp - 9], al
    mov BYTE PTR [bp - 0Ah], al
    xchg WORD PTR [bp + 8], cx
    mov WORD PTR [bp - 12h], cx
    xchg WORD PTR [bp + 6], dx
    mov WORD PTR [bp - 14h], dx

vga_69b0:
    test al, 1
    je vga_69ce
    mov di, WORD PTR [bp + 8]
    sub di, cx
    mov si, WORD PTR [bx + 1Ah]
    mov WORD PTR [bp - 12h], si
    sub si, cx
    mov ax, WORD PTR [bp + 6]
    sub ax, dx
    call scale_vga_line_clip
    add WORD PTR [bp - 14h], cx
    jmp SHORT vga_6979

vga_69ce:
    test al, 2
    je vga_69ec
    mov di, WORD PTR [bp + 6]
    sub di, dx
    mov si, WORD PTR [bx + 18h]
    mov WORD PTR [bp - 14h], si
    sub si, dx
    mov ax, WORD PTR [bp + 8]
    sub ax, cx
    call scale_vga_line_clip
    add WORD PTR [bp - 12h], cx
    jmp SHORT vga_6979

vga_69ec:
    test al, 4
    je vga_6a0c
    mov di, WORD PTR [bp + 8]
    sub di, cx
    mov si, WORD PTR [bx + 1Eh]
    dec si
    mov WORD PTR [bp - 12h], si
    sub si, cx
    mov ax, WORD PTR [bp + 6]
    sub ax, dx
    call scale_vga_line_clip
    add WORD PTR [bp - 14h], cx
    jmp vga_6979

vga_6a0c:
    mov di, WORD PTR [bp + 6]
    sub di, dx
    mov si, WORD PTR [bx + 1Ch]
    dec si
    mov WORD PTR [bp - 14h], si
    sub si, dx
    mov ax, WORD PTR [bp + 8]
    sub ax, cx
    call scale_vga_line_clip
    add WORD PTR [bp - 12h], cx
    jmp vga_6979

vga_6a28:
    cmp cx, WORD PTR [bp + 8]
    jle vga_6a39
    xchg WORD PTR [bp + 8], cx
    mov WORD PTR [bp - 12h], cx
    xchg WORD PTR [bp + 6], dx
    mov WORD PTR [bp - 14h], dx

vga_6a39:
    les di, DWORD PTR [bp + 6]
    mov WORD PTR [bp - 6], cx
    mov WORD PTR [bp - 2], es
    inc WORD PTR [bp - 2]
    mov ax, dx
    cmp ax, di
    jle vga_6a4c
    xchg di, ax

vga_6a4c:
    inc di
    mov WORD PTR [bp - 4], di
    mov WORD PTR [bp - 8], ax
    mov si, WORD PTR [bx + 0Ch]
    mov ax, WORD PTR [bp + 6]
    sub ax, dx
    jge vga_6a61
    neg ax
    neg si

vga_6a61:
    mov WORD PTR [bp - 0Eh], ax
    mov WORD PTR [bp - 10h], si
    mov si, 0FFFFh
    mov di, WORD PTR [bp + 8]
    sub di, cx
    je vga_6a7b
    mov si, di
    cmp si, ax
    jae vga_6a7b
    mov si, ax
    neg si

vga_6a7b:
    sar si, 1
    mov WORD PTR [bp - 0Ch], di
    add ax, di
    inc ax
    xchg cx, ax
    mov di, dx
    shl di, 1
    add di, WORD PTR [bx + 0Eh]
    add ax, WORD PTR [di]
    xchg di, ax
    les ax, DWORD PTR [bx]
    add di, ax
    call check_cursor_overlap
    mov al, BYTE PTR [bx + 28h]
    mov bx, WORD PTR [bx + 26h]
    and bx, 7
    xor al, BYTE PTR cs:[bx + vga_line_dispatch_xor_masks]
    shl bx, 1
    and bl, 7
    mov bx, WORD PTR cs:[bx + vga_line_dispatch]
    jmp bx

vga_6aaf:
    push cs
    call show_mouse_graphics
    jmp vga_6992

vga_6ab6:
    mov BYTE PTR es:[di], al

vga_6ab9:
    dec cx
    jle vga_6aaf
    test si, si
    jl vga_6ac6
    inc di
    sub si, WORD PTR [bp - 0Eh]
    jmp bx

vga_6ac6:
    add di, WORD PTR [bp - 10h]
    add si, WORD PTR [bp - 0Ch]
    jmp bx

vga_6ace:
    mov ah, 0FFh
    xor ah, al
    and BYTE PTR es:[di], ah
    jmp SHORT vga_6ab9

vga_6ad7:
    or BYTE PTR es:[di], al
    jmp SHORT vga_6ab9

vga_6adc:
    xor BYTE PTR es:[di], al
    jmp SHORT vga_6ab9
draw_vga_line ENDP

; Scale the clipped delta using signed multiplication and division.
scale_vga_line_clip PROC NEAR
    imul si
    idiv di
    mov cx, ax
    cwd
    test si, si
    jns vga_6aee
    neg si

vga_6aee:
    idiv si
    sar ax, 1
    sub cx, ax
    ret
scale_vga_line_clip ENDP

; Return the endpoint outcode in AL for the current clip rectangle.
classify_vga_line_endpoint PROC NEAR
    sub al, al
    cmp cx, WORD PTR [bx + 1Ah]
    jge vga_6afe
    or al, 1

vga_6afe:
    cmp dx, WORD PTR [bx + 18h]
    jge vga_6b05
    or al, 2

vga_6b05:
    cmp cx, WORD PTR [bx + 1Eh]
    jl vga_6b0c
    or al, 4

vga_6b0c:
    cmp dx, WORD PTR [bx + 1Ch]
    jl vga_6b13
    or al, 8

vga_6b13:
    ret
classify_vga_line_endpoint ENDP

vga_line_dispatch LABEL WORD
    DW OFFSET vga_6ab6, OFFSET vga_6ace, OFFSET vga_6ad7, OFFSET vga_6adc
vga_line_dispatch_xor_masks LABEL BYTE
    DB 0, 0, 0, 0, 0FFh, 0FFh, 0FFh, 0FFh

; Clip and draw a bitmap into the current port.
draw_vga_bitmap PROC FAR
    push bp
    mov bp, sp
    add sp, -18h
    push di
    push si
    mov bx, WORD PTR [Graf]
    les si, DWORD PTR [bp + 0Eh]
    mov ax, WORD PTR [bp + 0Ch]
    sub ax, WORD PTR [bx + 6]
    mov di, ax
    sub cx, cx
    sub di, WORD PTR [bx + 1Ah]
    jge vga_6b47
    mov ax, WORD PTR [bx + 1Ah]
    sub cx, di

vga_6b47:
    mov WORD PTR [bp - 6], ax
    mov di, ax
    add di, WORD PTR es:[si + 2]
    sub di, cx
    cmp di, WORD PTR [bx + 1Eh]
    jle vga_6b5a
    mov di, WORD PTR [bx + 1Eh]

vga_6b5a:
    mov WORD PTR [bp - 2], di
    sub di, ax
    mov WORD PTR [bp - 0Ah], di
    jle vga_6b94
    mov di, WORD PTR [bp + 0Ah]
    sub di, WORD PTR [bx + 4]
    sub dx, dx
    mov ax, di
    sub ax, WORD PTR [bx + 18h]
    jge vga_6b78
    mov di, WORD PTR [bx + 18h]
    sub dx, ax

vga_6b78:
    mov WORD PTR [bp - 8], di
    mov WORD PTR [bp - 16h], dx
    mov ax, di
    add ax, WORD PTR es:[si]
    sub ax, dx
    cmp ax, WORD PTR [bx + 1Ch]
    jle vga_6b8d
    mov ax, WORD PTR [bx + 1Ch]

vga_6b8d:
    mov WORD PTR [bp - 4], ax
    sub ax, di
    jg vga_6ba1

vga_6b94:
    les ax, DWORD PTR [bp + 0Eh]
    mov dx, es
    pop si
    pop di
    mov sp, bp
    pop bp
    retf 0Ch

vga_6ba1:
    mov WORD PTR [bp - 0Ch], ax
    les ax, DWORD PTR [bx]
    call check_cursor_overlap
    shl di, 1
    add di, WORD PTR [bx + 0Eh]
    add ax, WORD PTR [di]
    add ax, WORD PTR [bp - 6]
    mov WORD PTR [bp - 0Eh], ax
    mov ax, WORD PTR [bx + 0Ch]
    mov WORD PTR [bp - 12h], ax
    mov ds, WORD PTR [bp + 10h]
    mov ax, WORD PTR [si + 4]
    mov WORD PTR [bp - 18h], ax
    mul WORD PTR [bp - 16h]
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
    mov WORD PTR [bp - 14h], dx
    mov ds, dx
    add ax, cx
    add ax, 6
    add ax, si
    mov WORD PTR [bp - 16h], ax
    mov bx, WORD PTR [bp + 8]
    and bx, 7
    mov dl, BYTE PTR [bp + 6]
    xor dl, BYTE PTR cs:[bx + vga_bitmap_dispatch_xor_masks]
    mov dh, dl
    shl bx, 1
    and bl, 7
    mov bx, WORD PTR cs:[bx + vga_bitmap_dispatch]
    jmp SHORT vga_6c24

vga_6c0c:
    push ss
    pop ds
    push cs
    call show_mouse_graphics
    jmp SHORT vga_6b94

vga_6c14:
    lodsb
    test al, al
    je vga_6c1c
    mov BYTE PTR es:[di], dl

vga_6c1c:
    inc di
    loop vga_6c14

vga_6c1f:
    dec WORD PTR [bp - 0Ch]
    je vga_6c0c

vga_6c24:
    mov di, WORD PTR [bp - 0Eh]
    mov ax, WORD PTR [bp - 12h]
    add WORD PTR [bp - 0Eh], ax
    lds si, DWORD PTR [bp - 16h]
    mov cx, WORD PTR [bp - 18h]
    add cx, WORD PTR [bp - 16h]
    mov ax, cx
    and cx, 0Fh
    mov WORD PTR [bp - 16h], cx
    mov cl, 4
    shr ax, cl
    mov cx, ds
    add ax, cx
    mov WORD PTR [bp - 14h], ax
    mov cx, WORD PTR [bp - 0Ah]
    jmp bx

vga_6c4e:
    lodsb
    and al, dl
    not al
    and BYTE PTR es:[di], al
    inc di
    loop vga_6c4e
    jmp SHORT vga_6c1f

vga_6c5b:
    lodsb
    and al, dl
    or BYTE PTR es:[di], al
    inc di
    loop vga_6c5b
    jmp SHORT vga_6c1f

vga_6c66:
    lodsb
    and al, dl
    xor BYTE PTR es:[di], al
    inc di
    loop vga_6c66
    jmp SHORT vga_6c1f
draw_vga_bitmap ENDP

vga_bitmap_dispatch LABEL WORD
    DW OFFSET vga_6c14, OFFSET vga_6c4e, OFFSET vga_6c5b, OFFSET vga_6c66
vga_bitmap_dispatch_xor_masks LABEL BYTE
    DB 0, 0, 0, 0, 0FFh, 0FFh, 0FFh, 0FFh

; This adapter requires no mode-specific shutdown.
vga_mode_exit PROC FAR
    retf
vga_mode_exit ENDP

; Clip and fill the rectangle in the current port.
fill_vga_rectangle PROC FAR
    push bp
    mov bp, sp
    add sp, -0Ch
    push di
    push si
    mov bx, WORD PTR [Graf]
    les si, DWORD PTR [bp + 0Ah]
    mov dx, WORD PTR es:[si + 2]
    sub dx, WORD PTR [bx + 6]
    cmp dx, WORD PTR [bx + 1Ah]
    jge vga_6ca0
    mov dx, WORD PTR [bx + 1Ah]

vga_6ca0:
    mov WORD PTR [bp - 6], dx
    mov ax, WORD PTR es:[si + 6]
    sub ax, WORD PTR [bx + 6]
    cmp ax, WORD PTR [bx + 1Eh]
    jle vga_6cb2
    mov ax, WORD PTR [bx + 1Eh]

vga_6cb2:
    mov WORD PTR [bp - 2], ax
    sub ax, dx
    jle vga_6d17
    mov WORD PTR [bp - 0Ch], ax
    mov di, WORD PTR es:[si]
    sub di, WORD PTR [bx + 4]
    cmp di, WORD PTR [bx + 18h]
    jge vga_6cca
    mov di, WORD PTR [bx + 18h]

vga_6cca:
    mov WORD PTR [bp - 8], di
    mov si, WORD PTR es:[si + 4]
    sub si, WORD PTR [bx + 4]
    cmp si, WORD PTR [bx + 1Ch]
    jle vga_6cdc
    mov si, WORD PTR [bx + 1Ch]

vga_6cdc:
    mov WORD PTR [bp - 4], si
    sub si, di
    jle vga_6d17
    mov ax, WORD PTR [bx + 0Ch]
    mov WORD PTR [bp - 0Ah], ax
    shl di, 1
    add di, WORD PTR [bx + 0Eh]
    add dx, WORD PTR [di]
    les ax, DWORD PTR [bx]
    add dx, ax
    call check_cursor_overlap
    mov bx, WORD PTR [bp + 8]
    and bx, 7
    mov al, BYTE PTR cs:[bx + vga_fill_dispatch_xor_masks]
    xor al, BYTE PTR [bp + 6]
    mov ah, al
    shl bx, 1
    and bl, 7
    mov bx, WORD PTR cs:[bx + vga_fill_dispatch]
    jmp SHORT vga_6d31

vga_6d13:
    push cs
    call show_mouse_graphics

vga_6d17:
    les ax, DWORD PTR [bp + 0Ah]
    mov dx, es
    pop si
    pop di
    mov sp, bp
    pop bp
    retf 8

vga_6d24:
    shr cx, 1
    rep stosw
    jae vga_6d2b
    stosb

vga_6d2b:
    dec si
    je vga_6d13
    add dx, WORD PTR [bp - 0Ah]

vga_6d31:
    mov cx, WORD PTR [bp - 0Ch]
    mov di, dx
    jmp bx

vga_6d38:
    xor BYTE PTR es:[di], al
    inc di
    loop vga_6d38
    jmp SHORT vga_6d2b

vga_6d40:
    or BYTE PTR es:[di], al
    inc di
    loop vga_6d40
    jmp SHORT vga_6d2b

vga_6d48:
    and BYTE PTR es:[di], al
    inc di
    loop vga_6d48
    jmp SHORT vga_6d2b
fill_vga_rectangle ENDP

vga_fill_dispatch LABEL WORD
    DW OFFSET vga_6d24, OFFSET vga_6d48, OFFSET vga_6d40, OFFSET vga_6d38
vga_fill_dispatch_xor_masks LABEL BYTE
    DB 0, 0, 0, 0, 0FFh, 0FFh, 0FFh, 0FFh

; Composite bitmap pixels with the selected raster operation.
composite_vga_bitmap PROC FAR
    push bp
    mov bp, sp
    add sp, -14h
    push di
    push si
    mov bx, WORD PTR [Graf]
    les si, DWORD PTR [bp + 0Ch]
    mov ax, WORD PTR [bp + 0Ah]
    sub ax, WORD PTR [bx + 6]
    sub cx, cx
    mov di, ax
    sub di, WORD PTR [bx + 1Ah]
    jge vga_6d83
    mov ax, WORD PTR [bx + 1Ah]
    sub cx, di

vga_6d83:
    mov WORD PTR [bp - 6], ax
    mov di, ax
    add di, WORD PTR es:[si + 2]
    sub di, cx
    cmp di, WORD PTR [bx + 1Eh]
    jle vga_6d96
    mov di, WORD PTR [bx + 1Eh]

vga_6d96:
    mov WORD PTR [bp - 2], di
    sub di, ax
    mov WORD PTR [bp - 12h], di
    jle vga_6dd0
    mov di, WORD PTR [bp + 8]
    sub di, WORD PTR [bx + 4]
    sub dx, dx
    mov ax, di
    sub ax, WORD PTR [bx + 18h]
    jge vga_6db4
    mov di, WORD PTR [bx + 18h]
    sub dx, ax

vga_6db4:
    mov WORD PTR [bp - 8], di
    mov WORD PTR [bp - 0Eh], dx
    mov ax, di
    add ax, WORD PTR es:[si]
    sub ax, dx
    cmp ax, WORD PTR [bx + 1Ch]
    jle vga_6dc9
    mov ax, WORD PTR [bx + 1Ch]

vga_6dc9:
    mov WORD PTR [bp - 4], ax
    sub ax, di
    jg vga_6ddd

vga_6dd0:
    les ax, DWORD PTR [bp + 0Ch]
    mov dx, es
    pop si
    pop di
    mov sp, bp
    pop bp
    retf 0Ah

vga_6ddd:
    mov WORD PTR [bp - 14h], ax
    les ax, DWORD PTR [bx]
    call check_cursor_overlap
    add ax, WORD PTR [bp - 6]
    shl di, 1
    add di, WORD PTR [bx + 0Eh]
    add ax, WORD PTR [di]
    mov WORD PTR [bp - 0Ah], ax
    mov ax, WORD PTR [bx + 0Ch]
    mov WORD PTR [bp - 0Ch], ax
    mov ds, WORD PTR [bp + 0Eh]
    mov ax, WORD PTR [si + 4]
    mov WORD PTR [bp - 10h], ax
    mul WORD PTR [bp - 0Eh]
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
    add ax, cx
    add ax, 6
    add ax, si
    mov WORD PTR [bp - 0Eh], ax
    mov bx, WORD PTR [bp + 6]
    cmp bx, 8
    je vga_6e93
    cmp bx, 0Ah
    je vga_6e9a
    and bx, 7
    mov dl, BYTE PTR cs:[bx + vga_composite_dispatch_xor_masks]
    mov dh, dl
    shl bx, 1
    and bl, 7
    mov bx, WORD PTR cs:[bx + vga_composite_dispatch]
    jne vga_6e7f
    test dl, dl
    jne vga_6e7f
    mov bx, OFFSET vga_6e55
    jmp SHORT vga_6e7f

vga_6e55:
    shr cx, 1
    rep movsw
    jae vga_6e5c
    movsb

vga_6e5c:
    dec WORD PTR [bp - 14h]
    je vga_6e8a
    mov cx, WORD PTR [bp - 10h]
    add cx, WORD PTR [bp - 0Eh]
    mov ax, cx
    and cx, 0Fh
    mov WORD PTR [bp - 0Eh], cx
    mov cl, 4
    shr ax, cl
    mov cx, ds
    add ax, cx
    mov ds, ax
    mov ax, WORD PTR [bp - 0Ch]
    add WORD PTR [bp - 0Ah], ax

vga_6e7f:
    mov cx, WORD PTR [bp - 12h]
    mov di, WORD PTR [bp - 0Ah]
    mov si, WORD PTR [bp - 0Eh]
    jmp bx

vga_6e8a:
    push ss
    pop ds
    push cs
    call show_mouse_graphics
    jmp vga_6dd0

vga_6e93:
    mov bx, OFFSET vga_6ed6
    sub dl, dl
    jmp SHORT vga_6e7f

vga_6e9a:
    mov bx, OFFSET vga_6ee4
    jmp SHORT vga_6e7f

vga_6e9f:
    shr cx, 1
    je vga_6eab
    pushf

vga_6ea4:
    lodsw
    xor ax, dx
    stosw
    loop vga_6ea4
    popf

vga_6eab:
    jae vga_6e5c
    lodsb
    xor al, dl
    stosb
    jmp SHORT vga_6e5c

vga_6eb3:
    lodsb
    xor al, dl
    not ah
    and BYTE PTR es:[di], al
    inc di
    loop vga_6eb3
    jmp SHORT vga_6e5c

vga_6ec0:
    lodsb
    xor al, dl
    or BYTE PTR es:[di], al
    inc di
    loop vga_6ec0
    jmp SHORT vga_6e5c

vga_6ecb:
    lodsb
    xor al, dl
    xor BYTE PTR es:[di], al
    inc di
    loop vga_6ecb
    jmp SHORT vga_6e5c

vga_6ed6:
    lodsb
    test al, al
    je vga_6ede
    mov BYTE PTR es:[di], dl

vga_6ede:
    inc di
    loop vga_6ed6
    jmp vga_6e5c

vga_6ee4:
    shr cx, 1
    jae vga_6ef3
    lodsb
    test al, al
    je vga_6ef0
    mov BYTE PTR es:[di], al

vga_6ef0:
    inc di
    jcxz vga_6f0b

vga_6ef3:
    lodsw
    xchg al, ah
    test ah, ah
    je vga_6efd
    mov BYTE PTR es:[di], ah

vga_6efd:
    inc di
    test al, al
    je vga_6f08
    stosb
    loop vga_6ef3
    jmp vga_6e5c

vga_6f08:
    inc di
    loop vga_6ef3

vga_6f0b:
    jmp vga_6e5c
composite_vga_bitmap ENDP

vga_composite_dispatch LABEL WORD
    DW OFFSET vga_6e9f, OFFSET vga_6eb3, OFFSET vga_6ec0, OFFSET vga_6ecb
vga_composite_dispatch_xor_masks LABEL BYTE
    DB 0, 0, 0, 0, 0FFh, 0FFh, 0FFh, 0FFh
END

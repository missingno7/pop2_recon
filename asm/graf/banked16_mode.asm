; DOS banked 16-color adapter hooks and scanline raster operations.
.MODEL MEDIUM
.DATA
EXTRN near_heap_segment:WORD
EXTRN Graf:WORD
EXTRN default_port:BYTE
EXTRN surface_scanline_count:WORD
EXTRN surface_row_stride:WORD
EXTRN surface_scanline_offsets:WORD
.CODE
EXTRN show_mouse_graphics:NEAR
EXTRN check_cursor_overlap:NEAR
EXTRN __nmalloc:FAR

PUBLIC copy_banked16_rectangle
PUBLIC probe_banked16
PUBLIC initialize_banked16
PUBLIC draw_banked16_line
PUBLIC scale_banked16_line_clip
PUBLIC classify_banked16_line_endpoint
PUBLIC draw_banked16_bitmap
PUBLIC banked16_mode_exit
PUBLIC fill_banked16_rectangle
PUBLIC composite_banked16_bitmap
PUBLIC banked16_palette
PUBLIC banked16_mode_descriptor
PUBLIC banked16_copy_dispatch
PUBLIC banked16_surface_geometry
PUBLIC banked16_line_dispatch
PUBLIC banked16_bitmap_dispatch
PUBLIC banked16_fill_dispatch
PUBLIC banked16_transparency_masks
PUBLIC banked16_composite_dispatch
PUBLIC banked16_boundary_alignment

banked16_palette LABEL BYTE
    DB 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 0Ah, 0Bh
    DB 0Ch, 0, 0Eh, 0Fh, 0

banked16_mode_descriptor LABEL WORD
    DW 3, 140h, 10h, 102h
    DW OFFSET probe_banked16, OFFSET initialize_banked16, OFFSET banked16_mode_exit, 0, 0, 0
    DW OFFSET copy_banked16_rectangle, OFFSET draw_banked16_line, OFFSET draw_banked16_bitmap, OFFSET fill_banked16_rectangle, OFFSET composite_banked16_bitmap

; Clip both rectangles, then copy scanlines with the selected raster operation.
copy_banked16_rectangle PROC FAR
    push bp
    mov bp, sp
    add sp, -24h
    push di
    push si
    mov bx, WORD PTR [bp + 10h]
    les si, DWORD PTR [bp + 8]
    sub dx, dx
    lods WORD PTR es:[si]
    sub ax, WORD PTR [bx + 4]
    jge banked16_548b
    xchg dx, ax

banked16_548b:
    mov WORD PTR [bp - 16h], ax
    sub cx, cx
    lods WORD PTR es:[si]
    sub ax, WORD PTR [bx + 6]
    jge banked16_5498
    xchg cx, ax

banked16_5498:
    mov WORD PTR [bp - 14h], ax
    sub di, di
    lods WORD PTR es:[si]
    sub ax, WORD PTR [bx + 8]
    jle banked16_54a5
    xchg di, ax

banked16_54a5:
    mov WORD PTR [bp - 12h], di
    sub di, di
    lods WORD PTR es:[si]
    sub ax, WORD PTR [bx + 0Ah]
    jle banked16_54b2
    xchg di, ax

banked16_54b2:
    mov WORD PTR [bp - 10h], di
    mov bx, WORD PTR [bp + 12h]
    les si, DWORD PTR [bp + 0Ch]
    lods WORD PTR es:[si]
    sub ax, dx
    sub ax, WORD PTR [bx + 4]
    jge banked16_54c9
    sub WORD PTR [bp - 16h], ax
    sub ax, ax

banked16_54c9:
    xchg dx, ax
    lods WORD PTR es:[si]
    sub ax, cx
    sub ax, WORD PTR [bx + 6]
    jge banked16_54d8
    sub WORD PTR [bp - 14h], ax
    sub ax, ax

banked16_54d8:
    xchg cx, ax
    lods WORD PTR es:[si]
    sub ax, WORD PTR [bp - 12h]
    cmp ax, WORD PTR [bx + 8]
    jle banked16_54e6
    mov ax, WORD PTR [bx + 8]

banked16_54e6:
    sub ax, WORD PTR [bx + 4]
    mov WORD PTR [bp - 0Ah], ax
    lods WORD PTR es:[si]
    sub ax, WORD PTR [bp - 10h]
    cmp ax, WORD PTR [bx + 0Ah]
    jle banked16_54f9
    mov ax, WORD PTR [bx + 0Ah]

banked16_54f9:
    sub ax, WORD PTR [bx + 6]
    mov WORD PTR [bp - 8], ax
    cmp bx, WORD PTR [Graf]
    jne banked16_5533
    mov ax, WORD PTR [bx + 18h]
    sub ax, dx
    jle banked16_5511
    add dx, ax
    add WORD PTR [bp - 16h], ax

banked16_5511:
    mov ax, WORD PTR [bx + 1Ah]
    sub ax, cx
    jle banked16_551d
    add cx, ax
    add WORD PTR [bp - 14h], ax

banked16_551d:
    mov ax, WORD PTR [bx + 1Ch]
    sub ax, WORD PTR [bp - 0Ah]
    jge banked16_5528
    add WORD PTR [bp - 0Ah], ax

banked16_5528:
    mov ax, WORD PTR [bx + 1Eh]
    sub ax, WORD PTR [bp - 8]
    jge banked16_5533
    add WORD PTR [bp - 8], ax

banked16_5533:
    mov WORD PTR [bp - 0Ch], cx
    mov WORD PTR [bp - 0Eh], dx
    mov ax, WORD PTR [bp - 0Ah]
    sub ax, dx
    mov WORD PTR [bp - 20h], ax
    jle banked16_5563
    mov ax, 8
    sub cx, WORD PTR [bp - 14h]
    shr cx, 1
    jae banked16_5550
    mov ax, 404h

banked16_5550:
    mov WORD PTR [bp - 1Ch], ax
    mov dx, 0FFFFh
    mov di, WORD PTR [bp - 8]
    mov ax, WORD PTR [bp - 0Ch]
    sub di, ax
    mov WORD PTR [bp - 1Eh], di
    jg banked16_556b

banked16_5563:
    pop si
    pop di
    mov sp, bp
    pop bp
    retf 0Eh

banked16_556b:
    and ax, 1
    jne banked16_5574
    mov al, 2
    jmp SHORT banked16_5576

banked16_5574:
    mov dl, 0Fh

banked16_5576:
    sub di, ax
    jle banked16_5583
    shr di, 1
    jae banked16_558d
    mov dh, 0F0h
    inc di
    jmp SHORT banked16_558d

banked16_5583:
    shr di, 1
    jae banked16_5589
    mov dh, 0F0h

banked16_5589:
    and dl, dh
    sub di, di

banked16_558d:
    mov WORD PTR [bp - 2], di
    mov WORD PTR [bp - 18h], dx
    les ax, DWORD PTR [bp - 20h]
    add ax, WORD PTR [bp - 16h]
    mov WORD PTR [bp - 12h], ax
    mov ax, es
    add ax, WORD PTR [bp - 14h]
    mov WORD PTR [bp - 10h], ax
    mov si, bp
    les ax, DWORD PTR [bx]
    lea bp, [si - 6]
    call check_cursor_overlap
    mov di, WORD PTR [si + 10h]
    les ax, DWORD PTR [di]
    lea bp, [si - 0Eh]
    call check_cursor_overlap
    mov bp, si
    mov WORD PTR [bp - 1Ah], OFFSET banked16_copy_dispatch
    mov WORD PTR [bp - 24h], 1
    mov dx, WORD PTR [bp - 14h]
    mov di, dx
    cmp ax, WORD PTR [bx]
    mov ax, WORD PTR [bp - 0Ch]
    jne banked16_5617
    mov cx, es
    cmp cx, WORD PTR [bx + 2]
    jne banked16_5617
    mov si, WORD PTR [bp - 16h]
    cmp si, WORD PTR [bp - 0Eh]
    jge banked16_55ed
    mov cx, WORD PTR [bp - 20h]
    dec cx
    add WORD PTR [bp - 0Eh], cx
    add WORD PTR [bp - 16h], cx
    neg WORD PTR [bp - 24h]

banked16_55ed:
    cmp dx, ax
    jg banked16_5617
    mov cx, WORD PTR [bp - 1Eh]
    dec cx
    add ax, cx
    add dx, cx
    mov di, dx
    neg di
    std
    mov cx, WORD PTR [bp - 1Ch]
    test ch, ch
    je banked16_560a
    xchg cl, ch
    mov WORD PTR [bp - 1Ch], cx

banked16_560a:
    mov cx, WORD PTR [bp - 18h]
    xchg cl, ch
    mov WORD PTR [bp - 18h], cx
    mov WORD PTR [bp - 1Ah], OFFSET banked16_copy_dispatch_reverse

banked16_5617:
    and di, 1
    shl di, 1
    shl di, 1
    mov WORD PTR [bp - 4], di
    shr ax, 1
    shr dx, 1
    les cx, DWORD PTR [bx]
    add ax, cx
    mov WORD PTR [bp - 0Ch], ax
    mov ax, WORD PTR [bx + 0Eh]
    mov WORD PTR [bp - 6], ax
    mov bx, WORD PTR [bp + 10h]
    mov ax, WORD PTR [bx + 0Eh]
    mov WORD PTR [bp - 22h], ax
    lds cx, DWORD PTR [bx]
    add dx, cx
    mov WORD PTR [bp - 14h], dx
    mov cx, WORD PTR [bp - 1Ch]
    mov bx, WORD PTR [bp + 6]
    and bx, 7
    mov dl, BYTE PTR cs:[bx + banked16_copy_dispatch_xor_masks]
    and bx, 3
    shl bx, 1
    add bx, WORD PTR [bp - 1Ah]
    mov ax, WORD PTR cs:[bx]
    mov WORD PTR [bp - 1Ah], ax
    jmp SHORT banked16_56cb

banked16_5660:
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
    jl banked16_56bd
    je banked16_56a9
    test ch, ch
    jne banked16_5698
    xchg bx, cx
    test dl, dl
    jne banked16_568e
    shr cx, 1
    rep movsw
    xchg bx, cx
    jae banked16_56a9
    movsb
    jmp SHORT banked16_56a9

banked16_568e:
    lodsb
    xor al, dl
    stosb
    loop banked16_568e
    xchg bx, cx
    jmp SHORT banked16_56a9

banked16_5698:
    lodsb
    xor al, dl
    xchg al, ah
    rol ax, cl
    stosb
    xchg cl, ch
    rol ah, cl
    xchg cl, ch
    dec bx
    jg banked16_5698

banked16_56a9:
    lodsb
    xor al, dl
    xchg al, ah
    rol ax, cl
    mov dh, BYTE PTR [bp - 17h]
    and al, dh
    not dh
    and dh, BYTE PTR es:[di]
    or al, dh
    stosb

banked16_56bd:
    dec WORD PTR [bp - 20h]
    je banked16_56ff
    mov ax, WORD PTR [bp - 24h]
    add WORD PTR [bp - 0Eh], ax
    add WORD PTR [bp - 16h], ax

banked16_56cb:
    mov di, WORD PTR [bp - 0Eh]
    shl di, 1
    add di, WORD PTR [bp - 6]
    mov di, WORD PTR ss:[di]
    add di, WORD PTR [bp - 0Ch]
    mov si, WORD PTR [bp - 16h]
    shl si, 1
    add si, WORD PTR [bp - 22h]
    mov si, WORD PTR ss:[si]
    add si, WORD PTR [bp - 14h]
    mov bx, WORD PTR [bp - 2]
    mov dh, BYTE PTR [bp - 18h]
    sub ah, ah
    lodsb
    xor al, dl
    cmp cl, BYTE PTR [bp - 4]
    ja banked16_56fc
    mov ah, al
    lodsb
    xor al, dl

banked16_56fc:
    jmp WORD PTR [bp - 1Ah]

banked16_56ff:
    cld
    push ss
    pop ds
    push cs
    call show_mouse_graphics
    push cs
    call show_mouse_graphics
    jmp banked16_5563

banked16_570d:
    shl ax, cl
    and ah, dh
    not ah
    and BYTE PTR es:[di], ah
    inc di
    xchg cl, ch
    shl ax, cl
    xchg cl, ch
    dec bx
    jl banked16_5758
    je banked16_574b
    test ch, ch
    jne banked16_5737
    xchg bx, cx

banked16_5728:
    lodsb
    xor al, dl
    not al
    and BYTE PTR es:[di], al
    inc di
    loop banked16_5728
    xchg bx, cx
    jmp SHORT banked16_574b

banked16_5737:
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
    jg banked16_5737

banked16_574b:
    lodsb
    xor al, dl
    shl ax, cl
    and ah, BYTE PTR [bp - 17h]
    not ah
    and BYTE PTR es:[di], ah

banked16_5758:
    jmp banked16_56bd

banked16_575b:
    shl ax, cl
    and ah, dh
    or BYTE PTR es:[di], ah
    inc di
    xchg cl, ch
    shl ax, cl
    xchg cl, ch
    dec bx
    jl banked16_579e
    je banked16_5793
    test ch, ch
    jne banked16_5781
    xchg bx, cx

banked16_5774:
    lodsb
    xor al, dl
    or BYTE PTR es:[di], al
    inc di
    loop banked16_5774
    xchg bx, cx
    jmp SHORT banked16_5793

banked16_5781:
    lodsb
    xor al, dl
    shl ax, cl
    or BYTE PTR es:[di], ah
    inc di
    xchg cl, ch
    shl ax, cl
    xchg cl, ch
    dec bx
    jg banked16_5781

banked16_5793:
    lodsb
    xor al, dl
    shl ax, cl
    and ah, BYTE PTR [bp - 17h]
    or BYTE PTR es:[di], ah

banked16_579e:
    jmp banked16_56bd

banked16_57a1:
    shl ax, cl
    and ah, dh
    xor BYTE PTR es:[di], ah
    inc di
    xchg cl, ch
    shl ax, cl
    xchg cl, ch
    dec bx
    jl banked16_57e4
    je banked16_57d9
    test ch, ch
    jne banked16_57c7
    xchg bx, cx

banked16_57ba:
    lodsb
    xor al, dl
    xor BYTE PTR es:[di], al
    inc di
    loop banked16_57ba
    xchg bx, cx
    jmp SHORT banked16_57d9

banked16_57c7:
    lodsb
    xor al, dl
    shl ax, cl
    xor BYTE PTR es:[di], ah
    inc di
    xchg cl, ch
    shl ax, cl
    xchg cl, ch
    dec bx
    jg banked16_57c7

banked16_57d9:
    lodsb
    xor al, dl
    shl ax, cl
    and ah, BYTE PTR [bp - 17h]
    xor BYTE PTR es:[di], ah

banked16_57e4:
    jmp banked16_56bd

banked16_57e7:
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
    jl banked16_5848
    je banked16_5834
    test ch, ch
    jne banked16_5823
    xchg bx, cx
    test dl, dl
    jne banked16_5819
    dec di
    dec si
    shr cx, 1
    rep movsw
    inc di
    inc si
    xchg bx, cx
    jae banked16_5834
    movsb
    jmp SHORT banked16_5834

banked16_5819:
    lodsb
    xor al, dl
    stosb
    loop banked16_5819
    xchg bx, cx
    jmp SHORT banked16_5834

banked16_5823:
    lodsb
    xor al, dl
    xchg al, ah
    ror ax, cl
    stosb
    xchg cl, ch
    ror ah, cl
    xchg cl, ch
    dec bx
    jg banked16_5823

banked16_5834:
    lodsb
    xor al, dl
    xchg al, ah
    ror ax, cl
    mov dh, BYTE PTR [bp - 17h]
    and al, dh
    not dh
    and dh, BYTE PTR es:[di]
    or al, dh
    stosb

banked16_5848:
    jmp banked16_56bd

banked16_584b:
    ror ax, cl
    and ah, dh
    not ah
    and BYTE PTR es:[di], ah
    dec di
    xchg cl, ch
    ror ax, cl
    xchg cl, ch
    dec bx
    jl banked16_5896
    je banked16_5889
    test cl, cl
    jne banked16_5875
    xchg bx, cx

banked16_5866:
    lodsb
    xor al, dl
    not al
    and BYTE PTR es:[di], al
    dec di
    loop banked16_5866
    xchg bx, cx
    jmp SHORT banked16_5889

banked16_5875:
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
    jg banked16_5875

banked16_5889:
    lodsb
    xor al, dl
    ror ax, cl
    and ah, BYTE PTR [bp - 17h]
    not ah
    and BYTE PTR es:[di], ah

banked16_5896:
    jmp banked16_56bd

banked16_5899:
    ror ax, cl
    and ah, dh
    or BYTE PTR es:[di], ah
    dec di
    xchg cl, ch
    ror ax, cl
    xchg cl, ch
    dec bx
    jl banked16_58dc
    je banked16_58d1
    test cl, cl
    jne banked16_58bf
    xchg bx, cx

banked16_58b2:
    lodsb
    xor al, dl
    or BYTE PTR es:[di], al
    dec di
    loop banked16_58b2
    xchg bx, cx
    jmp SHORT banked16_58d1

banked16_58bf:
    lodsb
    xor al, dl
    ror ax, cl
    or BYTE PTR es:[di], ah
    dec di
    xchg cl, ch
    ror ax, cl
    xchg cl, ch
    dec bx
    jg banked16_58bf

banked16_58d1:
    lodsb
    xor al, dl
    ror ax, cl
    and ah, BYTE PTR [bp - 17h]
    or BYTE PTR es:[di], ah

banked16_58dc:
    jmp banked16_56bd

banked16_58df:
    ror ax, cl
    and ah, dh
    xor BYTE PTR es:[di], ah
    dec di
    xchg cl, ch
    ror ax, cl
    xchg cl, ch
    dec bx
    jl banked16_5922
    je banked16_5917
    test cl, cl
    jne banked16_5905
    xchg bx, cx

banked16_58f8:
    lodsb
    xor al, dl
    xor BYTE PTR es:[di], al
    dec di
    loop banked16_58f8
    xchg bx, cx
    jmp SHORT banked16_5917

banked16_5905:
    lodsb
    xor al, dl
    ror ax, cl
    xor BYTE PTR es:[di], ah
    dec di
    xchg cl, ch
    ror ax, cl
    xchg cl, ch
    dec bx
    jg banked16_5905

banked16_5917:
    lodsb
    xor al, dl
    ror ax, cl
    and ah, BYTE PTR [bp - 17h]
    xor BYTE PTR es:[di], ah

banked16_5922:
    jmp banked16_56bd
copy_banked16_rectangle ENDP

banked16_copy_dispatch LABEL WORD
    DW OFFSET banked16_5660, OFFSET banked16_570d, OFFSET banked16_575b, OFFSET banked16_57a1
banked16_copy_dispatch_reverse LABEL WORD
    DW OFFSET banked16_57e7, OFFSET banked16_584b, OFFSET banked16_5899, OFFSET banked16_58df
banked16_copy_dispatch_xor_masks LABEL BYTE
    DB 0, 0, 0, 0, 0FFh, 0FFh, 0FFh, 0FFh

; Check that this adapter and mode are available.
probe_banked16 PROC FAR
    mov ax, 0FC00h
    mov es, ax
    mov ax, 1
    cmp BYTE PTR es:[3FFEh], 0FFh
    jne banked16_5955
    cmp BYTE PTR es:[0], 21h
    je banked16_5957

banked16_5955:
    sub ax, ax

banked16_5957:
    retf
probe_banked16 ENDP

; Set the video mode and build the scanline offset table for the default port.
initialize_banked16 PROC FAR
    push di
    push si
    mov dx, 400h
    mov ax, WORD PTR [near_heap_segment]
    mov bx, ax
    dec ax

banked16_5963:
    mov es, ax
    mov cx, WORD PTR es:[3]
    cmp BYTE PTR es:[0], 5Ah
    je banked16_5977
    inc ax
    add ax, cx
    jmp SHORT banked16_5963

banked16_5977:
    cmp WORD PTR es:[1], 0
    jne banked16_59a5
    sub cx, dx
    jb banked16_59a5
    je banked16_5993
    add ax, cx
    dec cx
    mov BYTE PTR es:[0], 4Dh
    mov WORD PTR es:[3], cx

banked16_5993:
    mov es, ax
    mov BYTE PTR es:[0], 5Ah
    mov WORD PTR es:[1], bx
    mov WORD PTR es:[3], dx

banked16_59a5:
    push ds
    mov cx, 0Eh
    mov si, OFFSET banked16_surface_geometry
    mov di, OFFSET default_port
    push ds
    pop es
    push cs
    pop ds
    rep movsb
    pop ds
    mov ax, WORD PTR [surface_scanline_count]
    shl ax, 1
    push ax
    call __nmalloc
    add sp, 2
    mov WORD PTR [surface_scanline_offsets], ax
    test ax, ax
    jne banked16_59ce

banked16_59cb:
    pop si
    pop di
    retf

banked16_59ce:
    xchg di, ax
    sub si, si
    mov cx, WORD PTR [surface_scanline_count]

banked16_59d5:
    mov ax, si
    shr ax, 1
    shr ax, 1
    mul WORD PTR [surface_row_stride]
    mov WORD PTR [di], ax
    mov ax, si
    and ax, 3
    mov dx, 2000h
    mul dx
    add ax, WORD PTR [di]
    stosw
    inc si
    loop banked16_59d5
    mov ax, 9
    int 10h
    mov ax, 1002h
    push cs
    pop es
    mov dx, OFFSET banked16_palette
    int 10h
    jmp SHORT banked16_59cb
initialize_banked16 ENDP

banked16_surface_geometry LABEL WORD
    DW 0, 0B800h, 0, 0, 0C8h, 140h, 0A0h

; Clip to the current port and draw a line in its selected raster operation.
draw_banked16_line PROC FAR
    push bp
    mov bp, sp
    add sp, -1Ah
    push di
    push si
    mov bx, WORD PTR [Graf]
    mov ax, WORD PTR [bx + 28h]
    and al, 0Fh
    mov ah, al
    mov cl, 4
    shl ah, cl
    or al, ah
    mov di, WORD PTR [bx + 26h]
    mov dx, 7
    and di, dx
    xor al, BYTE PTR cs:[di + banked16_line_dispatch_xor_masks]
    mov BYTE PTR [bp - 0Ch], al
    shl di, 1
    and di, dx
    mov ax, WORD PTR cs:[di + banked16_line_dispatch]
    mov WORD PTR [bp - 16h], ax
    mov ax, WORD PTR [bx + 6]
    mov cx, WORD PTR [bp + 8]
    sub WORD PTR [bp + 8], ax
    xchg WORD PTR [bx + 24h], cx
    sub cx, ax
    mov WORD PTR [bp - 18h], cx
    mov ax, WORD PTR [bx + 4]
    mov dx, WORD PTR [bp + 6]
    sub WORD PTR [bp + 6], ax
    xchg WORD PTR [bx + 22h], dx
    sub dx, ax
    mov WORD PTR [bp - 1Ah], dx
    les dx, DWORD PTR [bp + 6]
    mov cx, es
    call classify_banked16_line_endpoint
    mov BYTE PTR [bp - 9], al

banked16_5a72:
    les dx, DWORD PTR [bp - 1Ah]
    mov cx, es
    call classify_banked16_line_endpoint
    mov BYTE PTR [bp - 0Ah], al
    mov ah, BYTE PTR [bp - 9]
    test ah, al
    jne banked16_5a8b
    or ah, al
    jne banked16_5a93
    jmp banked16_5b21

banked16_5a8b:
    pop si
    pop di
    mov sp, bp
    pop bp
    retf 4

banked16_5a93:
    cmp al, 0
    jne banked16_5aa9
    xchg BYTE PTR [bp - 9], al
    mov BYTE PTR [bp - 0Ah], al
    xchg WORD PTR [bp + 8], cx
    mov WORD PTR [bp - 18h], cx
    xchg WORD PTR [bp + 6], dx
    mov WORD PTR [bp - 1Ah], dx

banked16_5aa9:
    test al, 1
    je banked16_5ac7
    mov di, WORD PTR [bp + 8]
    sub di, cx
    mov si, WORD PTR [bx + 1Ah]
    mov WORD PTR [bp - 18h], si
    sub si, cx
    mov ax, WORD PTR [bp + 6]
    sub ax, dx
    call scale_banked16_line_clip
    add WORD PTR [bp - 1Ah], cx
    jmp SHORT banked16_5a72

banked16_5ac7:
    test al, 2
    je banked16_5ae5
    mov di, WORD PTR [bp + 6]
    sub di, dx
    mov si, WORD PTR [bx + 18h]
    mov WORD PTR [bp - 1Ah], si
    sub si, dx
    mov ax, WORD PTR [bp + 8]
    sub ax, cx
    call scale_banked16_line_clip
    add WORD PTR [bp - 18h], cx
    jmp SHORT banked16_5a72

banked16_5ae5:
    test al, 4
    je banked16_5b05
    mov di, WORD PTR [bp + 8]
    sub di, cx
    mov si, WORD PTR [bx + 1Eh]
    dec si
    mov WORD PTR [bp - 18h], si
    sub si, cx
    mov ax, WORD PTR [bp + 6]
    sub ax, dx
    call scale_banked16_line_clip
    add WORD PTR [bp - 1Ah], cx
    jmp banked16_5a72

banked16_5b05:
    mov di, WORD PTR [bp + 6]
    sub di, dx
    mov si, WORD PTR [bx + 1Ch]
    dec si
    mov WORD PTR [bp - 1Ah], si
    sub si, dx
    mov ax, WORD PTR [bp + 8]
    sub ax, cx
    call scale_banked16_line_clip
    add WORD PTR [bp - 18h], cx
    jmp banked16_5a72

banked16_5b21:
    cmp cx, WORD PTR [bp + 8]
    jle banked16_5b32
    xchg WORD PTR [bp + 8], cx
    mov WORD PTR [bp - 18h], cx
    xchg WORD PTR [bp + 6], dx
    mov WORD PTR [bp - 1Ah], dx

banked16_5b32:
    les di, DWORD PTR [bp + 6]
    mov WORD PTR [bp - 6], cx
    mov WORD PTR [bp - 2], es
    inc WORD PTR [bp - 2]
    mov ax, dx
    cmp ax, di
    jle banked16_5b45
    xchg di, ax

banked16_5b45:
    inc di
    mov WORD PTR [bp - 4], di
    mov WORD PTR [bp - 8], ax
    mov si, 1
    mov ax, WORD PTR [bp + 6]
    sub ax, dx
    jge banked16_5b5a
    neg ax
    neg si

banked16_5b5a:
    mov WORD PTR [bp - 10h], ax
    mov dx, si
    mov si, 0FFFFh
    mov di, WORD PTR [bp + 8]
    sub di, cx
    je banked16_5b73
    mov si, di
    cmp si, ax
    jae banked16_5b73
    mov si, ax
    neg si

banked16_5b73:
    sar si, 1
    mov WORD PTR [bp - 0Eh], di
    add di, ax
    inc di
    xchg cx, di
    les ax, DWORD PTR [bx]
    call check_cursor_overlap
    mov bx, WORD PTR [bx + 0Eh]
    mov WORD PTR [bp - 14h], bx
    mov bl, 0F0h
    mov bh, BYTE PTR [bp - 0Ch]
    shr di, 1
    jae banked16_5b93
    not bl

banked16_5b93:
    add ax, di
    mov WORD PTR [bp - 12h], ax
    jmp SHORT banked16_5bab

banked16_5b9a:
    mov ah, bh
    and ah, al
    not al
    and al, BYTE PTR es:[di]
    or al, ah
    stosb
    sub al, al
    dec cx
    jle banked16_5bd9

banked16_5bab:
    sub al, al
    mov di, WORD PTR [bp - 1Ah]
    shl di, 1
    add di, WORD PTR [bp - 14h]
    mov di, WORD PTR [di]
    add di, WORD PTR [bp - 12h]

banked16_5bba:
    or al, bl
    test si, si
    jl banked16_5bd0
    sub si, WORD PTR [bp - 10h]
    xor bl, 0FFh
    js banked16_5bca
    loop banked16_5bba

banked16_5bca:
    inc WORD PTR [bp - 12h]
    jmp WORD PTR [bp - 16h]

banked16_5bd0:
    add WORD PTR [bp - 1Ah], dx
    add si, WORD PTR [bp - 0Eh]
    jmp WORD PTR [bp - 16h]

banked16_5bd9:
    push cs
    call show_mouse_graphics
    jmp banked16_5a8b

banked16_5be0:
    and al, bh
    not al
    and BYTE PTR es:[di], al
    dec cx
    jg banked16_5bab
    jmp SHORT banked16_5bd9

banked16_5bec:
    and al, bh
    or BYTE PTR es:[di], al
    dec cx
    jg banked16_5bab
    jmp SHORT banked16_5bd9

banked16_5bf6:
    and al, bh
    xor BYTE PTR es:[di], al
    dec cx
    jg banked16_5bab
    jmp SHORT banked16_5bd9
draw_banked16_line ENDP

; Scale the clipped delta using signed multiplication and division.
scale_banked16_line_clip PROC NEAR
    imul si
    idiv di
    mov cx, ax
    cwd
    test si, si
    jns banked16_5c0d
    neg si

banked16_5c0d:
    idiv si
    sar ax, 1
    sub cx, ax
    ret
scale_banked16_line_clip ENDP

; Return the endpoint outcode in AL for the current clip rectangle.
classify_banked16_line_endpoint PROC NEAR
    sub al, al
    cmp cx, WORD PTR [bx + 1Ah]
    jge banked16_5c1d
    or al, 1

banked16_5c1d:
    cmp dx, WORD PTR [bx + 18h]
    jge banked16_5c24
    or al, 2

banked16_5c24:
    cmp cx, WORD PTR [bx + 1Eh]
    jl banked16_5c2b
    or al, 4

banked16_5c2b:
    cmp dx, WORD PTR [bx + 1Ch]
    jl banked16_5c32
    or al, 8

banked16_5c32:
    ret
classify_banked16_line_endpoint ENDP

banked16_line_dispatch LABEL WORD
    DW OFFSET banked16_5b9a, OFFSET banked16_5be0, OFFSET banked16_5bec, OFFSET banked16_5bf6
banked16_line_dispatch_xor_masks LABEL BYTE
    DB 0, 0, 0, 0, 0FFh, 0FFh, 0FFh, 0FFh

; Clip and draw a bitmap into the current port.
draw_banked16_bitmap PROC FAR
    push bp
    mov bp, sp
    add sp, -20h
    push di
    push si
    mov bx, WORD PTR [Graf]
    les si, DWORD PTR [bp + 0Eh]
    mov ax, WORD PTR [bp + 0Ch]
    sub ax, WORD PTR [bx + 6]
    sub cx, cx
    mov di, ax
    sub di, WORD PTR [bx + 1Ah]
    jge banked16_5c66
    mov ax, WORD PTR [bx + 1Ah]
    sub cx, di

banked16_5c66:
    mov WORD PTR [bp - 6], ax
    mov WORD PTR [bp - 10h], cx
    mov di, ax
    add di, WORD PTR es:[si + 2]
    sub di, cx
    cmp di, WORD PTR [bx + 1Eh]
    jle banked16_5c7c
    mov di, WORD PTR [bx + 1Eh]

banked16_5c7c:
    mov WORD PTR [bp - 2], di
    sub di, ax
    mov WORD PTR [bp - 0Ch], di
    jle banked16_5cb9
    mov ax, WORD PTR [bp + 0Ah]
    sub ax, WORD PTR [bx + 4]
    sub cx, cx
    mov di, ax
    sub di, WORD PTR [bx + 18h]
    jge banked16_5c9a
    mov ax, WORD PTR [bx + 18h]
    sub cx, di

banked16_5c9a:
    mov WORD PTR [bp - 8], ax
    mov WORD PTR [bp - 1Ah], cx
    mov di, ax
    add di, WORD PTR es:[si]
    sub di, cx
    cmp di, WORD PTR [bx + 1Ch]
    jle banked16_5caf
    mov di, WORD PTR [bx + 1Ch]

banked16_5caf:
    mov WORD PTR [bp - 4], di
    sub di, ax
    mov WORD PTR [bp - 0Eh], di
    jg banked16_5cc6

banked16_5cb9:
    les ax, DWORD PTR [bp + 0Eh]
    mov dx, es
    pop si
    pop di
    mov sp, bp
    pop bp
    retf 0Ch

banked16_5cc6:
    mov ax, 8
    mov dx, WORD PTR [bp - 6]
    sub dx, WORD PTR [bp - 10h]
    shr dx, 1
    jae banked16_5cd6
    mov ax, 404h

banked16_5cd6:
    mov WORD PTR [bp - 1Eh], ax
    mov ax, WORD PTR [bx + 0Eh]
    mov WORD PTR [bp - 16h], ax
    les ax, DWORD PTR [bx]
    call check_cursor_overlap
    mov cx, WORD PTR [bp - 6]
    mov dx, cx
    shr dx, 1
    add dx, ax
    mov WORD PTR [bp - 14h], dx
    mov dl, 0FFh
    mov di, WORD PTR [bp - 0Ch]
    and cx, 1
    jne banked16_5cfc
    mov cl, 2

banked16_5cfc:
    sub di, cx
    jle banked16_5d09
    shr di, 1
    jae banked16_5d11
    inc di
    mov dl, 0F0h
    jmp SHORT banked16_5d11

banked16_5d09:
    shr di, 1
    jae banked16_5d0f
    mov dl, 0F0h

banked16_5d0f:
    sub di, di

banked16_5d11:
    mov WORD PTR [bp - 0Ah], di
    mov BYTE PTR [bp - 20h], dl
    mov al, BYTE PTR [bp + 6]
    and al, 0Fh
    mov ah, al
    mov cl, 4
    shl ah, cl
    or al, ah
    mov bx, WORD PTR [bp + 8]
    mov dx, 7
    and bx, dx
    xor al, BYTE PTR cs:[bx + banked16_bitmap_dispatch_xor_masks]
    mov BYTE PTR [bp + 6], al
    shl bx, 1
    and bx, dx
    mov ax, WORD PTR cs:[bx + banked16_bitmap_dispatch]
    mov WORD PTR [bp + 8], ax
    lds si, DWORD PTR [bp + 0Eh]
    mov di, WORD PTR [bp - 10h]
    mov cx, di
    shr di, 1
    mov ax, WORD PTR [si + 4]
    mov WORD PTR [bp - 1Ch], ax
    mul WORD PTR [bp - 1Ah]
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
    mov WORD PTR [bp - 18h], dx
    mov ds, dx
    pop bx
    add ax, 6
    add si, ax
    add si, di
    mov WORD PTR [bp - 1Ah], si
    and cx, 1
    shl cx, 1
    shl cx, 1
    mov WORD PTR [bp - 10h], cx
    mov al, 0FFh
    shr al, cl
    mov BYTE PTR [bp - 12h], al
    mov dh, BYTE PTR [bp + 6]
    jmp SHORT banked16_5dc3

banked16_5d93:
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

banked16_5daa:
    shl ax, cl
    dec bx
    jge banked16_5d93
    and ah, BYTE PTR [bp - 20h]
    mov al, ah
    not al
    and al, BYTE PTR es:[di]
    and ah, dh
    or al, ah
    stosb

banked16_5dbe:
    dec WORD PTR [bp - 0Eh]
    je banked16_5e06

banked16_5dc3:
    mov di, WORD PTR [bp - 14h]
    mov si, WORD PTR [bp - 8]
    inc WORD PTR [bp - 8]
    shl si, 1
    add si, WORD PTR [bp - 16h]
    add di, WORD PTR ss:[si]
    lds si, DWORD PTR [bp - 1Ah]
    mov cx, WORD PTR [bp - 1Ch]
    add cx, si
    mov ax, cx
    and cx, 0Fh
    mov WORD PTR [bp - 1Ah], cx
    mov cl, 4
    shr ax, cl
    mov cx, ds
    add ax, cx
    mov WORD PTR [bp - 18h], ax
    mov bx, WORD PTR [bp - 0Ah]
    mov cx, WORD PTR [bp - 1Eh]
    sub ah, ah
    lodsb
    and al, BYTE PTR [bp - 12h]
    cmp cl, BYTE PTR [bp - 10h]
    ja banked16_5e03
    mov ah, al
    lodsb

banked16_5e03:
    jmp WORD PTR [bp + 8]

banked16_5e06:
    push ss
    pop ds
    push cs
    call show_mouse_graphics
    jmp banked16_5cb9

banked16_5e0f:
    and ah, dh
    xor BYTE PTR es:[di], ah
    inc di
    xchg cl, ch
    shl ax, cl
    xchg cl, ch
    lodsb

banked16_5e1c:
    shl ax, cl
    dec bx
    jge banked16_5e0f
    and ah, BYTE PTR [bp - 20h]
    and ah, dh
    xor BYTE PTR es:[di], ah
    jmp SHORT banked16_5dbe

banked16_5e2b:
    and ah, dh
    or BYTE PTR es:[di], ah
    inc di
    xchg cl, ch
    shl ax, cl
    xchg cl, ch
    lodsb

banked16_5e38:
    shl ax, cl
    dec bx
    jge banked16_5e2b
    and ah, BYTE PTR [bp - 20h]
    and ah, dh
    or BYTE PTR es:[di], ah
    jmp banked16_5dbe

banked16_5e48:
    and ah, dh
    not ah
    and BYTE PTR es:[di], ah
    inc di
    xchg cl, ch
    shl ax, cl
    xchg cl, ch
    lodsb

banked16_5e57:
    shl ax, cl
    dec bx
    jge banked16_5e48
    and ah, BYTE PTR [bp - 20h]
    and ah, dh
    not ah
    and BYTE PTR es:[di], ah
    jmp banked16_5dbe
draw_banked16_bitmap ENDP

banked16_bitmap_dispatch LABEL WORD
    DW OFFSET banked16_5daa, OFFSET banked16_5e57, OFFSET banked16_5e38, OFFSET banked16_5e1c
banked16_bitmap_dispatch_xor_masks LABEL BYTE
    DB 0, 0, 0, 0, 0FFh, 0FFh, 0FFh, 0FFh

; This adapter requires no mode-specific shutdown.
banked16_mode_exit PROC FAR
    retf
banked16_mode_exit ENDP

; Clip and fill the rectangle in the current port.
fill_banked16_rectangle PROC FAR
    push bp
    mov bp, sp
    add sp, -0Eh
    push di
    push si
    mov di, WORD PTR [Graf]
    les si, DWORD PTR [bp + 0Ah]
    mov ax, WORD PTR es:[si + 2]
    mov bx, WORD PTR [di + 6]
    sub ax, bx
    cmp ax, WORD PTR [di + 1Ah]
    jge banked16_5e9a
    mov ax, WORD PTR [di + 1Ah]

banked16_5e9a:
    mov WORD PTR [bp - 6], ax
    mov dx, ax
    shr dx, 1
    mov WORD PTR [bp - 0Eh], dx
    mov bx, WORD PTR es:[si + 6]
    sub bx, WORD PTR [di + 6]
    cmp bx, WORD PTR [di + 1Eh]
    jle banked16_5eb3
    mov bx, WORD PTR [di + 1Eh]

banked16_5eb3:
    mov WORD PTR [bp - 2], bx
    sub bx, ax
    jle banked16_5ed6
    mov dx, 0FFFFh
    and ax, 1
    jne banked16_5ec6
    mov al, 2
    jmp SHORT banked16_5ec8

banked16_5ec6:
    mov dl, 0Fh

banked16_5ec8:
    sub bx, ax
    jl banked16_5ee3
    shr bx, 1
    jae banked16_5ed2
    mov dh, 0Fh

banked16_5ed2:
    not dh
    jmp SHORT banked16_5eef

banked16_5ed6:
    les ax, DWORD PTR [bp + 0Ah]
    mov dx, es
    pop si
    pop di
    mov sp, bp
    pop bp
    retf 8

banked16_5ee3:
    shr bx, 1
    jae banked16_5ee9
    mov dh, 0F0h

banked16_5ee9:
    and dl, dh
    sub dh, dh
    sub bx, bx

banked16_5eef:
    mov WORD PTR [bp - 0Ah], bx
    not dx
    mov WORD PTR [bp - 0Ch], dx
    mov ax, WORD PTR es:[si]
    sub ax, WORD PTR [di + 4]
    cmp ax, WORD PTR [di + 18h]
    jge banked16_5f05
    mov ax, WORD PTR [di + 18h]

banked16_5f05:
    mov WORD PTR [bp - 8], ax
    mov si, WORD PTR es:[si + 4]
    sub si, WORD PTR [di + 4]
    cmp si, WORD PTR [di + 1Ch]
    jle banked16_5f17
    mov si, WORD PTR [di + 1Ch]

banked16_5f17:
    mov WORD PTR [bp - 4], si
    sub si, ax
    jle banked16_5ed6
    les ax, DWORD PTR [di]
    call check_cursor_overlap
    add WORD PTR [bp - 0Eh], ax
    mov bx, WORD PTR [di + 0Eh]
    mov di, WORD PTR [bp + 8]
    mov dx, 7
    and di, dx
    mov al, BYTE PTR cs:[di + banked16_fill_dispatch_xor_masks]
    xor al, BYTE PTR [bp + 6]
    and al, 0Fh
    mov ah, al
    mov cl, 4
    shl ah, cl
    or ah, al
    shl di, 1
    and di, dx
    mov dx, WORD PTR cs:[di + banked16_fill_dispatch]
    jmp SHORT banked16_5f76

banked16_5f4e:
    and BYTE PTR es:[di], al
    not al
    and al, ah
    or BYTE PTR es:[di], al
    inc di
    mov al, ah
    shr cx, 1
    rep stosw
    jae banked16_5f62
    stosb

banked16_5f62:
    mov al, BYTE PTR [bp - 0Bh]
    cmp al, 0FFh
    je banked16_5f73
    and BYTE PTR es:[di], al
    not al
    and al, ah
    or BYTE PTR es:[di], al

banked16_5f73:
    dec si
    je banked16_5f8b

banked16_5f76:
    mov di, WORD PTR [bp - 8]
    inc WORD PTR [bp - 8]
    shl di, 1
    mov di, WORD PTR [bx + di]
    add di, WORD PTR [bp - 0Eh]
    mov al, BYTE PTR [bp - 0Ch]
    mov cx, WORD PTR [bp - 0Ah]
    jmp dx

banked16_5f8b:
    push cs
    call show_mouse_graphics
    jmp banked16_5ed6

banked16_5f92:
    not al
    and al, ah
    xor BYTE PTR es:[di], al
    inc di
    jcxz banked16_5fa2

banked16_5f9c:
    xor BYTE PTR es:[di], ah
    inc di
    loop banked16_5f9c

banked16_5fa2:
    mov al, BYTE PTR [bp - 0Bh]
    xor al, 0FFh
    je banked16_5f73
    and al, ah
    xor BYTE PTR es:[di], al
    jmp SHORT banked16_5f73

banked16_5fb0:
    not al
    and al, ah
    or BYTE PTR es:[di], al
    inc di
    jcxz banked16_5fc0

banked16_5fba:
    or BYTE PTR es:[di], ah
    inc di
    loop banked16_5fba

banked16_5fc0:
    mov al, BYTE PTR [bp - 0Bh]
    xor al, 0FFh
    je banked16_5f73
    and al, ah
    or BYTE PTR es:[di], al
    jmp SHORT banked16_5f73

banked16_5fce:
    or al, ah
    and BYTE PTR es:[di], al
    inc di
    jcxz banked16_5fdc

banked16_5fd6:
    and BYTE PTR es:[di], ah
    inc di
    loop banked16_5fd6

banked16_5fdc:
    mov al, BYTE PTR [bp - 0Bh]
    cmp al, 0FFh
    je banked16_5f73
    or al, ah
    and BYTE PTR es:[di], al
    jmp SHORT banked16_5f73
fill_banked16_rectangle ENDP

banked16_fill_dispatch LABEL WORD
    DW OFFSET banked16_5f4e, OFFSET banked16_5fce, OFFSET banked16_5fb0, OFFSET banked16_5f92
banked16_fill_dispatch_xor_masks LABEL BYTE
    DB 0, 0, 0, 0, 0FFh, 0FFh, 0FFh, 0FFh

; Composite bitmap pixels with the selected raster operation.
composite_banked16_bitmap PROC FAR
    push bp
    mov bp, sp
    add sp, -1Ch
    push di
    push si
    mov bx, WORD PTR [Graf]
    les si, DWORD PTR [bp + 0Ch]
    mov ax, WORD PTR [bp + 0Ah]
    sub ax, WORD PTR [bx + 6]
    sub cx, cx
    mov di, ax
    sub di, WORD PTR [bx + 1Ah]
    jge banked16_601d
    mov ax, WORD PTR [bx + 1Ah]
    sub cx, di

banked16_601d:
    mov WORD PTR [bp - 6], ax
    mov di, ax
    add di, WORD PTR es:[si + 2]
    sub di, cx
    cmp di, WORD PTR [bx + 1Eh]
    jle banked16_6030
    mov di, WORD PTR [bx + 1Eh]

banked16_6030:
    mov WORD PTR [bp - 2], di
    sub di, ax
    mov WORD PTR [bp - 1Ah], di
    jle banked16_606d
    mov ax, WORD PTR [bp + 8]
    sub ax, WORD PTR [bx + 4]
    sub dx, dx
    mov di, ax
    sub di, WORD PTR [bx + 18h]
    jge banked16_604e
    mov ax, WORD PTR [bx + 18h]
    sub dx, di

banked16_604e:
    mov WORD PTR [bp - 8], ax
    mov WORD PTR [bp - 14h], dx
    mov di, ax
    add di, WORD PTR es:[si]
    sub di, dx
    cmp di, WORD PTR [bx + 1Ch]
    jle banked16_6063
    mov di, WORD PTR [bx + 1Ch]

banked16_6063:
    mov WORD PTR [bp - 4], di
    sub di, ax
    mov WORD PTR [bp - 1Ch], di
    jg banked16_607a

banked16_606d:
    les ax, DWORD PTR [bp + 0Ch]
    mov dx, es
    pop si
    pop di
    mov sp, bp
    pop bp
    retf 0Ah

banked16_607a:
    les ax, DWORD PTR [bx]
    call check_cursor_overlap
    mov si, WORD PTR [bp - 6]
    shr si, 1
    add ax, si
    mov WORD PTR [bp - 0Eh], ax
    mov ax, WORD PTR [bx + 0Eh]
    mov WORD PTR [bp - 10h], ax
    lds si, DWORD PTR [bp + 0Ch]
    mov ax, WORD PTR [si + 4]
    mov WORD PTR [bp - 16h], ax
    mul WORD PTR [bp - 14h]
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
    and ax, 1
    shr cx, 1
    add si, cx
    mov WORD PTR [bp - 14h], si
    mov dx, WORD PTR [bp - 6]
    sub dx, ax
    shl ax, 1
    shl ax, 1
    mov WORD PTR [bp - 0Ch], ax
    mov ax, 8
    shr dx, 1
    jae banked16_60dd
    mov ax, 404h

banked16_60dd:
    mov WORD PTR [bp - 18h], ax
    mov dx, 0FFFFh
    mov di, WORD PTR [bp - 1Ah]
    mov ax, WORD PTR [bp - 6]
    and ax, 1
    jne banked16_60f2
    mov al, 2
    jmp SHORT banked16_60f4

banked16_60f2:
    mov dl, 0Fh

banked16_60f4:
    sub di, ax
    jle banked16_6101
    shr di, 1
    jae banked16_610b
    mov dh, 0F0h
    inc di
    jmp SHORT banked16_610b

banked16_6101:
    shr di, 1
    jae banked16_6107
    mov dh, 0F0h

banked16_6107:
    and dl, dh
    sub di, di

banked16_610b:
    mov WORD PTR [bp - 0Ah], di
    mov WORD PTR [bp - 12h], dx
    mov cx, WORD PTR [bp - 18h]
    mov bx, WORD PTR [bp + 6]
    cmp bx, 8
    je banked16_613f
    cmp bx, 0Ah
    je banked16_613a
    mov di, 7
    and bx, di
    mov dl, BYTE PTR cs:[bx + banked16_composite_dispatch_xor_masks]
    shl bx, 1
    and bx, di
    mov ax, WORD PTR cs:[bx + banked16_composite_dispatch]
    mov WORD PTR [bp + 6], ax
    jmp banked16_61c5

banked16_613a:
    mov ax, OFFSET banked16_6323
    jmp SHORT banked16_6142

banked16_613f:
    mov ax, OFFSET banked16_62d0

banked16_6142:
    mov WORD PTR [bp + 6], ax
    sub dl, dl
    jmp SHORT banked16_61c5

banked16_6149:
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
    jl banked16_61a4
    je banked16_6190
    test ch, ch
    jne banked16_617f
    xchg bx, cx
    test dl, dl
    jne banked16_6175
    shr cx, 1
    rep movsw
    xchg bx, cx
    jae banked16_6190
    movsb
    jmp SHORT banked16_6190

banked16_6175:
    lodsb
    xor al, dl
    stosb
    loop banked16_6175
    xchg bx, cx
    jmp SHORT banked16_6190

banked16_617f:
    lodsb
    xor al, dl
    xchg al, ah
    rol ax, cl
    stosb
    xchg cl, ch
    rol ah, cl
    xchg cl, ch
    dec bx
    jg banked16_617f

banked16_6190:
    lodsb
    xor al, dl
    xchg al, ah
    rol ax, cl
    mov dh, BYTE PTR [bp - 11h]
    and al, dh
    not dh
    and dh, BYTE PTR es:[di]
    or al, dh
    stosb

banked16_61a4:
    dec WORD PTR [bp - 1Ch]
    je banked16_61f3
    mov bx, WORD PTR [bp - 16h]
    add bx, WORD PTR [bp - 14h]
    mov ax, bx
    and bx, 0Fh
    mov WORD PTR [bp - 14h], bx
    shr ax, 1
    shr ax, 1
    shr ax, 1
    shr ax, 1
    mov bx, ds
    add ax, bx
    mov ds, ax

banked16_61c5:
    mov di, WORD PTR [bp - 8]
    inc WORD PTR [bp - 8]
    shl di, 1
    add di, WORD PTR [bp - 10h]
    mov di, WORD PTR ss:[di]
    add di, WORD PTR [bp - 0Eh]
    mov bx, WORD PTR [bp - 0Ah]
    mov dh, BYTE PTR [bp - 12h]
    mov si, WORD PTR [bp - 14h]
    sub ah, ah
    lodsb
    xor al, dl
    cmp cl, BYTE PTR [bp - 0Ch]
    ja banked16_61ee
    mov ah, al
    lodsb
    xor al, dl

banked16_61ee:
    shl ax, cl
    jmp WORD PTR [bp + 6]

banked16_61f3:
    push ss
    pop ds
    push cs
    call show_mouse_graphics
    jmp banked16_606d

banked16_61fc:
    and ah, dh
    not ah
    and BYTE PTR es:[di], ah
    inc di
    xchg cl, ch
    shl ax, cl
    xchg cl, ch
    dec bx
    jl banked16_61a4
    je banked16_6238
    test ch, ch
    jne banked16_6224
    xchg bx, cx

banked16_6215:
    lodsb
    xor al, dl
    not al
    and BYTE PTR es:[di], al
    inc di
    loop banked16_6215
    xchg bx, cx
    jmp SHORT banked16_6238

banked16_6224:
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
    jg banked16_6224

banked16_6238:
    lodsb
    xor al, dl
    shl ax, cl
    and ah, BYTE PTR [bp - 11h]
    not ah
    and BYTE PTR es:[di], ah
    jmp banked16_61a4

banked16_6248:
    and ah, dh
    or BYTE PTR es:[di], ah
    inc di
    xchg cl, ch
    shl ax, cl
    xchg cl, ch
    dec bx
    jl banked16_6289
    je banked16_627e
    test ch, ch
    jne banked16_626c
    xchg bx, cx

banked16_625f:
    lodsb
    xor al, dl
    or BYTE PTR es:[di], al
    inc di
    loop banked16_625f
    xchg bx, cx
    jmp SHORT banked16_627e

banked16_626c:
    lodsb
    xor al, dl
    shl ax, cl
    or BYTE PTR es:[di], ah
    inc di
    xchg cl, ch
    shl ax, cl
    xchg cl, ch
    dec bx
    jg banked16_626c

banked16_627e:
    lodsb
    xor al, dl
    shl ax, cl
    and ah, BYTE PTR [bp - 11h]
    or BYTE PTR es:[di], ah

banked16_6289:
    jmp banked16_61a4

banked16_628c:
    and ah, dh
    xor BYTE PTR es:[di], ah
    inc di
    xchg cl, ch
    shl ax, cl
    xchg cl, ch
    dec bx
    jl banked16_62cd
    je banked16_62c2
    test ch, ch
    jne banked16_62b0
    xchg bx, cx

banked16_62a3:
    lodsb
    xor al, dl
    xor BYTE PTR es:[di], al
    inc di
    loop banked16_62a3
    xchg bx, cx
    jmp SHORT banked16_62c2

banked16_62b0:
    lodsb
    xor al, dl
    shl ax, cl
    xor BYTE PTR es:[di], ah
    inc di
    xchg cl, ch
    shl ax, cl
    xchg cl, ch
    dec bx
    jg banked16_62b0

banked16_62c2:
    lodsb
    xor al, dl
    shl ax, cl
    and ah, BYTE PTR [bp - 11h]
    xor BYTE PTR es:[di], ah

banked16_62cd:
    jmp banked16_61a4

banked16_62d0:
    xchg al, ah
    and al, dh
    mov dx, bx
    mov bx, OFFSET banked16_transparency_masks
    xlat BYTE PTR cs:[bx]
    and BYTE PTR es:[di], al
    inc di
    xchg cl, ch
    rol ah, cl
    xchg cl, ch
    dec dx
    jl banked16_631e
    je banked16_6311
    test ch, ch
    jne banked16_62fd
    xchg cx, dx

banked16_62f0:
    lodsb
    xlat BYTE PTR cs:[bx]
    and BYTE PTR es:[di], al
    inc di
    loop banked16_62f0
    xchg cx, dx
    jmp SHORT banked16_6311

banked16_62fd:
    lodsb
    xchg al, ah
    rol ax, cl
    xlat BYTE PTR cs:[bx]
    and BYTE PTR es:[di], al
    inc di
    xchg cl, ch
    rol ah, cl
    xchg cl, ch
    dec dx
    jg banked16_62fd

banked16_6311:
    lodsb
    xchg al, ah
    rol ax, cl
    and al, BYTE PTR [bp - 11h]
    xlat BYTE PTR cs:[bx]
    and BYTE PTR es:[di], al

banked16_631e:
    sub dl, dl
    jmp banked16_61a4

banked16_6323:
    xchg al, ah
    and al, dh
    mov dh, BYTE PTR [bp - 11h]
    push bp
    mov bp, bx
    mov bx, OFFSET banked16_transparency_masks
    mov dl, al
    xlat BYTE PTR cs:[bx]
    and al, BYTE PTR es:[di]
    or al, dl
    stosb
    xchg cl, ch
    rol ah, cl
    xchg cl, ch
    dec bp
    jl banked16_6385
    je banked16_6374
    test ch, ch
    jne banked16_635c
    xchg cx, bp

banked16_634b:
    lodsb
    mov dl, al
    xlat BYTE PTR cs:[bx]
    and al, BYTE PTR es:[di]
    or al, dl
    stosb
    loop banked16_634b
    xchg cx, bp
    jmp SHORT banked16_6374

banked16_635c:
    lodsb
    xchg al, ah
    rol ax, cl
    mov dl, al
    xlat BYTE PTR cs:[bx]
    and al, BYTE PTR es:[di]
    or al, dl
    stosb
    xchg cl, ch
    rol ah, cl
    xchg cl, ch
    dec bp
    jg banked16_635c

banked16_6374:
    lodsb
    xchg al, ah
    rol ax, cl
    and al, dh
    mov dl, al
    xlat BYTE PTR cs:[bx]
    and al, BYTE PTR es:[di]
    or al, dl
    stosb

banked16_6385:
    pop bp
    sub dl, dl
    jmp banked16_61a4
composite_banked16_bitmap ENDP

banked16_transparency_masks LABEL BYTE
    ; A zero source pixel leaves its destination nibble unchanged.
    pixel_pair = 0
    REPT 256
        preserve = 0
        IF (pixel_pair AND 0F0h) EQ 0
            preserve = preserve OR 0F0h
        ENDIF
        IF (pixel_pair AND 0Fh) EQ 0
            preserve = preserve OR 0Fh
        ENDIF
        DB preserve
        pixel_pair = pixel_pair + 1
    ENDM

banked16_composite_dispatch LABEL WORD
    DW OFFSET banked16_6149, OFFSET banked16_61fc, OFFSET banked16_6248, OFFSET banked16_628c
banked16_composite_dispatch_xor_masks LABEL BYTE
    DB 0, 0, 0, 0, 0FFh, 0FFh, 0FFh, 0FFh

banked16_boundary_alignment LABEL BYTE
    DB 0
END

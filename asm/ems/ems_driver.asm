; EMS backing-store operations. AL selects probe, open, close, seek, read, write.
; AH selects the seek origin. CX:DX holds seek offsets or move lengths; DS:SI
; supplies a conventional-memory buffer. Carry reports failure.
; SS addresses the shared backing-store workspace even when the caller's DS
; points at the transfer buffer. Names describe operations inferred from the code.

_DATA SEGMENT WORD PUBLIC 'DATA'
EXTRN ems_handle:WORD
EXTRN ems_position:DWORD
EXTRN ems_capacity:DWORD
EXTRN ems_position_hi:WORD
EXTRN ems_capacity_hi:WORD
EXTRN HEAP_STATE:DWORD
_DATA ENDS
DGROUP GROUP _DATA

EMS_TEXT SEGMENT BYTE PUBLIC 'CODE'
ASSUME CS:EMS_TEXT, DS:DGROUP, SS:DGROUP

EMS_PAGE_BYTES EQU 4000h

; INT 67h move descriptor. In a conventional-memory operand the offset/page
; pair holds offset/segment and the handle is zero.
ems_move_length_lo EQU WORD PTR [si]
ems_move_length_hi EQU WORD PTR [si+2]
ems_source_type    EQU BYTE PTR [si+4]
ems_source_handle  EQU WORD PTR [si+5]
ems_source_offset  EQU WORD PTR [si+7]
ems_source_page    EQU WORD PTR [si+9]
ems_dest_type      EQU BYTE PTR [si+0Bh]
ems_dest_handle    EQU WORD PTR [si+0Ch]
ems_dest_offset    EQU WORD PTR [si+0Eh]
ems_dest_page      EQU WORD PTR [si+10h]

PUBLIC EMS_DRIVER
EMS_DRIVER PROC FAR
    push bp
    mov bp, sp
    add sp, -12h
    push di
    push si
    push ds
    mov bx, ax
    sub bh, bh
    shl bx, 1
    jmp WORD PTR cs:[bx+ems_dispatch]
ems_dispatch LABEL WORD
    DW OFFSET ems_query, OFFSET ems_open, OFFSET ems_close
    DW OFFSET ems_seek, OFFSET ems_read, OFFSET ems_write

ems_query:
    call EMS_PROBE
    jmp SHORT ems_return

; Clear the first workspace word, then allocate one EMS page.
ems_open:
    sub ax, ax
    mov cx, 5
    mov di, OFFSET DGROUP:ems_handle
    push ss
    pop es
    stosw
    call EMS_PROBE
    jb ems_return
    push ax
    push dx
    mov bx, 1
    mov ah, 43h
    int 67h
    mov WORD PTR ss:[ems_handle], dx
    mov WORD PTR ss:[ems_capacity], EMS_PAGE_BYTES
    test ah, ah
    pop dx
    pop ax
    je ems_return
    stc
    jmp SHORT ems_return
ems_close:
    mov dx, WORD PTR ss:[ems_handle]
    mov ah, 45h
    int 67h
    test ah, ah
    jne ems_failure
    jmp SHORT ems_success
ems_seek:
    cmp ah, 0
    je ems_store_position
    cmp ah, 1
    je ems_seek_current
    cmp ah, 2
    jne ems_failure
    add dx, WORD PTR ss:[ems_capacity]
    adc cx, WORD PTR ss:[ems_capacity_hi]
ems_store_position:
    mov WORD PTR ss:[ems_position], dx
    mov WORD PTR ss:[ems_position_hi], cx
ems_success:
    clc
ems_return:
    pop ds
    pop si
    pop di
    mov sp, bp
    pop bp
    retf
ems_seek_current:
    add dx, WORD PTR ss:[ems_position]
    adc cx, WORD PTR ss:[ems_position_hi]
    jmp SHORT ems_store_position
ems_failure:
    stc
    jmp SHORT ems_return
; Build a LIM EMS move descriptor on the stack (EMS to conventional).
ems_read:
    mov WORD PTR [bp-4], si
    mov WORD PTR [bp-2], ds
    lea si, [bp-12h]
    push ss
    pop ds
    mov ems_move_length_lo, dx
    mov ems_move_length_hi, cx
    mov ems_dest_type, 0
    mov ems_dest_handle, 0
    mov ems_source_type, 1
    mov ax, WORD PTR [ems_handle]
    mov ems_source_handle, ax
    les dx, [ems_position]
    mov cx, es
    call EMS_SPLIT_PAGE_OFFSET
    mov ems_source_offset, bx
    mov ems_source_page, ax
    add dx, ems_move_length_lo
    adc cx, ems_move_length_hi
    cmp cx, WORD PTR [ems_capacity_hi]
    ja ems_failure
    jb ems_move
    cmp dx, WORD PTR [ems_capacity]
    ja ems_failure
ems_move:
    mov ax, 5700h
    int 67h
    test ah, ah
    je ems_store_position
    cmp ah, 94h
    jne ems_failure
    ; EMS 94h: translate a conventional address in the page frame and retry.
    push cx
    push dx
    mov di, si
    cmp BYTE PTR [di+4], 0
    je ems_segment_to_page
    add di, 7
ems_segment_to_page:
    mov ah, 41h
    int 67h
    mov dx, WORD PTR [di+9]
    sub dx, bx
    sub ax, ax
    mov cx, 4
ems_scale_paragraphs:
    shl dx, 1
    rcl ax, 1
    loop ems_scale_paragraphs
    xchg cx, ax
    add dx, WORD PTR [di+7]
    adc cx, ax
    call EMS_SPLIT_PAGE_OFFSET
    mov WORD PTR [di+7], bx
    mov WORD PTR [di+9], ax
    mov BYTE PTR [di+4], 1
    les bx, HEAP_STATE
    mov ax, WORD PTR es:[bx+0Ch]
    mov WORD PTR [di+5], ax
    pop dx
    pop cx
    jmp SHORT ems_move
ems_write_failed:
    jmp NEAR PTR ems_failure
; Build the opposite descriptor (conventional to EMS).
ems_write:
    mov WORD PTR [bp-0Bh], si
    mov WORD PTR [bp-9], ds
    lea si, [bp-12h]
    push ss
    pop ds
    mov ems_move_length_lo, dx
    mov ems_move_length_hi, cx
    mov ems_source_type, 0
    mov ems_source_handle, 0
    mov ems_dest_type, 1
    mov ax, WORD PTR [ems_handle]
    mov ems_dest_handle, ax
    les dx, [ems_position]
    mov cx, es
    call EMS_SPLIT_PAGE_OFFSET
    mov ems_dest_offset, bx
    mov ems_dest_page, ax
    add dx, ems_move_length_lo
    adc cx, ems_move_length_hi
    cmp cx, WORD PTR [ems_capacity_hi]
    ja ems_grow
    jb ems_write_in_range
    cmp dx, WORD PTR [ems_capacity]
    ja ems_grow
ems_write_in_range:
    jmp NEAR PTR ems_move
; Round the requested endpoint up to a whole 16 KiB page.
ems_grow:
    mov di, dx
    call EMS_SPLIT_PAGE_OFFSET
    sub dx, dx
    cmp dx, bx
    adc ax, dx
    xchg bx, ax
    mov dx, WORD PTR [ems_handle]
    mov ah, 51h
    int 67h
    test ah, ah
    jne ems_write_failed
    mov dx, di
    mov WORD PTR [ems_capacity], dx
    mov WORD PTR [ems_capacity_hi], cx
    jmp NEAR PTR ems_move
ems_signature DB "EMMXXXX0"
EMS_SIGNATURE_LENGTH EQU $ - ems_signature
EMS_DRIVER ENDP

; CX:DX byte offset -> AX logical page, BX offset within page. Preserves CX:DX.
PUBLIC EMS_SPLIT_PAGE_OFFSET
EMS_SPLIT_PAGE_OFFSET PROC NEAR
    push dx
    mov ax, cx
    xchg dx, ax
    mov bx, EMS_PAGE_BYTES
    div bx
    mov bx, dx
    pop dx
    ret
EMS_SPLIT_PAGE_OFFSET ENDP

; Recognize an EMS 4.x manager and return available bytes in DX:AX.
PUBLIC EMS_PROBE
EMS_PROBE PROC NEAR
    push ds
    mov ax, 3567h
    int 21h
    mov cx, EMS_SIGNATURE_LENGTH
    mov di, 0Ah
    push cs
    pop ds
    mov si, OFFSET ems_signature
    repe cmpsb
    jne ems_probe_failed
    mov ah, 46h
    int 67h
    test ah, ah
    jne ems_probe_failed
    cmp al, 40h
    jb ems_probe_failed
    mov ah, 42h
    int 67h
    test ah, ah
    jne ems_probe_failed
    mov ax, EMS_PAGE_BYTES
    mul bx
    clc
ems_probe_return:
    pop ds
    ret
ems_probe_failed:
    stc
    jmp SHORT ems_probe_return
EMS_PROBE ENDP

EMS_TEXT ENDS
END

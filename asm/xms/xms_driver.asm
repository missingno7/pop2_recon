; XMS backing-store operations. AL selects probe, open, close, seek, read, write.
; AH selects the seek origin. CX:DX holds seek offsets or move lengths; DS:SI
; supplies a conventional-memory buffer. Carry reports failure.
; The workspace uses SS (DGROUP) even when DS points at the transfer buffer.

; Workspace shared by the EMS and XMS drivers.
BACKING_DATA SEGMENT WORD COMMON 'DATA'
xms_entry DD 0
xms_handle DW 0
xms_position DD 0
xms_capacity DD 0
BACKING_DATA ENDS
DGROUP GROUP BACKING_DATA

XMS_TEXT SEGMENT BYTE PUBLIC 'CODE'
ASSUME CS:XMS_TEXT, DS:DGROUP, SS:DGROUP

XMS_KIB_BYTES EQU 400h

; AH=0Bh takes DS:SI pointing to a move descriptor. A zero handle means
; that the corresponding low/high offset pair holds a conventional far pointer.
xms_move_length_lo EQU WORD PTR [si]
xms_move_length_hi EQU WORD PTR [si+2]
xms_source_handle  EQU WORD PTR [si+4]
xms_source_lo      EQU WORD PTR [si+6]
xms_source_hi      EQU WORD PTR [si+8]
xms_dest_handle    EQU WORD PTR [si+0Ah]
xms_dest_lo        EQU WORD PTR [si+0Ch]
xms_dest_hi        EQU WORD PTR [si+0Eh]
xms_source_address EQU DWORD PTR [si+6]

PUBLIC XMS_DRIVER
XMS_DRIVER PROC FAR
    push bp
    mov bp, sp
    add sp, -22h
    push di
    push si
    push ds
    mov bx, ax
    sub bh, bh
    shl bx, 1
    jmp WORD PTR cs:[bx+xms_dispatch]
xms_dispatch LABEL WORD
    DW OFFSET xms_query, OFFSET xms_open, OFFSET xms_close
    DW OFFSET xms_seek, OFFSET xms_read, OFFSET xms_write
xms_query:
    call XMS_PROBE
    jmp SHORT xms_return
; Obtain the manager entry and try allocating one KiB.
xms_open:
    push ss
    pop ds
    sub ax, ax
    mov cx, 7
    mov di, OFFSET DGROUP:xms_entry
    push ds
    pop es
    stosw
    call XMS_PROBE
    jb xms_failure
    push ax
    push dx
    mov dx, 1
    mov ah, 9
    call [xms_entry]
    mov WORD PTR [xms_handle], dx
    mov WORD PTR [xms_capacity], XMS_KIB_BYTES
    dec ax
    pop dx
    pop ax
    jne xms_failure
    jmp SHORT xms_success
xms_close:
    mov dx, WORD PTR ss:[xms_handle]
    mov ah, 0Ah
    call ss:[xms_entry]
    dec ax
    jne xms_failure
    jmp SHORT xms_success
xms_seek:
    cmp ah, 0
    je xms_store_position
    cmp ah, 1
    je xms_seek_current
    cmp ah, 2
    je xms_seek_end
xms_failure:
    stc
    jmp SHORT xms_return
xms_seek_end:
    add dx, WORD PTR ss:[xms_capacity]
    adc cx, WORD PTR ss:[xms_capacity+2]
xms_store_position:
    mov WORD PTR ss:[xms_position], dx
    mov WORD PTR ss:[xms_position+2], cx
xms_success:
    clc
xms_return:
    pop ds
    pop si
    pop di
    mov sp, bp
    pop bp
    retf
xms_seek_current:
    add dx, WORD PTR ss:[xms_position]
    adc cx, WORD PTR ss:[xms_position+2]
    jmp SHORT xms_store_position
; Build the XMS move descriptor on the stack (XMS to conventional).
xms_read:
    mov WORD PTR [bp-12h], dx
    mov WORD PTR [bp-10h], cx
    mov WORD PTR [bp-8], 0
    mov WORD PTR [bp-6], si
    mov WORD PTR [bp-4], ds
    push ss
    pop ds
    mov ax, WORD PTR [xms_handle]
    mov WORD PTR [bp-0Eh], ax
    les ax, [xms_position]
    mov WORD PTR [bp-0Ch], ax
    mov WORD PTR [bp-0Ah], es
    add dx, ax
    mov ax, es
    adc cx, ax
    cmp cx, WORD PTR [xms_capacity+2]
    ja xms_failure
    jb xms_read_even_part
    cmp dx, WORD PTR [xms_capacity]
    ja xms_failure
xms_read_even_part:
    lea si, [bp-12h]
    mov di, xms_move_length_lo
    and di, 1
    xor xms_move_length_lo, di
    mov ah, 0Bh
    call [xms_entry]
    dec ax
    jne xms_read_failed
    shr di, 1
    jae xms_read_done
    ; XMS moves require an even length. Fetch the last byte via a two-byte move.
    lea si, [bp-22h]
    mov xms_move_length_lo, 2
    mov xms_move_length_hi, di
    mov xms_dest_handle, di
    lea ax, [bp-2]
    mov xms_dest_lo, ax
    mov xms_dest_hi, ds
    mov ax, WORD PTR [xms_handle]
    mov xms_source_handle, ax
    mov xms_source_lo, dx
    mov xms_source_hi, cx
    sub xms_source_lo, 1
    sbb xms_source_hi, di
    mov ah, 0Bh
    call [xms_entry]
    dec ax
    jne xms_read_failed
    les di, DWORD PTR [bp-6]
    mov ax, es
    add di, WORD PTR [bp-12h]
    jae xms_read_pointer_ready
    add ax, 1000h
xms_read_pointer_ready:
    add ax, WORD PTR [bp-10h]
    mov es, ax
    mov ax, WORD PTR [bp-2]
    stosb
xms_read_done:
    jmp NEAR PTR xms_store_position
xms_read_failed:
    jmp NEAR PTR xms_failure
; Build the opposite descriptor (conventional to XMS).
xms_write:
    mov WORD PTR [bp-12h], dx
    mov WORD PTR [bp-10h], cx
    mov WORD PTR [bp-0Eh], 0
    mov WORD PTR [bp-0Ch], si
    mov WORD PTR [bp-0Ah], ds
    push ss
    pop ds
    mov ax, WORD PTR [xms_handle]
    mov WORD PTR [bp-8], ax
    les ax, [xms_position]
    mov WORD PTR [bp-6], ax
    mov WORD PTR [bp-4], es
    add dx, ax
    mov ax, es
    adc cx, ax
    cmp cx, WORD PTR [xms_capacity+2]
    ja xms_grow
    jb xms_write_even_part
    cmp dx, WORD PTR [xms_capacity]
    jbe xms_write_even_part
; Grow the handle to cover the requested endpoint, rounding up in KiB.
xms_grow:
    mov es, cx
    mov si, dx
    xchg cx, ax
    xchg dx, ax
    mov cx, XMS_KIB_BYTES
    div cx
    sub bx, bx
    cmp bx, dx
    adc bx, ax
    mov di, bx
    mov ah, 0Fh
    mov dx, WORD PTR [xms_handle]
    call [xms_entry]
    dec ax
    je xms_update_capacity
xms_write_failed:
    jmp NEAR PTR xms_failure
xms_update_capacity:
    mov ax, XMS_KIB_BYTES
    mul di
    mov WORD PTR [xms_capacity], ax
    mov WORD PTR [xms_capacity+2], dx
    mov cx, es
    mov dx, si
xms_write_even_part:
    lea si, [bp-12h]
    mov di, xms_move_length_lo
    and di, 1
    xor xms_move_length_lo, di
    mov ah, 0Bh
    call [xms_entry]
    dec ax
    jne xms_write_failed
    shr di, 1
    jae xms_write_done
    ; Preserve the neighboring byte while writing the odd trailing byte.
    lea si, [bp-22h]
    mov xms_move_length_lo, 2
    mov xms_move_length_hi, di
    mov xms_dest_handle, di
    lea ax, [bp-2]
    mov xms_dest_lo, ax
    mov xms_dest_hi, ds
    mov ax, WORD PTR [xms_handle]
    mov xms_source_handle, ax
    mov xms_source_lo, dx
    mov xms_source_hi, cx
    sub xms_source_lo, 1
    sbb xms_source_hi, di
    mov ah, 0Bh
    call [xms_entry]
    dec ax
    jne xms_write_failed
    les di, DWORD PTR [bp-0Ch]
    mov ax, es
    add di, WORD PTR [bp-12h]
    jae xms_write_pointer_ready
    add ax, 1000h
xms_write_pointer_ready:
    add ax, WORD PTR [bp-10h]
    mov es, ax
    mov ax, WORD PTR es:[di]
    mov BYTE PTR [bp-2], al
    sub ax, ax
    xchg xms_source_handle, ax
    mov xms_dest_handle, ax
    les ax, xms_source_address
    xchg xms_dest_lo, ax
    mov xms_source_lo, ax
    mov ax, es
    xchg xms_dest_hi, ax
    mov xms_source_hi, ax
    mov ah, 0Bh
    call [xms_entry]
    dec ax
    jne xms_odd_write_failed
xms_write_done:
    jmp NEAR PTR xms_store_position
xms_odd_write_failed:
    jmp NEAR PTR xms_write_failed
XMS_DRIVER ENDP

PUBLIC XMS_PROBE
XMS_PROBE PROC NEAR
; Discover the XMS manager via INT 2Fh and query the largest free block.
    push ds
    push ss
    pop ds
    mov ax, 4300h
    int 2Fh
    cmp al, 80h
    jne xms_probe_failed
    mov ax, 4310h
    int 2Fh
    mov WORD PTR [xms_entry], bx
    mov WORD PTR [xms_entry+2], es
    sub ah, ah
    call [xms_entry]
    cmp ah, 2
    jb xms_probe_failed
    mov ah, 8
    call [xms_entry]
    cmp bl, 80h
    je xms_probe_failed
    cmp bl, 81h
    je xms_probe_failed
    cmp bl, 0A0h
    je xms_probe_failed
    mov cx, XMS_KIB_BYTES
    mul cx
    clc
xms_probe_return:
    pop ds
    ret
xms_probe_failed:
    stc
    jmp SHORT xms_probe_return
XMS_PROBE ENDP

XMS_TEXT ENDS
END

; Expand a compressed overlay section: the decompressor of the RTLink/Plus run-time
; library. The input is a stream of literals and copies from a sliding window, both
; coded with adaptive prefix codes.
        NAME    EXPAND

$$OVLMGR_UTILS SEGMENT WORD PUBLIC '$$OVLMGR_UTILS'
ASSUME CS:$$OVLMGR_UTILS, DS:NOTHING, ES:NOTHING, SS:NOTHING
PUBLIC $$RTLEXPANDSECTION

; Frame of expand_stream, shared by the near routines.
mode_arg        EQU <bp+1eh>
out_size_arg    EQU <bp+1ah>
out_arg         EQU <bp+16h>
work_size       EQU <bp+12h>
work_arg        EQU <bp+0eh>
write_arg       EQU <bp+0ah>
read_arg        EQU <bp+6>
in_to_end       EQU <bp-2>
out_to_end      EQU <bp-4>
copy_flags      EQU <bp-6>
bit_state       EQU <bp-8>
stream_halve    EQU <bp-0ah>
stream_update   EQU <bp-0ch>
literal_kind    EQU <bp-0eh>
low_bits        EQU <bp-10h>
window_mask     EQU <bp-12h>
window_size     EQU <bp-14h>
end_seen        EQU <bp-16h>
distance_tree   EQU <bp-18h>
length_tree     EQU <bp-1ah>
literal_tree    EQU <bp-1ch>
window          EQU <bp-1eh>
window_next     EQU <bp-20h>
mode            EQU <bp-22h>
read_routine    EQU <bp-26h>
in_left         EQU <bp-2ah>
in_next         EQU <bp-2eh>
write_routine   EQU <bp-32h>
out_left        EQU <bp-36h>
out_next        EQU <bp-3ah>
out_size        EQU <bp-3eh>
out_start       EQU <bp-42h>
in_chunk        EQU <bp-44h>
in_chunk_left   EQU <bp-46h>
out_chunk       EQU <bp-48h>
out_chunk_left  EQU <bp-4ah>
token_count     EQU <bp-4ch>
token_room      EQU <bp-4eh>
tokens          EQU <bp-50h>
work_segment    EQU <bp-52h>

; Fields of an adaptive prefix code; the tables follow it in the work area.
UPDATE_LEFT     EQU 0
UPDATE_EVERY    EQU 2
LONGEST         EQU 4
SYMBOL_BITS     EQU 6
SYMBOLS         EQU 8
LONGEST_USED    EQU 0ah
LENGTH_COUNTS   EQU 0ch
LIMITS          EQU 0eh
LIMITS_2        EQU 10h
FREQUENCIES     EQU 12h
ORDER           EQU 14h
FIRST_CODES     EQU 16h
ESCAPE          EQU 18h
HALVE_LEFT      EQU 1ah
HALVE_EVERY     EQU 1ch
LAST_LENGTH     EQU 1eh
LAST_USED       EQU 20h
HALVE_START     EQU 22h
UPDATE_START    EQU 24h
MOVES           EQU 26h
TREE_HEAD       EQU 28h

WORK_SIZE_BYTES EQU 0f76h
work_area    DB WORK_SIZE_BYTES DUP (0)
input_buffer DB 100h DUP (0)

; Expand one compressed section from the open program file. Arguments: destination
; paragraph and offset, then the file handle. Returns the status of expand_stream in AX.
$$RTLEXPANDSECTION PROC FAR
    push bp
    mov bp, sp
    push bx
    push cx
    push dx
    push es
    mov ax, [bp+0ah]
    mov input_handle, ax
    mov ax, ss
    mov ds, ax
    mov ax, 1
    push ax
    mov ax, [bp+8]
    mov cl, 4
    rol ax, cl
    mov dx, 0fh
    and dx, ax
    xor ax, dx
    push dx
    push ax
    push WORD PTR [bp+6]
    xor ax, ax
    push ax
    push ax
    mov ax, WORK_SIZE_BYTES
    push ax
    push cs
    mov ax, OFFSET work_area
    push ax
    xor ax, ax
    push ax
    push ax
    push cs
    mov ax, OFFSET read_input
    push ax
    call FAR PTR expand_stream
    add sp, 1ah
    pop es
    pop dx
    pop cx
    pop bx
    pop bp
    retf 6
$$RTLEXPANDSECTION ENDP


input_handle DW 0

; Read routine given to expand_stream: fill the input buffer from the file, store its
; address through the far pointer argument and return the number of bytes in DX:AX.
read_input PROC FAR
    push bp
    mov bp, sp
    push ds
    les bx, [bp+6]
    mov ax, cs
    mov ds, ax
    mov dx, OFFSET input_buffer
    mov es:[bx], dx
    mov es:[bx+2], ds
    mov bx, input_handle
    mov cx, 100h
    mov ah, 3fh
    int 21h
    jae inp_1
    xor ax, ax
inp_1:
    xor dx, dx
    pop ds
    pop bp
    retf
read_input ENDP

; The decompressor. Arguments: read routine, write routine (0 without), work area and
; its size, destination and its size, mode (1: the whole output stays in memory).
; AX: 0 done, -1 damaged input, -2 input ended. The near routines below share its frame.
expand_stream PROC FAR
    push bp
    mov bp, sp
    sub sp, 52h
    push si
    push di
    push ds
    mov WORD PTR [in_left], 0
    mov WORD PTR [in_left+2], 0
    mov WORD PTR [in_chunk_left], 0
    mov WORD PTR [in_chunk], 0
    mov WORD PTR [out_chunk_left], 0
    mov WORD PTR [out_chunk], 0
    mov WORD PTR [end_seen], 0
    mov WORD PTR [out_to_end], 0
    mov WORD PTR [in_to_end], 0
    lds si, [read_arg]
    mov [read_routine], si
    mov [read_routine+2], ds
    lds si, [write_arg]
    mov [write_routine], si
    mov [write_routine+2], ds
    mov ax, [mode_arg]
    mov [mode], ax
    call next_input
    mov ch, 8
    les di, [in_next]
    mov cl, 10h
    call read_bits
    jb exp_1
    cmp ax, 0b59ch
    jne exp_1
    mov cl, 8
    call read_bits
    jb exp_1
    mov [literal_kind], ax
    mov cl, 8
    call read_bits
    jb exp_1
    mov cl, 0ch
    call read_bits
    jb exp_1
    mov [stream_update], ax
    mov cl, 0ch
    call read_bits
    jb exp_1
    mov [stream_halve], ax
    mov cl, 4
    call read_bits
    jb exp_1
    mov [in_next], di
    mov [bit_state], cx
    cmp al, 8
    jne exp_2
    mov WORD PTR [window_size], 2000h
    mov WORD PTR [window_mask], 1fffh
    mov WORD PTR [low_bits], 7
    jmp exp_3
exp_1:
    jmp exp_13
exp_2:
    mov WORD PTR [window_size], 1000h
    mov WORD PTR [window_mask], 0fffh
    mov WORD PTR [low_bits], 6
exp_3:
    mov cx, [work_size]
    mov dx, [work_size+2]
    lds si, [work_arg]
    mov [work_segment], ds
    mov [tokens], si
    add si, 600h
    sub cx, 600h
    sbb dx, 0
    jb exp_4
    cmp WORD PTR [mode], 1
    je exp_5
    mov [window_next], si
    mov [window], si
    add si, [window_size]
    sub cx, [window_size]
    sbb dx, 0
    jae exp_5
exp_4:
    jmp exp_12
exp_5:
    or dx, dx
    je exp_6
    mov cx, 0ffffh
exp_6:
    cmp WORD PTR [literal_kind], 1
    je exp_7
    mov [literal_tree], si
    push cx
    mov ax, 1008h
    mov dx, [stream_update]
    mov bx, [stream_halve]
    call start_tree
    pop ax
    jb exp_4
    sub ax, cx
    add si, ax
exp_7:
    mov [length_tree], si
    push cx
    mov ax, 0c06h
    mov dx, [stream_update]
    mov bx, [stream_halve]
    call start_tree
    pop ax
    jb exp_4
    sub ax, cx
    add si, ax
    mov [distance_tree], si
    push cx
    mov ax, 0c06h
    mov dx, [stream_update]
    mov bx, [stream_halve]
    call start_tree
    pop ax
    jb exp_4
    lds si, [out_arg]
    mov [out_start], si
    mov [out_next], si
    mov [out_start+2], ds
    mov [out_next+2], ds
    mov ax, [out_size_arg]
    mov dx, [out_size_arg+2]
    mov [out_size], ax
    mov [out_size+2], dx
    mov [out_left], ax
    mov [out_left+2], dx
    mov bh, [mode]
    and bh, 1
    or bh, 4
    mov [copy_flags], bx
    cmp WORD PTR [mode], 1
    je exp_8
    mov es, [work_segment]
    mov di, [window_next]
    mov cx, [window_size]
    shr cx, 1
    xor ax, ax
    rep stosw
exp_8:
    mov WORD PTR [token_room], 200h
    call decode_tokens
    jb exp_13
    call write_tokens
    jb exp_11
    test WORD PTR [token_room], 0ffffh
    je exp_8
    call finish_output
exp_9:
    xor ax, ax
exp_10:
    pop ds
    pop di
    pop si
    mov sp, bp
    pop bp
    retf
exp_11:
    test WORD PTR [token_count], 0ffffh
    jne exp_12
    test WORD PTR [end_seen], 0ffffh
    jne exp_9
    mov WORD PTR [token_room], 200h
    call decode_tokens
    cmp WORD PTR [token_room], 200h
    jne exp_12
    jmp exp_9
exp_12:
    mov ax, 0ffffh
    jmp exp_10
exp_13:
    mov ax, 0fffeh
    jmp exp_10
expand_stream ENDP

; Decode up to token_room tokens into the token buffer: a literal, or a match of a
; distance and a length marked with 80h. Carry: input ended.
decode_tokens PROC NEAR
    mov ds, [work_segment]
    les di, [in_next]
    mov cx, [bit_state]
    mov dx, [tokens]
dec_1:
    call read_flag
    jb dec_4
    jne dec_5
    cmp WORD PTR [literal_kind], 1
    je dec_3
    mov si, [literal_tree]
    call decode_symbol
dec_2:
    jb dec_4
    xchg dx, si
    mov BYTE PTR [si], 0
    mov [si+1], ax
    xchg dx, si
    add dx, 3
    dec WORD PTR [token_room]
    jne dec_1
    jmp dec_7
dec_3:
    mov cl, 8
    call read_bits
    jmp dec_2
dec_4:
    stc
    jmp dec_8
dec_5:
    mov cl, [low_bits]
    xchg dx, bx
    call read_bits
    xchg dx, bx
    jb dec_4
    mov si, dx
    mov [si+1], ax
    mov si, [distance_tree]
    call decode_symbol
    jb dec_4
    mov cl, [low_bits]
    shl ax, cl
    mov si, dx
    or [si+1], ax
    je dec_6
    mov si, [length_tree]
    call decode_symbol
    jb dec_4
    or al, 80h
    mov si, dx
    mov [si], al
    add dx, 3
    dec WORD PTR [token_room]
    je dec_7
    jmp dec_1
dec_6:
    mov WORD PTR [end_seen], 0ffffh
dec_7:
    clc
dec_8:
    mov [in_next], di
    mov [bit_state], cx
    retn
decode_tokens ENDP

; Write the tokens of the buffer to the output and, when there is one, the window.
; Carry: the output is full.
write_tokens PROC NEAR
    test WORD PTR [out_chunk_left], 0ffffh
    jne wrt_1
    call next_output
    jb wrt_10
wrt_1:
    mov ds, [work_segment]
    mov dx, [tokens]
    mov cx, 200h
    sub cx, [token_room]
    je wrt_2
    mov [token_count], cx
    mov ax, [out_chunk]
    sub ax, [out_chunk_left]
    mov bx, [window_next]
    les di, [out_next]
    add di, ax
    add bx, ax
    mov ax, [copy_flags]
    jmp wrt_4
wrt_2:
    jmp wrt_25
wrt_3:
    mov bx, [window_next]
    les di, [out_next]
    mov ax, [copy_flags]
wrt_4:
    mov si, dx
    test BYTE PTR [si], 80h
    jne wrt_12
    mov al, [si+1]
    test ah, 3
    jne wrt_5
    xchg di, bx
    mov cx, es
    mov si, ds
    mov es, si
    stosb
    xchg di, bx
    mov es, cx
wrt_5:
    stosb
    dec WORD PTR [out_chunk_left]
    je wrt_7
wrt_6:
    add dx, 3
    dec WORD PTR [token_count]
    jne wrt_4
    jmp wrt_25
wrt_7:
    call next_output
    jb wrt_9
    add dx, 3
    dec WORD PTR [token_count]
    jne wrt_3
    jmp wrt_25
wrt_8:
    or cx, cx
    jne wrt_10
wrt_9:
    dec WORD PTR [token_count]
wrt_10:
    stc
    retn
wrt_11:
    jmp wrt_26
wrt_12:
    mov cl, [si]
    and cx, 7fh
    mov si, [si+1]
    inc si
wrt_13:
    test ah, 3
    je wrt_19
    mov al, cl
    cmp cx, [out_chunk_left]
    jb wrt_14
    mov cx, [out_chunk_left]
wrt_14:
    test ah, 4
    jne wrt_11
wrt_15:
    sub [out_chunk_left], cx
    sub al, cl
    mov bx, si
    sub si, di
    neg si
    mov ds, [out_next+2]
    shr cx, 1
    rep movsw
    jae wrt_16
    movsb
wrt_16:
    mov si, bx
    mov cl, al
    test WORD PTR [out_chunk_left], 0ffffh
    je wrt_17
    jcxz wrt_18
    jmp wrt_13
wrt_17:
    call next_output
    jb wrt_8
    mov ds, [work_segment]
    les di, [out_next]
    mov ax, [copy_flags]
    mov bx, [window_next]
    jcxz wrt_18
    jmp wrt_13
wrt_18:
    mov ds, [work_segment]
    jmp wrt_6
wrt_19:
    push dx
    push si
    mov al, cl
    cmp cx, [out_chunk_left]
    jb wrt_20
    mov cx, [out_chunk_left]
wrt_20:
    mov dx, bx
    sub dx, [window]
    sub dx, si
    jae wrt_21
    neg dx
    cmp cx, dx
    jb wrt_21
    mov cx, dx
wrt_21:
    sub [out_chunk_left], cx
    sub al, cl
    sub si, bx
    neg si
    sub si, [window]
    and si, [window_mask]
    add si, [window]
    mov dx, cx
    shr cx, 1
    rep movsw
    jae wrt_22
    movsb
wrt_22:
    mov cx, dx
    sub si, dx
    xchg di, bx
    mov es, [work_segment]
    shr cx, 1
    rep movsw
    jae wrt_23
    movsb
wrt_23:
    xchg di, bx
    mov es, [out_next+2]
    mov cl, al
    pop si
    pop dx
    test WORD PTR [out_chunk_left], 0ffffh
    je wrt_17
    jcxz wrt_24
    jmp wrt_13
wrt_24:
    jmp wrt_6
wrt_25:
    clc
    retn
wrt_26:
    mov bx, di
    sub bx, [out_start]
    cmp bx, [window_size]
    jae wrt_31
    sub bx, si
    jae wrt_29
    neg bx
    xchg bx, cx
    cmp cx, bx
    jb wrt_27
    mov cx, bx
wrt_27:
    sub [out_chunk_left], cx
    sub bx, cx
    sub al, cl
    push ax
    xor ax, ax
    shr cx, 1
    rep stosw
    jae wrt_28
    stosb
wrt_28:
    pop ax
    xchg bx, cx
    jcxz wrt_30
wrt_29:
    jmp wrt_15
wrt_30:
    mov bx, si
    jmp wrt_16
wrt_31:
    and ah, 0fbh
    and WORD PTR [copy_flags], 0fb00h
    jmp wrt_15
write_tokens ENDP

; Account for the input chunk just used and prepare the next one; call the read
; routine when the buffer is empty. Carry: no input left.
next_input PROC NEAR
    push ax
    mov ax, [in_chunk]
    sub ax, [in_chunk_left]
    sub [in_left], ax
    sbb WORD PTR [in_left+2], 0
    mov ax, [in_left]
    or ax, [in_left+2]
    je nin_2
    mov ax, [in_next]
    add ax, [in_chunk]
    sub ax, [in_chunk_left]
    test WORD PTR [in_to_end], 0ffffh
    jne nin_1
    sub ax, 8000h
    add WORD PTR [in_next+2], 800h
nin_1:
    mov [in_next], ax
    jmp nin_6
nin_2:
    push bx
    push cx
    push dx
    push es
    push ds
    mov ax, ss
    mov ds, ax
    push ss
    lea ax, [in_next]
    push ax
    call DWORD PTR [read_routine]
    add sp, 4
    mov [in_left], ax
    mov [in_left+2], dx
    or dx, dx
    jne nin_4
    mov ax, [in_next]
    add ax, [in_left]
    je nin_3
    jb nin_4
nin_3:
    mov WORD PTR [in_to_end], 0ffffh
    jmp nin_5
nin_4:
    mov WORD PTR [in_to_end], 0
nin_5:
    pop ds
    pop es
    pop dx
    pop cx
    pop bx
nin_6:
    test WORD PTR [in_left+2], 0ffffh
    jne nin_7
    mov ax, [in_left]
    or ax, ax
    je nin_9
    add ax, [in_next]
    jb nin_7
    mov ax, [in_left]
    jmp nin_8
nin_7:
    mov ax, 0fff0h
    test WORD PTR [in_to_end], 0ffffh
    jne nin_8
    mov ax, [in_next]
    sub ax, 8000h
    neg ax
nin_8:
    mov [in_chunk_left], ax
    mov [in_chunk], ax
    clc
    pop ax
    retn
nin_9:
    stc
    pop ax
    retn
nin_10:
    jmp nout_8
nin_11:
    jmp nout_7
next_input ENDP

; The same for the output: call the write routine when its buffer is full and choose
; how many bytes may be stored without crossing a segment or the window end.
next_output PROC NEAR
    push ax
    push dx
    push cx
    mov ax, [out_chunk]
    sub ax, [out_chunk_left]
    sub [out_left], ax
    sbb WORD PTR [out_left+2], 0
    add [window_next], ax
    add [out_next], ax
    jae nout_1
    sub WORD PTR [out_next], 8000h
    add WORD PTR [out_next+2], 800h
nout_1:
    mov ax, [out_left]
    or ax, [out_left+2]
    jne nin_10
    test WORD PTR [copy_flags], 100h
    jne nin_11
    push bx
    push ds
    push es
    test WORD PTR [copy_flags], 200h
    je nout_2
    push si
    push di
    mov es, [work_segment]
    lds si, [out_next]
    mov cx, [window_size]
    sub si, cx
    mov di, [window]
    mov [window_next], di
    shr cx, 1
    rep movsw
    and WORD PTR [copy_flags], 0f900h
    pop di
    pop si
nout_2:
    push WORD PTR [out_size+2]
    push WORD PTR [out_size]
    push ss
    lea ax, [out_start]
    push ax
    mov ax, ss
    mov ds, ax
    call DWORD PTR [write_routine]
    add sp, 8
    mov cx, ax
    or cx, dx
    je nout_3
    mov [out_size], ax
    mov [out_size+2], dx
nout_3:
    mov ax, [out_start]
    mov [out_next], ax
    mov ax, [out_start+2]
    mov [out_next+2], ax
    mov ax, [out_size]
    mov [out_left], ax
    mov ax, [out_size+2]
    mov [out_left+2], ax
    or ax, ax
    jne nout_5
    mov ax, [out_start]
    add ax, [out_size]
    je nout_4
    jb nout_5
nout_4:
    mov WORD PTR [out_to_end], 0ffffh
    jmp nout_6
nout_5:
    mov WORD PTR [out_to_end], 0
nout_6:
    pop es
    pop ds
    pop bx
    jmp nout_8
nout_7:
    stc
    pop cx
    pop dx
    pop ax
    retn
nout_8:
    test WORD PTR [out_to_end], 0ffffh
    jne nout_9
    mov ax, [window_size]
    add ax, 8000h
    cmp ax, [out_next]
    ja nout_9
    sub WORD PTR [out_next], 8000h
    add WORD PTR [out_next+2], 800h
nout_9:
    test WORD PTR [copy_flags], 300h
    jne nout_10
    mov ax, [window_next]
    sub ax, [window]
    cmp ax, [window_size]
    jb nout_10
    and ax, [window_mask]
    add ax, [window]
    mov [window_next], ax
nout_10:
    mov cx, 0ffffh
    test WORD PTR [copy_flags], 300h
    jne nout_12
    test WORD PTR [out_size+2], 0ffffh
    jne nout_11
    mov ax, [out_size]
    shr ax, 1
    cmp ax, [window_size]
    jbe nout_12
nout_11:
    mov dx, [out_size+2]
    mov ax, [out_size]
    sub ax, [out_left]
    sbb dx, [out_left+2]
    or dx, dx
    jne nout_12
    mov cx, [window_size]
    sub cx, ax
    ja nout_12
    mov cx, 0ffffh
    or WORD PTR [copy_flags], 200h
    and WORD PTR [copy_flags], 0fb00h
nout_12:
    mov ax, 0fff0h
    test WORD PTR [out_to_end], 0ffffh
    jne nout_13
    mov ax, 8000h
    add ax, [window_size]
    sub ax, [out_next]
nout_13:
    test WORD PTR [copy_flags], 300h
    jne nout_14
    mov dx, [window_next]
    sub dx, [window]
    sub dx, [window_size]
    neg dx
    cmp dx, ax
    ja nout_14
    mov ax, dx
nout_14:
    test WORD PTR [out_left+2], 0ffffh
    jne nout_15
    cmp ax, [out_left]
    jb nout_15
    mov ax, [out_left]
nout_15:
    cmp ax, cx
    jb nout_16
    mov ax, cx
nout_16:
    mov [out_chunk_left], ax
    mov [out_chunk], ax
    pop cx
    pop dx
    pop ax
    clc
    retn
nout_17:
    call next_input
    les di, [in_next]
    jb nout_18
    jmp sym_2
nout_18:
    stc
    retn
next_output ENDP

; Decode one symbol of the adaptive prefix code in the tree at SI. CH holds the number of
; unused bits of the byte at ES:DI. AX: symbol. Carry: input ended.
decode_symbol PROC NEAR
    mov al, 1
    mov cl, ch
    shl al, cl
    dec al
    je nout_18
    and al, es:[di]
    xor ah, ah
    xor bx, bx
    xchg bl, ch
    dec bx
    shl bx, 1
    shl bx, 1
    shl bx, 1
    add bx, [si+LIMITS]
sym_1:
    cmp ax, [bx]
    jae sym_3
    mov ah, al
    inc di
    dec WORD PTR [in_chunk_left]
    je nout_17
sym_2:
    mov al, es:[di]
    add cl, 8
    add bx, 40h
    jmp sym_1
sym_3:
    sub bx, 6
    dec cl
    je sym_5
sym_4:
    cmp ax, [bx]
    jb sym_5
    shr ax, 1
    sub bx, 8
    inc ch
    dec cl
    je sym_5
    jmp sym_4
sym_5:
    inc cl
    add bx, 6
    sub ax, [bx]
    shl ax, 1
    sub bx, [si+LIMITS]
    shr bx, 1
    shr bx, 1
    add bx, [si+FIRST_CODES]
    mov bx, [bx]
    add bx, ax
    mov ax, [bx]
    sub ax, [si+FREQUENCIES]
    shr ax, 1
    or ch, ch
    jne sym_6
    inc di
    dec WORD PTR [in_chunk_left]
    mov ch, 8
    jne sym_6
    call next_input
    les di, [in_next]
    jae sym_6
    xor ch, ch
sym_6:
    call count_symbol
    jae sym_7
    call update_tree
    mov bx, [si+FIRST_CODES]
    call set_limits
sym_7:
    cmp ax, [si+ESCAPE]
    jne sym_8
    mov cl, [si+SYMBOL_BITS]
    xchg bx, dx
    call read_bits
    xchg bx, dx
    call add_symbol
    call set_limits
sym_8:
    clc
    retn
sym_9:
    stc
    retn
decode_symbol ENDP

; Read CL bits (at most 16) into AX. Carry: input ended.
read_bits PROC NEAR
    xor dx, dx
    mov al, 1
    xchg ch, cl
    shl al, cl
    dec al
    je sym_9
    and al, es:[di]
    xor ah, ah
    xchg ch, cl
    sub ch, cl
    ja bits_5
    je bits_3
    cmp ch, 0f8h
    ja bits_3
    cmp ch, 0f0h
    ja bits_1
    mov dh, al
    inc di
    dec WORD PTR [in_chunk_left]
    je bits_10
    mov dl, es:[di]
    jmp bits_2
bits_1:
    mov dl, al
bits_2:
    inc di
    dec WORD PTR [in_chunk_left]
    je bits_9
    mov ah, es:[di]
    jmp bits_4
bits_3:
    mov ah, al
bits_4:
    inc di
    dec WORD PTR [in_chunk_left]
    je bits_8
    mov al, es:[di]
bits_5:
    and ch, 7
    je bits_7
    mov cl, ch
    mov ch, 80h
    sar ch, cl
    shl ch, 1
    shr ax, cl
    ror dx, cl
    and ch, dh
    xor dh, ch
    or ah, ch
    mov ch, cl
bits_6:
    retn
bits_7:
    mov al, ah
    mov ah, dl
    mov dl, dh
    xor dh, dh
    mov ch, 8
    retn
bits_8:
    call next_input
    jb bits_11
    les di, [in_next]
    mov al, es:[di]
    jmp bits_5
bits_9:
    call next_input
    jb bits_6
    les di, [in_next]
    mov ah, es:[di]
    jmp bits_4
bits_10:
    call next_input
    jb bits_6
    les di, [in_next]
    mov dl, es:[di]
    jmp bits_2
bits_11:
    and ch, 7
    mov cl, 8
    sub cl, ch
    cmp cl, 8
    stc
    jne bits_6
    mov al, ah
    mov ah, dl
    mov dl, dh
    xor dh, dh
    xor ch, ch
    retn
read_bits ENDP

; Read one bit into the zero flag. Carry: input ended.
read_flag PROC NEAR
    xchg ch, cl
    dec cl
    js flag_5
    mov ch, 1
    shl ch, cl
    and ch, es:[di]
    or cl, cl
    je flag_1
    xchg ch, cl
    or cl, cl
    retn
flag_1:
    inc di
    dec WORD PTR [in_chunk_left]
    je flag_4
flag_2:
    mov cl, 8
flag_3:
    xchg ch, cl
    or cl, cl
    retn
flag_4:
    call next_input
    les di, [in_next]
    jae flag_2
    xor cl, cl
    jmp flag_3
flag_5:
    stc
    retn
read_flag ENDP

; Give the write routine the rest of the output.
finish_output PROC NEAR
    cmp WORD PTR [mode], 1
    je fin_2
    push ax
    push bx
    push cx
    push dx
    push es
    mov ax, [out_size]
    mov dx, [out_size+2]
    sub ax, [out_left]
    sbb dx, [out_left+2]
    add ax, [out_chunk]
    adc dx, 0
    sub ax, [out_chunk_left]
    sbb dx, 0
    push dx
    push ax
    push ss
    lea ax, [out_start]
    push ax
    mov ax, ss
    mov ds, ax
    call DWORD PTR [write_routine]
    add sp, 8
    mov cx, ax
    or cx, dx
    je fin_1
    mov [out_size], ax
    mov [out_left], ax
    mov [out_size+2], dx
    mov [out_left+2], dx
fin_1:
    pop es
    pop dx
    pop cx
    pop bx
    pop ax
fin_2:
    retn
finish_output ENDP

; Set up a tree in the work area at DS:SI, CX bytes left, with its first limits cleared.
; AH: longest code, AL: symbol bits, DX: update interval, BX: halving interval.
start_tree PROC NEAR
    call init_tree
    jb tree_3
    push ax
    push bx
    push es
    push di
    push cx
    cmp cx, 0c0h
    jb tree_1
    sub cx, 0c0h
    add sp, 2
    push cx
    mov ax, 1
    mov cx, [si+SYMBOL_BITS]
    shl ax, cl
    mov [si+ESCAPE], ax
    mov ax, ds
    mov es, ax
    mov di, [si+LIMITS]
    xor ax, ax
    mov cx, 60h
    rep stosw
    clc
    jmp tree_2
tree_1:
    stc
tree_2:
    pop cx
    pop di
    pop es
    pop bx
    pop ax
tree_3:
    retn
start_tree ENDP

; Recompute the code limits from the code length whose list pointer is BX.
set_limits PROC NEAR
    push ax
    push bx
    push cx
    push dx
    push di
    sub bx, [si+FIRST_CODES]
    mov di, bx
    mov cx, bx
    shr cx, 1
    add bx, [si+LENGTH_COUNTS]
    shl di, 1
    shl di, 1
    add di, [si+LIMITS]
    mov ax, 1
    jcxz lim_1
    mov ax, [di-8]
lim_1:
    sub cx, [si+LONGEST]
    neg cx
    jcxz lim_3
    shl ax, 1
    rcl dx, 1
lim_2:
    sub ax, [bx]
    mov [di], ax
    shl ax, 1
    mov [di+2], ax
    mov dx, ax
    shl ax, 1
    mov [di+4], ax
    shl ax, 1
    shl ax, 1
    mov [di+6], ax
    mov ax, dx
    add bx, 2
    add di, 8
    loop lim_2
lim_3:
    pop di
    pop dx
    pop cx
    pop bx
    pop ax
    retn
set_limits ENDP

; Give symbol AX, seen for the first time, a code: append it to the order and lengthen a
; code to make room. BX: list pointer of the first length that changed.
add_symbol PROC NEAR
    push bp
    push cx
    push si
    mov bp, si
    mov bx, ax
    shl bx, 1
    add bx, ds:[bp+FREQUENCIES]
    mov WORD PTR [bx], 1
    mov si, ds:[bp+SYMBOLS]
    shl si, 1
    add si, ds:[bp+ORDER]
    xchg [si], bx
    inc WORD PTR ds:[bp+SYMBOLS]
    cmp WORD PTR ds:[bp+SYMBOLS], 2
    mov bx, ds:[bp+FIRST_CODES]
    je add_5
    mov bx, ds:[bp+LONGEST_USED]
    cmp bx, ds:[bp+LONGEST]
    jb add_2
    sub bx, 2
    shl bx, 1
    add bx, ds:[bp+LENGTH_COUNTS]
add_1:
    test WORD PTR [bx], 0ffffh
    jne add_3
    sub bx, 2
    jmp add_1
add_2:
    dec bx
    shl bx, 1
    add bx, ds:[bp+LENGTH_COUNTS]
    inc WORD PTR ds:[bp+LONGEST_USED]
add_3:
    mov cx, bx
    sub cx, ds:[bp+LENGTH_COUNTS]
    shr cx, 1
    inc cx
    sub cx, ds:[bp+LONGEST]
    neg cx
    dec WORD PTR [bx]
    add WORD PTR [bx+2], 2
    sub bx, ds:[bp+LENGTH_COUNTS]
    add bx, ds:[bp+FIRST_CODES]
    push bx
    add bx, 4
    sub WORD PTR [bx-2], 2
add_4:
    add WORD PTR [bx], 2
    add bx, 2
    loop add_4
    pop bx
add_5:
    pop si
    pop cx
    pop bp
    retn
add_symbol ENDP

; Adapt the code to the frequencies: halve them now and then, sort the symbols of the
; order, and move symbols between code lengths while that shortens the output.
update_tree PROC NEAR
    push bp
    push ax
    push bx
    push cx
    push dx
    push si
    push di
    mov bp, si
    mov ax, [si+LONGEST]
    dec ax
    shl ax, 1
    add ax, [si+FIRST_CODES]
    mov [si+LAST_LENGTH], ax
    mov WORD PTR [si+MOVES], 0
    mov bx, [si+ORDER]
    mov cx, [si+SYMBOLS]
    mov si, [si+HALVE_LEFT]
    dec si
    mov ds:[bp+HALVE_LEFT], si
    xor ax, ax
upd_1:
    mov di, [bx]
    mov dx, [di]
    or si, si
    jne upd_2
    shr dx, 1
    mov [di], dx
upd_2:
    cmp ax, dx
    jae upd_3
    mov ax, dx
upd_3:
    add bx, 2
    loop upd_1
    or ax, ax
    je upd_11
    mov dx, 8000h
upd_4:
    test ax, dx
    jne upd_5
    sar dx, 1
    jmp upd_4
upd_5:
    mov bx, ds:[bp+ORDER]
    mov cx, ds:[bp+SYMBOLS]
upd_6:
    push cx
    mov di, [bx]
    test [di], dx
    jne upd_10
    dec cx
    je upd_10
    lea si, [bx+2]
upd_7:
    mov di, [si]
    test [di], dx
    je upd_8
    mov ax, [bx]
    xchg [si], ax
    mov [bx], ax
    sub si, 2
    cmp si, bx
    je upd_10
    add si, 2
    pop ax
    dec ax
    push ax
    add bx, 2
upd_8:
    add si, 2
    loop upd_7
    pop cx
    sar dx, 1
    test dx, 1
    je upd_6
    jmp upd_11
upd_9:
    jmp upd_25
upd_10:
    pop cx
    add bx, 2
    loop upd_6
upd_11:
    mov cx, ds:[bp+LONGEST_USED]
    mov ax, cx
    dec ax
    shl ax, 1
    add ax, ds:[bp+FIRST_CODES]
    mov ds:[bp+LAST_USED], ax
    mov bx, ds:[bp+FIRST_CODES]
    mov si, ds:[bp+LENGTH_COUNTS]
    sub si, bx
upd_12:
    cmp WORD PTR [bx+si], 0
    je upd_17
    push cx
    push bx
    push si
    cmp WORD PTR [bx+si], 3
    jb upd_13
    cmp bx, ds:[bp+LAST_LENGTH]
    je upd_13
    mov di, [bx]
    mov di, [di]
    mov ax, [di]
    mov di, [bx+2]
    mov dx, di
    mov di, [di-2]
    sub ax, [di]
    mov di, dx
    mov di, [di-4]
    sub ax, [di]
    jns upd_9
upd_13:
    cmp cx, 2
    jbe upd_16
    sub cx, 2
    mov di, [bx+2]
    mov di, [di-2]
    mov ax, [di]
    add bx, 4
upd_14:
    mov di, [bx]
    mov dx, di
    mov di, [di]
    sub ax, [di]
    cmp WORD PTR [bx+si], 2
    jb upd_15
    or ax, ax
    js upd_21
    mov di, dx
    mov di, [di+2]
    cmp ax, [di]
    jb upd_21
upd_15:
    add bx, 2
    loop upd_14
upd_16:
    pop si
    pop bx
    pop cx
upd_17:
    add bx, 2
    loop upd_12
    mov ax, ds:[bp+MOVES]
    cmp ax, 10h
    jae upd_18
    cmp ax, 8
    jae upd_19
    cmp WORD PTR ds:[bp+HALVE_EVERY], 1
    je upd_19
    shl WORD PTR ds:[bp+UPDATE_EVERY], 1
    shr WORD PTR ds:[bp+HALVE_EVERY], 1
    shr WORD PTR ds:[bp+HALVE_LEFT], 1
    jmp upd_19
upd_18:
    mov ax, ds:[bp+UPDATE_START]
    mov ds:[bp+UPDATE_EVERY], ax
    mov ax, ds:[bp+HALVE_START]
    mov ds:[bp+HALVE_EVERY], ax
upd_19:
    mov ax, ds:[bp+UPDATE_EVERY]
    mov ds:[bp+UPDATE_LEFT], ax
    test WORD PTR ds:[bp+HALVE_LEFT], 0ffffh
    jne upd_20
    mov ax, ds:[bp+HALVE_EVERY]
    mov ds:[bp+HALVE_LEFT], ax
upd_20:
    pop di
    pop si
    pop dx
    pop cx
    pop bx
    pop ax
    pop bp
    retn
    jmp upd_25
upd_21:
    inc WORD PTR ds:[bp+MOVES]
    mov dx, bx
    sub dx, 2
    pop si
    pop bx
    pop cx
    dec WORD PTR [bx+si]
    sub WORD PTR [bx+2], 2
    add bx, 2
    add WORD PTR [bx+si], 2
    add WORD PTR [bx+2], 2
upd_22:
    cmp bx, dx
    je upd_23
    add bx, 2
    add WORD PTR [bx+2], 2
    jmp upd_22
upd_23:
    inc WORD PTR [bx+si]
    add WORD PTR [bx+2], 2
    sub WORD PTR [bx+si+2], 2
    mov bx, ds:[bp+LAST_USED]
    test WORD PTR [bx+si], 0ffffh
    jne upd_24
    dec WORD PTR ds:[bp+LONGEST_USED]
upd_24:
    jmp upd_11
upd_25:
    inc WORD PTR ds:[bp+MOVES]
    inc WORD PTR [bx+si-2]
    sub WORD PTR [bx+si], 3
    add WORD PTR [bx+si+2], 2
    add WORD PTR [bx], 2
    sub WORD PTR [bx+2], 4
    cmp bx, ds:[bp+LAST_USED]
    jne upd_26
    inc WORD PTR ds:[bp+LONGEST_USED]
upd_26:
    pop si
    pop bx
    pop cx
    jmp upd_11
update_tree ENDP

; Count symbol AX. Carry: the tree is due for an update.
count_symbol PROC NEAR
    push bx
    mov bx, ax
    shl bx, 1
    add bx, [si+FREQUENCIES]
    inc WORD PTR [bx]
    dec WORD PTR [si]
    jne cnt_1
    stc
cnt_1:
    pop bx
    retn
count_symbol ENDP

; Lay out a tree at DS:SI in CX bytes; carry when they do not suffice. CX: bytes left.
init_tree PROC NEAR
    push es
    push di
    push ax
    push bx
    push dx
    push cx
    cmp cx, 2
    jbe ini_1
    mov [si], dx
    mov [si+UPDATE_EVERY], dx
    mov [si+HALVE_LEFT], bx
    mov [si+HALVE_EVERY], bx
    mov [si+UPDATE_START], dx
    mov [si+HALVE_START], bx
    xor bh, bh
    mov bl, al
    mov [si+SYMBOL_BITS], bx
    mov bl, ah
    mov [si+LONGEST], bx
    mov dx, cx
    mov cx, [si+SYMBOL_BITS]
    mov ax, 1
    shl ax, cl
    add ax, bx
    shl ax, 1
    shl ax, 1
    add ax, 32h
    cmp ax, dx
    jbe ini_2
ini_1:
    stc
    jmp ini_3
ini_2:
    sub dx, ax
    add sp, 2
    push dx
    mov dx, 2
    shl dx, cl
    mov bx, [si+LONGEST]
    shl bx, 1
    lea ax, [si+TREE_HEAD]
    mov [si+LENGTH_COUNTS], ax
    add ax, bx
    mov [si+FIRST_CODES], ax
    add ax, bx
    add ax, 4
    mov [si+ORDER], ax
    add ax, dx
    add ax, 4
    mov [si+FREQUENCIES], ax
    add ax, dx
    add ax, 2
    mov [si+LIMITS], ax
    mov [si+LIMITS_2], ax
    mov WORD PTR [si+SYMBOLS], 1
    mov WORD PTR [si+LONGEST_USED], 1
    mov ax, ds
    mov es, ax
    mov di, [si+LENGTH_COUNTS]
    mov ax, 2
    stosw
    mov cx, [si+LONGEST]
    dec cx
    xor ax, ax
    rep stosw
    mov di, [si+FIRST_CODES]
    mov ax, [si+ORDER]
    stosw
    mov cx, [si+LONGEST]
    add ax, 4
    rep stosw
    mov di, [si+ORDER]
    mov ax, [si+FREQUENCIES]
    add ax, dx
    stosw
    add ax, 2
    mov cx, dx
    shr cx, 1
    inc cx
    rep stosw
    mov di, [si+FREQUENCIES]
    mov cx, dx
    shr cx, 1
    mov ax, 8000h
    rep stosw
    xor ax, ax
    stosw
    stosw
    clc
ini_3:
    pop cx
    pop dx
    pop bx
    pop ax
    pop di
    pop es
    retn
init_tree ENDP

$$OVLMGR_UTILS ENDS
END

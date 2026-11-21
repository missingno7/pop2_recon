.MODEL MEDIUM
.DATA
text_codes DB 0, " aetonisrdlhugfcwypbmk,vSA.T'PMxBCIRGDWHqE-zNFK"
           DB "L0j:51YJ8\U?73Q;2!469", 13, 10, 9, "OVXZ()*+"
           DB 22h, "#$%&<=>/@[]^_", 60h, 7
           DB 1, 2, 3, 4, 5, 6, 8, 11, 12, 14, 15, 16, 17, 18, 19, 20
           DB 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, "{|}~", 127

.CODE
PUBLIC XLATTXT4

decoded_count EQU WORD PTR [bp-16]
escape_width  EQU WORD PTR [bp-14]
written       EQU WORD PTR [bp-12]
bit_position  EQU WORD PTR [bp-8]
code_width    EQU WORD PTR [bp-6]
bits_left     EQU WORD PTR [bp-4]
source_offset EQU WORD PTR [bp+6]
source_segment EQU WORD PTR [bp+8]

XLATTXT4 PROC FAR
    push bp
    mov bp, sp
    sub sp, 16
    push di
    push si

    ; A word count precedes the bitstream. Its top two bits select escape width.
    les bx, DWORD PTR [bp+6]
    add source_offset, 2
    mov si, es:[bx]
    mov cl, 6
    mov bx, source_offset
    mov al, es:[bx]
    shr al, cl
    sub ah, ah
    add ax, 5
    mov escape_width, ax
    mov written, 0
    mov bit_position, 2
    mov di, 4
    or si, si
    jne begin_decode
    jmp finished

begin_decode:
    mov decoded_count, si
next_code:
    mov code_width, di
    sub di, di
    mov si, code_width
    or si, si
    je have_code
    mov bits_left, si
    mov si, di
    mov di, bit_position

read_bit:
    shl si, 1
    mov cl, 3
    mov bx, di
    shr bx, cl
    add bx, source_offset
    mov es, source_segment
    mov al, es:[bx]
    cbw
    mov cx, di
    and cl, 7
    mov dx, 80h
    sar dx, cl
    test ax, dx
    je advance_bit
    or si, 1
advance_bit:
    inc di
    dec bits_left
    jne read_bit
    mov bit_position, di
    mov di, si

have_code:
    or di, di
    jne translate_code
    cmp code_width, 4
    jne translate_code
    mov si, escape_width
    mov code_width, si
    mov di, si
    jmp short next_output
    even

translate_code:
    ; Eight-bit codes are literal. Shorter codes index the table.
    mov si, di
    cmp code_width, 8
    je write_byte
    mov al, text_codes[si]
    cbw
    mov si, ax
write_byte:
    mov ax, si
    les bx, DWORD PTR [bp+10]
    mov si, written
    mov es:[bx+si], al
    inc written
    mov di, 4
next_output:
    mov ax, written
    cmp decoded_count, ax
    ja next_code

finished:
    mov ax, WORD PTR [bp+10]
    mov dx, WORD PTR [bp+12]
    pop si
    pop di
    mov sp, bp
    pop bp
    retf 8
XLATTXT4 ENDP
END


; Run-time services shared by the RTLink/Plus utilities: the fatal error handler,
; the termination hook in the PSP and the chain of routines that are called when
; the program terminates or a preload section is brought in.
        NAME    RTCOMMON

EXTRN $$RTLCOMMON_MESSAGES:WORD         ; table of message pointers

$$OVLMGR_UTILS SEGMENT BYTE PUBLIC '$$OVLMGR_UTILS'
ASSUME CS:$$OVLMGR_UTILS, DS:NOTHING, ES:NOTHING, SS:NOTHING

PUBLIC $$RTOVEXEOFFSET, $$RTLSETERROR, $$RTL_FATAL_ERROR, $$RTL_ORG_TERMINATE
PUBLIC $$RTL_TERM_PSP, $$RTL_TERMINATE, $$RTL_TERM_INIT, $$RTL_REGISTER
PUBLIC $$RTL_PRELOAD

$$RTOVEXEOFFSET DW 0, 0                 ; offset of the overlays in the program file

; Exchange the fatal error handler with the far address in DS:SI.
$$RTLSETERROR PROC FAR
    push ax
    xchg WORD PTR $$RTL_FATAL_ERROR, si
    mov ax, ds
    xchg WORD PTR $$RTL_FATAL_ERROR+2, ax
    mov ds, ax
    pop ax
    ret
$$RTLSETERROR ENDP

$$RTL_FATAL_ERROR DD default_error
$$RTL_ORG_TERMINATE DD 0                ; terminate address found in the PSP
$$RTL_TERM_PSP DW 0

; Default handler. DS:BX is a zero-terminated table of near string pointers; they
; are printed to standard error behind the common prefix, then the program ends.
default_error PROC FAR
    push bx
    mov bx, 2
    mov cx, 1
    mov ah, 46h                         ; standard output becomes standard error
    int 21h
    pop bx
    push ds
    mov ax, cs
    mov ds, ax
    mov dx, $$RTLCOMMON_MESSAGES
    mov ah, 9
    int 21h
    pop ds
next_string:
    mov si, [bx]
    or si, si
    jz strings_done
next_character:
    lodsb
    or al, al
    jz string_done
    mov dl, al
    mov ah, 2
    int 21h
    jmp next_character
string_done:
    add bx, 2
    jmp next_string
strings_done:
    mov ax, cs
    mov ds, ax
    mov ah, 0fh                         ; wait for a key only in a graphics mode
    int 10h
    cmp al, 3
    jbe exit_program
    cmp al, 7
    jz exit_program
    mov dx, $$RTLCOMMON_MESSAGES+2
    mov ah, 9
    int 21h
    mov ah, 7
    int 21h
exit_program:
    mov ax, 4c01h
    int 21h
default_error ENDP

; For a program that ends without DOS's terminate call: run the routines
; registered for it and give the PSP its terminate address back.
$$RTL_TERMINATE PROC FAR
    push ax
    push es
    mov al, 1
    call run_chain
    mov es, $$RTL_TERM_PSP
    cli
    mov ax, WORD PTR $$RTL_ORG_TERMINATE
    mov es:[0ah], ax
    mov ax, WORD PTR $$RTL_ORG_TERMINATE+2
    mov es:[0ch], ax
    mov psp_hooked, 0
    sti
    pop es
    pop ax
    ret
$$RTL_TERMINATE ENDP

; ES is the PSP: remember it and its terminate address, once.
$$RTL_TERM_INIT PROC FAR
    push ax
    mov $$RTL_TERM_PSP, es
    mov ax, WORD PTR $$RTL_ORG_TERMINATE
    or ax, WORD PTR $$RTL_ORG_TERMINATE+2
    jnz already_saved
    cli
    mov ax, es:[0ah]
    mov WORD PTR $$RTL_ORG_TERMINATE, ax
    mov ax, es:[0ch]
    mov WORD PTR $$RTL_ORG_TERMINATE+2, ax
    sti
already_saved:
    pop ax
    ret
$$RTL_TERM_INIT ENDP

; Register the routine DS:DX for the events in AL: 1 $$RTL_TERMINATE, 2 normal
; termination, 4 preload.
$$RTL_REGISTER PROC FAR
    push bx
    push ax
    mov bx, OFFSET chain
find_free:
    mov ax, cs:[bx]
    or ax, cs:[bx+2]
    jz found_free
    add bx, 6
    jmp find_free

psp_hooked DW 0

found_free:
    test psp_hooked, 0ffh
    jnz store_routine
    pop ax
    push ax
    test al, 2
    jz store_routine
    mov psp_hooked, 0ffh
    push es
    mov es, $$RTL_TERM_PSP
    cli
    mov WORD PTR es:[0ah], OFFSET normal_termination
    mov es:[0ch], cs
    sti
    pop es
store_routine:
    cmp bx, OFFSET chain_end
    je chain_full
    mov cs:[bx], dx
    mov cs:[bx+2], ds
    pop ax
    mov cs:[bx+4], al
    pop bx
    ret
chain_full:
    call internal_error

chain DW 24 DUP (0)                     ; eight routines: far address, event word
chain_end DW 0, 0
$$RTL_REGISTER ENDP

; Report an internal error through the current handler.
internal_error PROC FAR
    mov ax, cs
    mov ds, ax
    mov bx, $$RTLCOMMON_MESSAGES+4
    mov error_strings, bx
    mov bx, $$RTLCOMMON_MESSAGES+6
    mov error_strings+2, bx
    mov error_strings+4, 0
    mov bx, OFFSET error_strings
    mov ah, 6
    mov al, 1
    xor cx, cx
    jmp $$RTL_FATAL_ERROR

error_strings DW 0, 0, 0
internal_error ENDP

$$RTL_PRELOAD PROC FAR
    push ax
    push es
    mov al, 4
    call run_chain
    pop es
    pop ax
    ret
$$RTL_PRELOAD ENDP

; The hooked terminate address: run the chain, restore the PSP, go on to DOS.
normal_termination PROC FAR
    push ax
    push es
    mov al, 2
    call run_chain
    mov es, $$RTL_TERM_PSP
    cli
    mov ax, WORD PTR $$RTL_ORG_TERMINATE
    mov es:[0ah], ax
    mov ax, WORD PTR $$RTL_ORG_TERMINATE+2
    mov es:[0ch], ax
    mov psp_hooked, 0
    sti
    pop es
    pop ax
    jmp $$RTL_ORG_TERMINATE
normal_termination ENDP

; Call every registered routine whose events include AL.
run_chain PROC NEAR
    push bx
    push dx
    mov bx, OFFSET chain
next_routine:
    mov dx, cs:[bx]
    or dx, cs:[bx+2]
    jz chain_done
    test cs:[bx+4], al
    jz skip_routine
    pushf
    call DWORD PTR cs:[bx]
    popf
skip_routine:
    add bx, 6
    jmp next_routine
chain_done:
    pop dx
    pop bx
    ret
run_chain ENDP
$$OVLMGR_UTILS ENDS
END

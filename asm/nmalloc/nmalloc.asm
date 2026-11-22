; DOS near-heap entry points in the shared C runtime code segment.

HEAP_END        EQU 6
FIRST_FREE      EQU 8
FREE_BIT        EQU 1
MAX_REQUEST     EQU 0FFE8h

_DATA SEGMENT WORD PUBLIC 'DATA'
EXTRN __nheap_desc:BYTE
; Two separately bound words of the same far callback slot.
EXTRN allocation_handler:DWORD
EXTRN allocation_handler_segment:WORD
_DATA ENDS
DGROUP GROUP _DATA

_TEXT SEGMENT WORD PUBLIC 'CODE'
ASSUME CS:_TEXT, DS:DGROUP
EXTRN __searchseg:NEAR
EXTRN __growseg:NEAR
PUBLIC __nfree, __nmalloc

__nfree PROC FAR
    push bp
    mov bp, sp
    push si
    mov bx, [bp+6]
    mov si, OFFSET DGROUP:__nheap_desc
    cmp [si+HEAP_END], bx
    jae SHORT free_done
    ; Back up from the payload to its two-byte block header.
    dec bx
    dec bx
    or BYTE PTR [bx], FREE_BIT
    cmp [si+FIRST_FREE], bx
    jbe SHORT free_done
    mov [si+FIRST_FREE], bx
free_done:
    pop si
    mov sp, bp
    pop bp
    retf
__nfree ENDP

__nmalloc PROC FAR
    push bp
    mov bp, sp
    push si
    push di
    mov cx, [bp+6]
    cmp cx, MAX_REQUEST
    ja SHORT ask_handler
    mov bx, OFFSET DGROUP:__nheap_desc
    ; The runtime helpers take BX and CX. Carry signals failure.
    call __searchseg
    jnc SHORT malloc_done
    call __growseg
    jc SHORT ask_handler
retry_search:
    call __searchseg
    jnc SHORT malloc_done
ask_handler:
    mov ax, WORD PTR allocation_handler
    or ax, allocation_handler_segment
    jz SHORT allocation_failed
    push bx
    push cx
    call allocation_handler
    pop cx
    pop bx
    test ax, ax
    jnz SHORT retry_search
allocation_failed:
    cwd
malloc_done:
    pop di
    pop si
    pop bp
    retf
__nmalloc ENDP
_TEXT ENDS

END

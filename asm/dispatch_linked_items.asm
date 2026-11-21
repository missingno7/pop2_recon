.MODEL MEDIUM
.CODE
PUBLIC DISPATCH_LINKED_ITEMS

DISPATCH_LINKED_ITEMS PROC FAR
    push bp
    mov bp, sp
    push di
    mov di, [bp+8]
    lea di, [di+52h]
next_item:
    mov di, [di]
    test di, di
    jz linked_items_done
    push di
    mov ax, 3
    push ax
    push WORD PTR [bp+6]
    call DWORD PTR [di+4]
    jmp SHORT next_item
linked_items_done:
    pop di
    pop bp
    retf 4
DISPATCH_LINKED_ITEMS ENDP
END

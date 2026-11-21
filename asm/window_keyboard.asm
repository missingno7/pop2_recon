.MODEL MEDIUM
.CODE
EXTRN STEP_SELECTABLE_ITEM:NEAR
EXTRN SET_SELECTED_ITEM:NEAR
EXTRN UPPERCASE_EVENT_CHARACTER:NEAR
PUBLIC NOTIFY_SELECTED_ITEM, SELECTED_ITEM_ORDINAL, DISPATCH_WINDOW_KEY

NOTIFY_SELECTED_ITEM PROC FAR
    push bp
    mov bp, sp
    mov bx, [bp+6]
    mov bx, [bx+52h]
    test bx, bx
    jz notify_done
    push bx
    mov ax, 5
    push ax
    sub ax, ax
    push ax
    call DWORD PTR [bx+4]
notify_done:
    pop bp
    retf 2
NOTIFY_SELECTED_ITEM ENDP

SELECTED_ITEM_ORDINAL PROC FAR
    push bp
    mov bp, sp
    sub ax, ax
    mov cx, [bp+6]
    jcxz ordinal_done
    mov bx, cx
    mov bx, [bx+2]
    lea bx, [bx+50h]
count_item:
    inc ax
    mov bx, [bx]
    cmp bx, cx
    jne count_item
ordinal_done:
    pop bp
    retf 2
SELECTED_ITEM_ORDINAL ENDP

DISPATCH_WINDOW_KEY PROC FAR
    push bp
    mov bp, sp
    push di
    push si
    mov di, [bp+6]
    mov si, [bp+8]
    mov si, [si+52h]
    test si, si
    jz scan_accelerators
    push si
    mov ax, 7
    push ax
    push WORD PTR [di]
    call DWORD PTR [si+4]
    mov [di], ax
    cmp al, 0dh
    je window_key_done
    mov dx, 1
    cmp al, 9
    je step_selection
    neg dx
    cmp ax, 0f00h
    jne scan_accelerators
step_selection:
    push si
    push dx
    push cs
    call STEP_SELECTABLE_ITEM
    push WORD PTR [si+2]
    push ax
    push cs
    call SET_SELECTED_ITEM
    sub si, si
    mov [di], si
    jmp SHORT window_key_done
scan_accelerators:
    push WORD PTR [di]
    push cs
    call UPPERCASE_EVENT_CHARACTER
    mov [bp+6], ax
    mov si, [bp+8]
    add si, 50h
scan_next_item:
    mov si, [si]
    test si, si
    jz window_key_done
    mov cx, [si+1ah]
    jcxz scan_next_item
    mov cx, [si+1ch]
    jcxz scan_next_item
    mov cx, [si+14h]
    jcxz scan_next_item
    push cx
    push cs
    call UPPERCASE_EVENT_CHARACTER
    cmp ax, [bp+6]
    jne scan_next_item
    push si
    mov ax, 7
    push ax
    push WORD PTR [di]
    call DWORD PTR [si+4]
    mov [di], ax
window_key_done:
    xchg si, ax
    pop si
    pop di
    pop bp
    retf 4
DISPATCH_WINDOW_KEY ENDP
END

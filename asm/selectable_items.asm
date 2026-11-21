.MODEL MEDIUM
.CODE
EXTRN HGETRSRC:NEAR
EXTRN RESOLVE_HANDLE_TO_FAR_PAYLOAD:NEAR
EXTRN STROKE_ITEM_EDGES:NEAR
PUBLIC ITEM_BY_INDEX, LOAD_ITEM_DEFINITION, LOAD_ITEM_LIST
PUBLIC STEP_SELECTABLE_ITEM, SET_SELECTED_ITEM, SELECT_ITEM_BY_INDEX

ITEM_BY_INDEX PROC FAR
    push bp
    mov bp, sp
    sub ax, ax
    mov bx, [bp+8]
    lea bx, [bx+50h]
next_item:
    mov bx, [bx]
    test bx, bx
    jz return_indexed_item
    inc ax
    cmp ax, [bp+6]
    jne next_item
return_indexed_item:
    xchg bx, ax
    pop bp
    retf 4
ITEM_BY_INDEX ENDP

LOAD_ITEM_DEFINITION PROC FAR
    push bp
    mov bp, sp
    push di
    push si
    sub di, di
    mov si, [bp+6]
    mov ax, 4344h
    push ax
    mov ax, 4546h
    push ax
    les bx, DWORD PTR [si]
    push WORD PTR es:[bx]
    push cs
    call HGETRSRC
    push ax
    push cs
    call RESOLVE_HANDLE_TO_FAR_PAYLOAD
    xchg bx, ax
    mov es, dx
    or dx, bx
    jz definition_done
    push di
    mov ax, 0
    push ax
    push si
    call DWORD PTR es:[bx]
    xchg di, ax
    test di, di
    jz definition_done
    mov bx, [bp+8]
    mov [di+2], bx
    add bx, 50h
find_last_item:
    mov cx, [bx]
    jcxz append_item
    mov bx, cx
    jmp SHORT find_last_item
append_item:
    mov [bx], di
    mov [di], cx
    push di
    mov ax, 1
    push ax
    push cx
    call DWORD PTR [di+4]
definition_done:
    xchg di, ax
    pop si
    pop di
    pop bp
    retf 4
LOAD_ITEM_DEFINITION ENDP

LOAD_ITEM_LIST PROC FAR
    push bp
    mov bp, sp
    push di
    les bx, DWORD PTR [bp+6]
    mov di, es:[bx]
    add WORD PTR [bp+6], 2
load_next_item:
    dec di
    jl item_list_done
    push WORD PTR [bp+10]
    lea ax, [bp+6]
    push ax
    push cs
    call NEAR PTR LOAD_ITEM_DEFINITION
    jmp SHORT load_next_item
item_list_done:
    pop di
    pop bp
    retf 6
LOAD_ITEM_LIST ENDP

STEP_SELECTABLE_ITEM PROC FAR
    push bp
    mov bp, sp
    push di
    push si
    mov bx, [bp+8]
    test bx, bx
    jz return_item
    mov di, [bx+2]
    mov si, [bp+6]
check_remaining_steps:
    test si, si
    jl step_previous
    jg step_next
return_item:
    xchg bx, ax
    pop si
    pop di
    pop bp
    retf 4
step_next:
    mov bx, [bx]
    test bx, bx
    jnz check_next
    mov bx, [di+50h]
check_next:
    mov cx, [bx+1ah]
    jcxz step_next
    mov cx, [bx+1ch]
    jcxz step_next
    dec si
    jmp SHORT check_remaining_steps
step_previous:
    xchg bx, ax
    lea bx, [di+50h]
    cmp [bx], ax
    jne find_previous
    sub ax, ax
find_previous:
    cmp [bx], ax
    je check_previous
    mov bx, [bx]
    jmp SHORT find_previous
check_previous:
    mov cx, [bx+1ah]
    jcxz step_previous
    mov cx, [bx+1ch]
    jcxz step_previous
    inc si
    jmp SHORT check_remaining_steps
STEP_SELECTABLE_ITEM ENDP

SET_SELECTED_ITEM PROC FAR
    push bp
    mov bp, sp
    push di
    push si
    mov bx, [bp+8]
    mov di, [bp+6]
    mov si, [bx+52h]
    cmp di, si
    je selected_item_ready
    test di, di
    jz test_previous_item
    mov cx, [di+1ah]
    jcxz selected_item_ready
    mov cx, [di+1ch]
    jcxz selected_item_ready
test_previous_item:
    test si, si
    jz store_selected_item
    mov bx, si
    call STROKE_ITEM_EDGES
    push si
    mov ax, 4
    push ax
    sub ax, ax
    push ax
    call DWORD PTR [si+4]
store_selected_item:
    mov bx, [bp+8]
    mov [bx+52h], di
    test di, di
    jz selected_item_ready
    mov bx, di
    call STROKE_ITEM_EDGES
    push di
    mov ax, 4
    push ax
    mov ax, 1
    push ax
    call DWORD PTR [di+4]
selected_item_ready:
    mov ax, [bx+52h]
    pop si
    pop di
    pop bp
    retf 4
SET_SELECTED_ITEM ENDP

SELECT_ITEM_BY_INDEX PROC FAR
    push bp
    mov bp, sp
    push WORD PTR [bp+8]
    push WORD PTR [bp+6]
    push cs
    call NEAR PTR ITEM_BY_INDEX
    push WORD PTR [bp+8]
    push ax
    push cs
    call NEAR PTR SET_SELECTED_ITEM
    pop bp
    retf 4
SELECT_ITEM_BY_INDEX ENDP
END

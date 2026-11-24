; Repaint grid cells and update their indexed selection.
.MODEL MEDIUM
.CODE
EXTRN select_grid_item:NEAR
EXTRN grid_index_to_coordinates:NEAR
EXTRN draw_grid_items:NEAR

PUBLIC PAINT_GRID_ITEMS
PUBLIC DRAW_INDEXED_ITEM

; Visit the grid row by row, saving loop coordinates around each callback.
PAINT_GRID_ITEMS PROC FAR
    push bp
    mov bp, sp
    push di
    mov di, word ptr [bp+6]
    sub dx, dx
start_grid_row:
    sub cx, cx
paint_next_grid_cell:
    push cx
    push dx
    push di
    add cx, word ptr [di+2ch]
    push cx
    add dx, word ptr [di+2ah]
    push dx
    call draw_grid_items
    pop dx
    pop cx
    inc cx
    cmp cx, word ptr [di+28h]
    jl paint_next_grid_cell
    inc dx
    cmp dx, word ptr [di+26h]
    jl start_grid_row
    pop di
    pop bp
    retf 2
PAINT_GRID_ITEMS ENDP

DRAW_INDEXED_ITEM PROC FAR
    push bp
    mov bp, sp
    push di
    mov di, word ptr [bp+0ah]
    mov cx, word ptr [bp+8]
    jcxz return_grid_cell_update
    cmp cx, word ptr [di+30h]
    jg return_grid_cell_update
    push cx
    mov ax, 5
    mul cx
    xchg bx, ax
    and byte ptr [bx+di+2dh], 0feh
    cmp word ptr [bp+6], 0
    jne repaint_indexed_grid_cell
    or byte ptr [bx+di+2dh], 1
    sub cx, word ptr [di+2eh]
    jne repaint_indexed_grid_cell
    push di
    push cx
    push cs
    call select_grid_item
repaint_indexed_grid_cell:
    pop ax
    call grid_index_to_coordinates
    push di
    push dx
    push ax
    call draw_grid_items
return_grid_cell_update:
    pop di
    pop bp
    retf 6
DRAW_INDEXED_ITEM ENDP

END

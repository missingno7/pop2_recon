.MODEL MEDIUM
EXTRN LEGACY_CATALOG:BYTE
.DATA
EXTRN SHAPE_METADATA_LIST:WORD
.CODE
EXTRN RESOLVE_HANDLE_TO_FAR_PAYLOAD:NEAR
PUBLIC FIND_SHAPE_METADATA

FIND_SHAPE_METADATA PROC NEAR
    push bp
    mov si, OFFSET SHAPE_METADATA_LIST
    push ss
    pop ds
next_handle:
    mov cx, [si]
    jcxz legacy_catalog_search
    xchg ax, si
    push cx
    push cs
    call RESOLVE_HANDLE_TO_FAR_PAYLOAD
    xchg ax, si
    mov ds, dx
    lea bp, [si+52h]
    sub cx, cx
next_group:
    cmp cx, [si+50h]
    jz next_handle
    lea bx, [si+50h]
    add bx, ds:[bp+4]
    sub dx, dx
    lea di, [bx-9]
next_record:
    cmp dx, [bx]
    jz advance_group
    inc dx
    add di, 0bh
    cmp ax, [di+9]
    jne next_record
    mov bx, bp
done:
    pop bp
    ret
advance_group:
    add bp, 6
    inc cx
    jmp SHORT next_group

legacy_catalog_search:
    xchg ax, cx
    mov bx, SEG LEGACY_CATALOG
    mov ds, bx
    mov bx, 0fffbh
legacy_record:
    add bx, 0fh
    cmp bx, 145h
    je done
    cmp cx, [bx+0dh]
    jne legacy_record
    xchg ax, cx
    lea di, [bx+4]
    mov si, 6
    jmp SHORT done
FIND_SHAPE_METADATA ENDP
END

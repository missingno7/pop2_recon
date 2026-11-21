.MODEL MEDIUM
.DATA
saved_vectors DD 0, 0, 0, 0
EXTRN MOUSE_PRESENT:WORD
EXTRN event_count:WORD
EXTRN event_records:BYTE
.CODE
EXTRN mouse_button_down:NEAR
PUBLIC mouse_button_unqueued


PUBLIC RESTORE_DOS_VECTORS
RESTORE_DOS_VECTORS PROC FAR
    push ds
    lds dx, saved_vectors
    mov ax, 2510h
    int 21h
    lds dx, DWORD PTR ss:[saved_vectors+4]
    mov ax, 2510h
    int 21h
    lds dx, DWORD PTR ss:[saved_vectors+8]
    mov ax, 2516h
    int 21h
    lds dx, DWORD PTR ss:[saved_vectors+12]
    mov ax, 2524h
    int 21h
    mov cx, ss:[MOUSE_PRESENT]
    jcxz vectors_done
    sub ax, ax
    int 33h
vectors_done:
    pop ds
    retf
RESTORE_DOS_VECTORS ENDP

mouse_button_unqueued PROC FAR
    push bp
    mov bp, sp
    push WORD PTR [bp+6]
    push cs
    call mouse_button_down
    test ax, ax
    je done
    mov cx, event_count
    jcxz done
    mov bx, OFFSET event_records
    mov dx, WORD PTR [bp+6]
next_record:
    cmp WORD PTR [bx], 4
    jne advance
    cmp dx, WORD PTR [bx+10]
    jne advance
    sub ax, ax
    jmp short done
advance:
    add bx, 14
    loop next_record
done:
    pop bp
    retf 2
mouse_button_unqueued ENDP
END

.MODEL MEDIUM
.DATA
EXTRN event_count:WORD
EXTRN event_records:BYTE
.CODE
EXTRN mouse_button_down:NEAR
PUBLIC mouse_button_unqueued

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

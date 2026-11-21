.MODEL MEDIUM
.CODE
PUBLIC EMPTYRECT

EMPTYRECT PROC FAR
    push bp
    mov bp, sp
    mov ax, 1
    les bx, [bp+6]
    mov cx, es:[bx+2]
    cmp cx, es:[bx+6]
    jge done
    mov dx, es:[bx]
    cmp dx, es:[bx+4]
    jge done
    sub ax, ax
done:
    pop bp
    retf 4
EMPTYRECT ENDP
END

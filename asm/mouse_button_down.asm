.MODEL MEDIUM
.DATA
EXTRN mouse_present:WORD
.CODE
PUBLIC MOUSE_BUTTON_DOWN

MOUSE_BUTTON_DOWN PROC FAR
    push bp
    mov bp, sp
    sub ax, ax
    cmp ax, mouse_present
    je done

    ; Mouse driver function 3 returns the current button bits in BX.
    mov ax, 3
    int 33h
    sub ax, ax
    stc
    mov cl, BYTE PTR [bp+6]
    rcl ax, cl
    and ax, bx
    je done
    mov ax, 1
done:
    pop bp
    retf 2
MOUSE_BUTTON_DOWN ENDP
END

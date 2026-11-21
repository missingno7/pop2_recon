.MODEL MEDIUM
.CODE
EXTRN FIND_RESOURCE_ENTRY:NEAR
EXTRN MATERIALIZE_RESOURCE:NEAR
PUBLIC HGETRSRC

HGETRSRC PROC FAR
    push bp
    mov bp, sp
    push di
    push si
    push ds

    les ax, DWORD PTR [bp+8]
    mov dx, es
    mov cx, WORD PTR [bp+6]
    call FIND_RESOURCE_ENTRY
    sbb ax, ax
    je done
    call MATERIALIZE_RESOURCE
done:
    pop ds
    pop si
    pop di
    pop bp
    retf 6
HGETRSRC ENDP
END

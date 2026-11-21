.MODEL MEDIUM
.CODE
EXTRN FIND_RESOURCE_ENTRY:NEAR
EXTRN DISPOSE_HANDLE:NEAR
PUBLIC RELEASE_TAGGED_RESOURCE

RELEASE_TAGGED_RESOURCE PROC FAR
    push bp
    mov bp, sp
    push di
    push si
    push ds
    les ax, DWORD PTR [bp+8]
    mov dx, es
    mov cx, [bp+6]
    call FIND_RESOURCE_ENTRY
    jnc done
    sub cx, cx
    xchg cx, [di+9]
    jcxz done
    push cx
    push cs
    call DISPOSE_HANDLE
done:
    pop ds
    pop si
    pop di
    pop bp
    retf 6
RELEASE_TAGGED_RESOURCE ENDP
END

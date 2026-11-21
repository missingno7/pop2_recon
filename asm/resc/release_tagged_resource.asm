.MODEL MEDIUM
.CODE
EXTRN FIND_RESOURCE_ENTRY:NEAR
EXTRN DISPOSE_HANDLE:NEAR
EXTRN FIND_SHAPE_METADATA:NEAR
EXTRN MATERIALIZE_RESOURCE:NEAR
PUBLIC UNLINK_SHAPE_METADATA, HGETRSRC
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

UNLINK_SHAPE_METADATA PROC FAR
    push bp
    mov bp, sp
    push di
    push si
    push ds
    mov ax, [bp+6]
    call FIND_SHAPE_METADATA
    test ax, ax
    jz unlink_done
    mov WORD PTR [di+9], 0
    xchg ax, bp
    and WORD PTR [bp], 0efffh
unlink_done:
    pop ds
    pop si
    pop di
    pop bp
    retf 2
UNLINK_SHAPE_METADATA ENDP

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
    je get_resource_done
    call MATERIALIZE_RESOURCE
get_resource_done:
    pop ds
    pop si
    pop di
    pop bp
    retf 6
HGETRSRC ENDP
END

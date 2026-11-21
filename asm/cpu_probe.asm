CPU_TEXT SEGMENT WORD PUBLIC 'CODE'
        ASSUME CS:CPU_TEXT
        PUBLIC CPU_PROBE

; Return the numeric processor class while preserving the caller's FLAGS.
CPU_PROBE PROC FAR
        pushf
        xor     ax, ax
        push    ax
        popf
        pushf
        pop     ax
        and     ax, 8000h
        cmp     ax, 8000h
        je      pre286

        mov     ax, 7000h
        push    ax
        popf
        pushf
        pop     ax
        and     ax, 7000h
        mov     ax, 286
        je      finished
        mov     ax, 386
        jmp     short finished

pre286:
        mov     ax, 0ffffh
        mov     cl, 32
        shl     ax, cl
        mov     ax, 186
        jne     finished
        mov     ax, 86

finished:
        popf
        retf
CPU_PROBE ENDP
CPU_TEXT ENDS
        END

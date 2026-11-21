.MODEL MEDIUM
.CODE
EXTRN START_SPEAKER_PAYLOAD:NEAR
EXTRN START_DIGITAL_PAYLOAD:NEAR
EXTRN START_MIDI_PAYLOAD:NEAR
PUBLIC START_SOUND_PAYLOAD

START_SOUND_PAYLOAD PROC FAR
    push bp
    mov bp, sp
    les bx, [bp+0ah]
    mov ax, es
    or ax, bx
    jz done
    mov al, es:[bx]
    and al, 7fh
    cmp al, 1
    je digital_payload
    cmp al, 2
    je midi_payload
    cmp al, 0
    je speaker_payload
    xor ax, ax
done:
    pop bp
    retf 8

speaker_payload:
    push es
    push bx
    les ax, [bp+6]
    push es
    push ax
    call START_SPEAKER_PAYLOAD
    jmp short done

digital_payload:
    push es
    push bx
    les ax, [bp+6]
    push es
    push ax
    call START_DIGITAL_PAYLOAD
    jmp short done

midi_payload:
    push es
    push bx
    les ax, [bp+6]
    push es
    push ax
    call START_MIDI_PAYLOAD
    jmp short done
START_SOUND_PAYLOAD ENDP
END

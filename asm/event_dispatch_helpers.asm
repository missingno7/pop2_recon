.MODEL MEDIUM
.DATA
EXTRN WINDOW_LIST:BYTE
event_hook DD 0
.CODE
PUBLIC FIND_EVENT_WINDOW, NOTIFY_EVENT_HOOK

FIND_EVENT_WINDOW PROC NEAR
    mov cx, OFFSET WINDOW_LIST
next_window:
    mov bx, cx
    mov cx, [bx+2ch]
    jcxz lookup_done
    cmp cx, [di+0ah]
    jne next_window
lookup_done:
    ret
FIND_EVENT_WINDOW ENDP

NOTIFY_EVENT_HOOK PROC NEAR
    mov ax, WORD PTR event_hook
    or ax, WORD PTR [event_hook+2]
    jz notification_done
    push di
    call event_hook
notification_done:
    ret
NOTIFY_EVENT_HOOK ENDP
END

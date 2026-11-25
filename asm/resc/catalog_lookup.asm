.MODEL MEDIUM
EXTRN LEGACY_CATALOG:BYTE
.DATA
PUBLIC SHAPE_METADATA_LIST, RESOURCE_CATALOG_LIST, CATALOG_CALLBACK
SHAPE_METADATA_LIST DW 0
RESOURCE_CATALOG_LIST DW 0
CATALOG_CALLBACK DD 0
PUBLIC resident_handle_flags
catalog_open_mode DB 0
; The built-in catalog uses the zero offset word as its persistent handle flags.
resident_handle_flags DW 0
                      DW SEG LEGACY_CATALOG
.CODE
EXTRN CLEAR_PAYLOAD_HEADER:NEAR
EXTRN HUNLOCK:NEAR
EXTRN CHECK_DOS_DRIVE:NEAR
EXTRN QUALIFY_DOS_PATH:NEAR
EXTRN ALLOCATE_PINNED_PAYLOAD:NEAR
EXTRN MATERIALIZE_HANDLE:NEAR
EXTRN HGETSIZE:NEAR
EXTRN REPLACE_HANDLE_PAYLOAD:NEAR
EXTRN HNEW:NEAR
EXTRN HLOCK:NEAR
EXTRN RELOCATE_HANDLE_PAYLOAD:NEAR
EXTRN HPURGE:NEAR
EXTRN HFREE:NEAR
EXTRN MOVE_HUGE_BYTES:NEAR
PUBLIC QUERY_RESOURCE_PAYLOAD, MATERIALIZE_RESOURCE, READ_RESOURCE_PAYLOAD, FIND_RESOURCE_ENTRY
EXTRN H2P:NEAR
PUBLIC FIND_SHAPE_METADATA

PUBLIC OPEN_CATALOG_FILE, CLOSE_CATALOG_FILE

PUBLIC LOAD_RESOURCE_PINNED, READRSRC, CLOSEMAP, OPENMAP

LOAD_RESOURCE_PINNED PROC FAR
    push bp
    mov bp, sp
    add sp, -8
    push di
    push si
    push ds
    sub ax, ax
    mov [bp-4], ax
    mov [bp-2], ax
    les ax, DWORD PTR [bp+8]
    mov dx, es
    mov cx, [bp+6]
    call FIND_RESOURCE_ENTRY
    jnc pinned_done
    call QUERY_RESOURCE_PAYLOAD
    mov [bp-8], dx
    mov [bp-6], cx
    push ax
    mov bx, [si-2]
    push WORD PTR ss:[bx]
    push bx
    or WORD PTR ss:[bx], 8000h
    push cx
    push dx
    push cs
    call ALLOCATE_PINNED_PAYLOAD
    mov [bp-4], ax
    mov [bp-2], dx
    pop bx
    pop WORD PTR ss:[bx]
    pop cx
    or ax, dx
    jz pinned_done
    jcxz pinned_file
    les ax, DWORD PTR [bp-4]
    push es
    push ax
    push cx
    push cs
    call H2P
    push dx
    push ax
    les ax, DWORD PTR [bp-8]
    push es
    push ax
    push cs
    call MOVE_HUGE_BYTES
pinned_done:
    les ax, DWORD PTR [bp-4]
    mov dx, es
    pop ds
    pop si
    pop di
    mov sp, bp
    pop bp
    retf 6
pinned_file:
    les dx, DWORD PTR [bp-8]
    mov cx, es
    les bx, DWORD PTR [bp-4]
    call READ_RESOURCE_PAYLOAD
    test ax, ax
    jz pinned_done
    jl pinned_file
    sub ax, ax
    cwd
    xchg ax, [bp-4]
    xchg dx, [bp-2]
    push dx
    push ax
    push cs
    call CLEAR_PAYLOAD_HEADER
    jmp SHORT pinned_done
LOAD_RESOURCE_PINNED ENDP

READRSRC PROC FAR
    push bp
    mov bp, sp
    push di
    push si
    push ds
    les ax, DWORD PTR [bp+16]
    mov dx, es
    mov cx, [bp+14]
    call FIND_RESOURCE_ENTRY
    jnc into_failed
    call QUERY_RESOURCE_PAYLOAD
    cmp cx, [bp+8]
    ja check_limit
    jb use_length
    cmp dx, [bp+6]
    jbe use_length
check_limit:
    mov bx, [bp+6]
    or bx, [bp+8]
    jnz into_failed
use_length:
    mov [bp+6], dx
    mov [bp+8], cx
    xchg cx, ax
into_target:
    les bx, DWORD PTR [bp+10]
    jcxz into_file
    push es
    push bx
    push cx
    push cs
    call H2P
    push dx
    push ax
    les ax, DWORD PTR [bp+6]
    push es
    push ax
    push cs
    call MOVE_HUGE_BYTES
into_result:
    les ax, DWORD PTR [bp+6]
    mov dx, es
into_done:
    pop ds
    pop si
    pop di
    pop bp
    retf 14
into_file:
    call READ_RESOURCE_PAYLOAD
    test ax, ax
    jz into_result
    jg into_failed
    sub cx, cx
    jmp SHORT into_target
into_failed:
    sub ax, ax
    cwd
    jmp SHORT into_done
READRSRC ENDP

CLOSEMAP PROC FAR
    push bp
    mov bp, sp
    push di
    push si
    push ds
    mov si, OFFSET SHAPE_METADATA_LIST
next_close_catalog:
    mov di, [si]
    test di, di
    jz close_resource_done
    push di
    push cs
    call H2P
    cmp di, [bp+6]
    je remove_catalog
    xchg ax, si
    mov ds, dx
    jmp SHORT next_close_catalog
remove_catalog:
    xchg ax, bx
    mov es, dx
    mov ax, es:[bx]
    mov [si], ax
    cmp di, RESOURCE_CATALOG_LIST
    jne close_file
    mov RESOURCE_CATALOG_LIST, ax
close_file:
    mov cx, es:[bx+2]
    jcxz release_children
    mov bx, cx
    mov ah, 3eh
    int 21h
release_children:
    push di
    push di
    push cs
    call HLOCK
    xchg ax, si
    mov ds, dx
    push bp
    sub bp, bp
next_type:
    cmp bp, [si+50h]
    je release_catalog
    mov bx, bp
    shl bx, 1
    add bx, bp
    shl bx, 1
    lea di, [si+50h]
    add di, [bx+si+56h]
    push bp
    sub bp, bp
next_resource:
    cmp bp, [di]
    je next_type_index
    mov bx, bp
    shl bx, 1
    shl bx, 1
    add bx, bp
    shl bx, 1
    add bx, bp
    mov cx, [bx+di+0bh]
    jcxz next_resource_index
    push cx
    push cs
    call HFREE
next_resource_index:
    inc bp
    jmp SHORT next_resource
next_type_index:
    pop bp
    inc bp
    jmp SHORT next_type
release_catalog:
    pop bp
    push cs
    call HFREE
close_resource_done:
    pop ds
    pop si
    pop di
    pop bp
    retf 2
CLOSEMAP ENDP

OPENMAP PROC FAR
    push bp
    mov bp, sp
    add sp, -78
    push di
    push si
    push ds
    mov WORD PTR [bp-10], 0
    lds dx, DWORD PTR [bp+6]
    mov al, CATALOG_OPEN_MODE
    mov ah, 3dh
    int 21h
    jc mount_done
    push ss
    pop ds
    mov [bp-2], ax
    xchg ax, bx
    mov cx, 6
    lea dx, [bp-8]
    mov ah, 3fh
    int 21h
    jc mount_close_file
    sub ax, cx
    jne mount_close_file
    push ax
    mov ax, [bp-4]
    add ax, 50h
    push ax
    push cs
    call HNEW
    xchg ax, cx
    jcxz mount_close_file
    mov [bp-10], cx
    push cx
    push cs
    call HLOCK
    mov [bp-14], ax
    mov [bp-12], dx
    mov bx, [bp-2]
    les dx, DWORD PTR [bp-8]
    mov cx, es
    mov ax, 4200h
    int 21h
    jnc read_catalog_table
mount_free_handle:
    sub ax, ax
    xchg ax, [bp-10]
    push ax
    push cs
    call HFREE
mount_close_file:
    mov bx, [bp-2]
    mov ah, 3eh
    int 21h
mount_done:
    mov ax, [bp-10]
    pop ds
    pop si
    pop di
    mov sp, bp
    pop bp
    retf 4
read_catalog_table:
    lds si, DWORD PTR [bp-14]
    mov [si+2], bx
    mov cx, [bp-4]
    lea dx, [si+50h]
    mov ah, 3fh
    int 21h
    jc mount_free_handle
    sub ax, cx
    jne mount_free_handle
    lea ax, [si+10h]
    push ds
    push ax
    les ax, DWORD PTR [bp+6]
    push es
    push ax
    push ss
    pop ds
    push cs
    call QUALIFY_DOS_PATH
    test ax, ax
    jne mount_free_handle
    les bx, DWORD PTR [bp-14]
    lea ax, [bx+4]
    push es
    push ax
    mov al, es:[bx+10h]
    cbw
    push ax
    push cs
    call CHECK_DOS_DRIVE
    test ax, ax
    jne mount_free_handle
    mov ax, [bp-10]
    push ax
    mov RESOURCE_CATALOG_LIST, ax
    xchg ax, SHAPE_METADATA_LIST
    lds si, DWORD PTR [bp-14]
    mov [si], ax
    call OPEN_CATALOG_FILE
    push bp
    sub bp, bp
preload_type:
    lea di, [si+50h]
    cmp bp, [di]
    je preload_done
    mov bx, di
    mov ax, 6
    mul bp
    add bx, ax
    add di, [bx+6]
    push bp
    sub bp, bp
preload_entry:
    cmp bp, [di]
    je preload_next_type
    push di
    mov ax, 11
    mul bp
    add di, ax
    add di, 2
    test BYTE PTR [di+8], 20h
    jz preload_next_entry
    call MATERIALIZE_RESOURCE
preload_next_entry:
    pop di
    inc bp
    jmp SHORT preload_entry
preload_next_type:
    pop bp
    inc bp
    jmp SHORT preload_type
preload_done:
    pop bp
    push cs
    call HUNLOCK
    jmp NEAR PTR mount_done
OPENMAP ENDP


CLOSE_CATALOG_FILE PROC NEAR
    push bp
    push ds
    mov bp, [si-2]
    push WORD PTR [bp]
    or WORD PTR [bp], 8000h
    sub cx, cx
    xchg cx, [si+2]
    jcxz check_catalog_callback
    mov bx, cx
    mov ah, 3eh
    int 21h
check_catalog_callback:
    push ss
    pop ds
    mov ax, WORD PTR CATALOG_CALLBACK
    or ax, WORD PTR CATALOG_CALLBACK+2
    jz close_done
    push bp
    call CATALOG_CALLBACK
    test ax, ax
    jz close_done
    mov ax, 1
close_done:
    pop WORD PTR [bp]
    pop ds
    pop bp
    ret
CLOSE_CATALOG_FILE ENDP

OPEN_CATALOG_FILE PROC NEAR
    push bp
    push di
    mov bp, [si-2]
    push WORD PTR [bp]
    or WORD PTR [bp], 8000h
    sub ax, ax
    cmp bp, OFFSET resident_handle_flags
    je open_done
open_file:
    cmp ax, [si+2]
    jne inspect_open_files
    lea dx, [si+10h]
    mov al, ss:[CATALOG_OPEN_MODE]
    mov ah, 3dh
    int 21h
    jc open_error
    mov [si+2], ax
inspect_open_files:
    push ss
    pop es
    mov di, OFFSET SHAPE_METADATA_LIST
next_open_file:
    mov ax, es:[di]
    test ax, ax
    jz open_done
    xchg ax, di
    push di
    push cs
    call H2P
    xchg di, ax
    mov es, dx
    cmp ax, bp
    je next_open_file
    mov al, [si+10h]
    sub al, es:[di+10h]
    jne next_open_file
    cmp al, [si+4]
    je close_other_file
    push di
    push si
    add di, 4
    add si, 4
compare_paths:
    lodsb
    scasb
    jne path_compared
    test al, al
    jne compare_paths
path_compared:
    pop si
    pop di
    je next_open_file
close_other_file:
    sub cx, cx
    xchg cx, es:[di+2]
    jcxz next_open_file
    xchg bx, cx
    mov ah, 3eh
    int 21h
    jmp SHORT next_open_file
open_error:
    push ax
    call CLOSE_CATALOG_FILE
    test ax, ax
    pop ax
    jne open_file
open_done:
    pop WORD PTR [bp]
    pop di
    pop bp
    ret
OPEN_CATALOG_FILE ENDP

FIND_SHAPE_METADATA PROC NEAR
    push bp
    mov si, OFFSET SHAPE_METADATA_LIST
    push ss
    pop ds
next_handle:
    mov cx, [si]
    jcxz legacy_catalog_search
    xchg ax, si
    push cx
    push cs
    call H2P
    xchg ax, si
    mov ds, dx
    lea bp, [si+52h]
    sub cx, cx
next_group:
    cmp cx, [si+50h]
    jz next_handle
    lea bx, [si+50h]
    add bx, ds:[bp+4]
    sub dx, dx
    lea di, [bx-9]
next_record:
    cmp dx, [bx]
    jz advance_group
    inc dx
    add di, 0bh
    cmp ax, [di+9]
    jne next_record
    mov bx, bp
done:
    pop bp
    ret
advance_group:
    add bp, 6
    inc cx
    jmp SHORT next_group

legacy_catalog_search:
    xchg ax, cx
    mov bx, SEG LEGACY_CATALOG
    mov ds, bx
    mov bx, 0fffbh
legacy_record:
    add bx, 0fh
    cmp bx, 145h
    je done
    cmp cx, [bx+0dh]
    jne legacy_record
    xchg ax, cx
    lea di, [bx+4]
    mov si, 6
    jmp SHORT done
FIND_SHAPE_METADATA ENDP

QUERY_RESOURCE_PAYLOAD PROC NEAR
    push bp
    mov bp, [si-2]
    push WORD PTR [bp]
    push bp
    or WORD PTR [bp], 8000h
    mov ax, [di+9]
    test ax, ax
    jz stored_size
    xchg ax, bp
    push bp
    push cs
    call MATERIALIZE_HANDLE
    or ax, dx
    jz stored_size
    push bp
    push cs
    call HGETSIZE
    xchg ax, dx
    xchg ax, cx
    xchg ax, bp
    jmp SHORT payload_done
stored_size:
    les dx, DWORD PTR [di+6]
    mov cx, es
    and cx, 0fh
payload_done:
    pop bp
    pop WORD PTR [bp]
    pop bp
    ret
QUERY_RESOURCE_PAYLOAD ENDP

MATERIALIZE_RESOURCE PROC NEAR
    push bp
    mov bp, [si-2]
    push WORD PTR [bp]
    push bp
    or WORD PTR [bp], 8000h
    sub bp, bp
    call QUERY_RESOURCE_PAYLOAD
    add bp, ax
    jnz materialize_done
    add bp, [di+9]
    jz allocate_handle
    push bp
    push cx
    push dx
    push cs
    call REPLACE_HANDLE_PAYLOAD
    xchg ax, cx
    jcxz read_payload
    jmp SHORT materialize_done
allocate_handle:
    push cx
    push dx
    push cs
    call HNEW
    add bp, ax
    jz materialize_done
read_payload:
    push WORD PTR [bp]
    push bp
    push cs
    call HLOCK
    xchg ax, bx
    mov es, dx
    mov cl, [di+8]
    and cx, 0fh
    mov dx, [di+6]
    call READ_RESOURCE_PAYLOAD
    pop WORD PTR [bp]
    test ax, ax
    jl read_payload
    jg discard_payload
    or WORD PTR [bp], 1000h
    cmp bp, [di+9]
    mov [di+9], bp
    je materialize_done
    test BYTE PTR [di+8], 80h
    jz check_purgeable
    push bp
    push cs
    call RELOCATE_HANDLE_PAYLOAD
    push bp
    push cs
    call HLOCK
check_purgeable:
    test BYTE PTR [di+8], 40h
    jz materialize_done
    push bp
    push cs
    call HPURGE
materialize_done:
    pop bx
    pop WORD PTR ss:[bx]
    xchg ax, bp
    pop bp
    ret
discard_payload:
    push bp
    push cs
    call HFREE
    sub bp, bp
    jmp SHORT materialize_done
MATERIALIZE_RESOURCE ENDP

READ_RESOURCE_PAYLOAD PROC NEAR
    push bp
    mov bp, sp
    add sp, -14
    mov BYTE PTR [bp-5], 0
    mov [bp-4], bx
    mov [bp-2], es
    mov [bp-10], si
    mov [bp-8], ds
    mov [bp-14], dx
    mov [bp-12], cx
    test BYTE PTR [di+8], 10h
    jz load_from_file
    push es
    push bx
    les ax, DWORD PTR [di+2]
    push es
    push ax
    push cx
    push dx
    push cs
    call MOVE_HUGE_BYTES
    sub ax, ax
    jmp NEAR PTR read_done
load_from_file:
    call OPEN_CATALOG_FILE
    test ax, ax
    jz seek_record
    jmp NEAR PTR read_done
seek_record:
    mov bx, [si+2]
    les dx, DWORD PTR [di+2]
    mov cx, es
    mov ax, 4200h
    int 21h
    jc read_done
    mov cx, 1
    lea dx, [bp-6]
    push ss
    pop ds
    mov ah, 3fh
    int 21h
    jc read_done
    cmp ax, cx
    jne read_error
    lds si, DWORD PTR [bp-4]
normalize_destination:
    mov ax, si
    and si, 0fh
    mov cl, 4
    shr ax, cl
    mov cx, ds
    add cx, ax
    mov ds, cx
    mov cx, [bp-14]
    cmp cx, 0fff0h
    ja full_chunk
    cmp WORD PTR [bp-12], 0
    je read_chunk
full_chunk:
    mov cx, 0fff0h
read_chunk:
    mov dx, si
    mov ah, 3fh
    int 21h
    jc read_done
    sub ax, cx
    jne read_error
    jcxz read_done
    push cx
checksum:
    lodsb
    add ah, al
    loop checksum
    add [bp-5], ah
    pop cx
    mov si, dx
    sub [bp-14], cx
    sbb WORD PTR [bp-12], 0
    mov ax, [bp-14]
    or ax, [bp-12]
    jz check_checksum
    add si, cx
    jnc normalize_destination
    mov ax, ds
    add ax, 1000h
    mov ds, ax
    jmp SHORT normalize_destination
check_checksum:
    mov ax, [bp-6]
    not ah
    sub al, ah
    jne read_error
    cbw
read_done:
    lds si, DWORD PTR [bp-10]
    mov sp, bp
    pop bp
    ret
read_error:
    lds si, DWORD PTR [bp-10]
    call CLOSE_CATALOG_FILE
    xchg ax, cx
    mov ax, 13
    jcxz read_done
    mov ax, -1
    jmp SHORT read_done
READ_RESOURCE_PAYLOAD ENDP

FIND_RESOURCE_ENTRY PROC NEAR
    push bp
    mov bp, sp
    add sp, -8
    push ss
    pop ds
    mov [bp-8], ax
    mov [bp-6], dx
    mov [bp-2], cx
    mov si, OFFSET RESOURCE_CATALOG_LIST
next_catalog:
    mov cx, [si]
    jcxz legacy_search
    push cx
    push cs
    call H2P
    xchg ax, si
    mov ds, dx
    mov cx, [si+50h]
    jcxz next_catalog

    inc cx
    mov [bp-4], cx
    sub cx, cx
    les di, DWORD PTR [bp-8]
    mov dx, es
search_type:
    mov ax, cx
    add ax, [bp-4]
    shr ax, 1
    cmp ax, cx
    je next_catalog
    mov bx, ax
    shl bx, 1
    add bx, ax
    shl bx, 1
    cmp dx, [bx+si+4eh]
    jg type_above
    jl type_below
    cmp di, [bx+si+4ch]
    jl type_below
    jg type_above
    lea di, [si+50h]
    add di, [bx+si+50h]
    mov cx, [di]
    jcxz next_catalog

    inc cx
    mov [bp-4], cx
    sub cx, cx
    mov dx, [bp-2]
search_id:
    mov ax, cx
    add ax, [bp-4]
    shr ax, 1
    cmp ax, cx
    je next_catalog
    mov bx, ax
    shl bx, 1
    shl bx, 1
    add bx, ax
    shl bx, 1
    add bx, ax
    cmp dx, [bx+di-9]
    jl id_below
    jg id_above
    lea di, [bx+di-9]
    stc
search_done:
    mov sp, bp
    pop bp
    ret

legacy_search:
    les ax, DWORD PTR [bp-8]
    mov dx, es
    mov cx, [bp-2]
    mov di, SEG LEGACY_CATALOG
    mov ds, di
    mov di, 0fffbh
    jmp SHORT legacy_type_record
id_below:
    mov [bp-4], ax
    jmp SHORT search_id
id_above:
    xchg ax, cx
    jmp SHORT search_id
type_below:
    mov [bp-4], ax
    jmp SHORT search_type
type_above:
    xchg ax, cx
    jmp SHORT search_type
legacy_type_record:
    add di, 0fh
    cmp di, 145h
    je search_done
    cmp ax, [di]
    jne legacy_type_record
    cmp dx, [di+2]
    jne legacy_type_record
    cmp cx, [di+4]
    jne legacy_type_record
    add di, 4
    mov si, 6
    stc
    jmp SHORT search_done
FIND_RESOURCE_ENTRY ENDP
END

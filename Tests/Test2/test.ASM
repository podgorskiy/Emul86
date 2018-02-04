org 100h
; OF | SF | ZF | CF

mov al, 0xF0
sar al, 1
sar al, 2
sar al, 3

mov ax, 0xF000
sar ax, 2

; 0  | 0  | 0  | 0
mov al, 0x7F
ADD al, 0

; 0  | 0  | 0  | 1
mov al, 0xFF
ADD al, 0x7F

; 0  | 0  | 1  | 0
mov al, 0x0
ADD al, 0x0

; 0  | 0  | 1  | 1
mov al, 0xFF
ADD al, 0x1

; 0  | 1  | 0  | 0
mov al, 0xFF
ADD al, 0x0

; 0  | 1  | 0  | 1
mov al, 0xFF
ADD al, 0xFF

; 1  | 0  | 0  | 1
mov al, 0xFF
ADD al, 0x80

; 1  | 0  | 1  | 1
mov al, 0x80
ADD al, 0x80

; 1  | 1  | 0  | 0
mov al, 0x7F
ADD al, 0x7F

nop
nop

; OF | SF | ZF | CF
 
; 0  | 0  | 0  | 0
mov al, 0xFF
SUB al, 0xFE

; 0  | 0  | 0  | 1
mov al, 0x7E
SUB al, 0xFF

; 0  | 0  | 1  | 0
mov al, 0xFF
SUB al, 0xFF

; 0  | 1  | 0  | 0
mov al, 0xFF
SUB al, 0x7F

; 0  | 1  | 0  | 1
mov al, 0xFE
SUB al, 0xFF

; 1  | 0  | 0  | 0
mov al, 0xFE
SUB al, 0x7F

; 1  | 1  | 0  | 1
mov al, 0x7F
SUB al, 0xFF

nop
nop
nop
nop


; 0  | 0  | 0  | 0
mov ax, 0x7FFF
ADD ax, 0

; 0  | 0  | 0  | 1
mov ax, 0xFFFF
ADD ax, 0x7FFF

; 0  | 0  | 1  | 0
mov ax, 0x0000
ADD ax, 0x0000

; 0  | 0  | 1  | 1
mov ax, 0xFFFF
ADD ax, 0x0001

; 0  | 1  | 0  | 0
mov ax, 0xFFFF
ADD ax, 0x0000

; 0  | 1  | 0  | 1
mov ax, 0xFFFF
ADD ax, 0xFFFF

; 1  | 0  | 0  | 1
mov ax, 0xFFFF
ADD ax, 0x8000

; 1  | 0  | 1  | 1
mov ax, 0x8000
ADD ax, 0x8000

; 1  | 1  | 0  | 0
mov ax, 0x7FFF
ADD ax, 0x7FFF

nop
nop

; OF | SF | ZF | CF
 
; 0  | 0  | 0  | 0
mov ax, 0xFFFF
SUB ax, 0xFFFE

; 0  | 0  | 0  | 1
mov ax, 0x7FFE
SUB ax, 0xFFFF

; 0  | 0  | 1  | 0
mov ax, 0xFFFF
SUB ax, 0xFFFF

; 0  | 1  | 0  | 0
mov ax, 0xFFFF
SUB ax, 0x7FFF

; 0  | 1  | 0  | 1
mov ax, 0xFFFE
SUB ax, 0xFFFF

; 1  | 0  | 0  | 0
mov ax, 0xFFFE
SUB ax, 0x7FFF

; 1  | 1  | 0  | 1
mov ax, 0x7FFF
SUB ax, 0xFFFF


nop
nop
nop
nop
nop

mov al, 0x52
ror al, 1
ror al, 1
ror al, 1
ror al, 1
ror al, 1
ror al, 1
ror al, 1
ror al, 1
nop
ror al, 8
nop
rol al, 1
rol al, 1
rol al, 1
rol al, 1
rol al, 1
rol al, 1
rol al, 1
rol al, 1
nop
rcr al, 1
rcr al, 1
rcr al, 1
rcr al, 1
rcr al, 1
rcr al, 1
rcr al, 1
rcr al, 1
rcr al, 1
nop
rcl al, 1
rcl al, 1
rcl al, 1
rcl al, 1
rcl al, 1
rcl al, 1
rcl al, 1
rcl al, 1
rcl al, 1
nop
shl al, 1
shl al, 1
shl al, 1
shl al, 1
shl al, 1
shl al, 1
shl al, 1
shl al, 1


mov si, msg_error
call printstr

mov ah, 0
int 16h

printstr:
push ds
push di
push ax
mov ax, 0b800h
mov ds, ax
mov di, 0
c:
mov al, [cs:si]
mov byte [di], al
inc di
inc si
mov byte [di], 00001111b
inc di
mov al, [cs:si]
and al, al
jnz c
pop ax
pop di
pop si
ret

msg_error  db 'Error! failed: ',0
msg_success  db 'Success!',0

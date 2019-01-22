org 0000

; OF | SF | ZF | CF

start:
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

STC
; 0  | 0  | 0  | 0
mov al, 0x7F
ADC al, 0

; 0  | 0  | 0  | 1
mov al, 0xFF
ADC al, 0x7F

; 0  | 0  | 1  | 0
mov al, 0x0
ADC al, 0x0

; 0  | 0  | 1  | 1
mov al, 0xFF
ADC al, 0x1

; 0  | 1  | 0  | 0
mov al, 0xFF
ADC al, 0x0

; 0  | 1  | 0  | 1
mov al, 0xFF
ADC al, 0xFF

; 1  | 0  | 0  | 1
mov al, 0xFF
ADC al, 0x80

; 1  | 0  | 1  | 1
mov al, 0x80
ADC al, 0x80

; 1  | 1  | 0  | 0
mov al, 0x7F
ADC al, 0x7F

nop
nop

STC
; OF | SF | ZF | CF
 
; 0  | 0  | 0  | 0
mov al, 0xFF
SBB al, 0xFE

; 0  | 0  | 0  | 1
mov al, 0x7E
SBB al, 0xFF

; 0  | 0  | 1  | 0
mov al, 0xFF
SBB al, 0xFF

; 0  | 1  | 0  | 0
mov al, 0xFF
SBB al, 0x7F

; 0  | 1  | 0  | 1
mov al, 0xFE
SBB al, 0xFF

; 1  | 0  | 0  | 0
mov al, 0xFE
SBB al, 0x7F

; 1  | 1  | 0  | 1
mov al, 0x7F
SBB al, 0xFF

nop
nop
nop
nop

STC
; 0  | 0  | 0  | 0
mov ax, 0x7FFF
ADC ax, 0

; 0  | 0  | 0  | 1
mov ax, 0xFFFF
ADC ax, 0x7FFF

; 0  | 0  | 1  | 0
mov ax, 0x0000
ADC ax, 0x0000

; 0  | 0  | 1  | 1
mov ax, 0xFFFF
ADC ax, 0x0001

; 0  | 1  | 0  | 0
mov ax, 0xFFFF
ADC ax, 0x0000

; 0  | 1  | 0  | 1
mov ax, 0xFFFF
ADC ax, 0xFFFF

; 1  | 0  | 0  | 1
mov ax, 0xFFFF
ADC ax, 0x8000

; 1  | 0  | 1  | 1
mov ax, 0x8000
ADC ax, 0x8000

; 1  | 1  | 0  | 0
mov ax, 0x7FFF
ADC ax, 0x7FFF

nop
nop

STC
; OF | SF | ZF | CF
 
; 0  | 0  | 0  | 0
mov ax, 0xFFFF
SBB ax, 0xFFFE

; 0  | 0  | 0  | 1
mov ax, 0x7FFE
SBB ax, 0xFFFF

; 0  | 0  | 1  | 0
mov ax, 0xFFFF
SBB ax, 0xFFFF

; 0  | 1  | 0  | 0
mov ax, 0xFFFF
SBB ax, 0x7FFF

; 0  | 1  | 0  | 1
mov ax, 0xFFFE
SBB ax, 0xFFFF

; 1  | 0  | 0  | 0
mov ax, 0xFFFE
SBB ax, 0x7FFF

; 1  | 1  | 0  | 1
mov ax, 0x7FFF
SBB ax, 0xFFFF

; OF | SF | ZF | CF
 
; 0  | 0  | 0  | 0
mov bx, 0xFFFF
CMP bx, 0xFFFE

; 0  | 0  | 0  | 1
mov dx, 0x7FFE
CMP dx, 0xFFFF

; 0  | 0  | 1  | 0
mov ax, 0xFFFF
CMP ax, 0xFFFF

; 0  | 1  | 0  | 0
mov ax, 0xFFFF
CMP ax, 0x7FFF

; 0  | 1  | 0  | 1
mov ax, 0xFFFE
CMP ax, 0xFFFF

; 1  | 0  | 0  | 0
mov ax, 0xFFFE
CMP ax, 0x7FFF

; 1  | 1  | 0  | 1
mov ax, 0x7FFF
CMP ax, 0xFFFF


; OF | SF | ZF | CF
 
; 0  | 0  | 0  | 0
mov bx, 0xFFFF
XOR bx, 0xFFFE

; 0  | 0  | 0  | 1
mov dx, 0x7FFE
XOR dx, 0xFFFF

; 0  | 0  | 1  | 0
mov ax, 0xFFFF
XOR ax, 0xFFFF

; 0  | 1  | 0  | 0
mov ax, 0xFFFF
XOR ax, 0x7FFF

; 0  | 1  | 0  | 1
mov ax, 0xFFFE
XOR ax, 0xFFFF

; 1  | 0  | 0  | 0
mov ax, 0xFFFE
XOR ax, 0x7FFF

; 1  | 1  | 0  | 1
mov ax, 0x7FFF
XOR ax, 0xFFFF



; OF | SF | ZF | CF
 
; 0  | 0  | 0  | 0
mov bx, 0xFFFF
AND bx, 0xFFFE

; 0  | 0  | 0  | 1
mov dx, 0x7FFE
AND dx, 0xFFFF

; 0  | 0  | 1  | 0
mov ax, 0xFFFF
AND ax, 0xFFFF

; 0  | 1  | 0  | 0
mov ax, 0xFFFF
AND ax, 0x7FFF

; 0  | 1  | 0  | 1
mov ax, 0xFFFE
AND ax, 0xFFFF

; 1  | 0  | 0  | 0
mov ax, 0xFFFE
AND ax, 0x7FFF

; 1  | 1  | 0  | 1
mov ax, 0x7FFF
AND ax, 0xFFFF


; OF | SF | ZF | CF
 
; 0  | 0  | 0  | 0
mov bx, 0xFFFF
OR bx, 0xFFFE

; 0  | 0  | 0  | 1
mov dx, 0x7FFE
OR dx, 0xFFFF

; 0  | 0  | 1  | 0
mov ax, 0xFFFF
OR ax, 0xFFFF

; 0  | 1  | 0  | 0
mov ax, 0xFFFF
OR ax, 0x7FFF

; 0  | 1  | 0  | 1
mov ax, 0xFFFE
OR ax, 0xFFFF

; 1  | 0  | 0  | 0
mov ax, 0xFFFE
OR ax, 0x7FFF

; 1  | 1  | 0  | 1
mov ax, 0x7FFF
OR ax, 0xFFFF


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

mov bx, 10
mov word[bx], 100

; 0  | 0  | 0  | 0
mov al, 0x7F
MUL byte[bx]

; 0  | 0  | 0  | 1
mov al, 0xFF
MUL word[bx]

; 0  | 0  | 1  | 0
mov ax, 0xFF0
MUL word[bx]

; 0  | 0  | 1  | 1
mov al, 0xFF
MUL word[bx]

; 0  | 1  | 0  | 0
mov al, 0xFF
MUL word[bx]

; 0  | 1  | 0  | 1
mov al, 0xFF
MUL word[bx]

; 1  | 0  | 0  | 1
mov ax, 0x1FF
MUL byte[bx]

; 1  | 0  | 1  | 1
mov al, 0x80
MUL word[bx]

; 1  | 1  | 0  | 0
mov al, 0x7F
MUL word[bx]

nop
nop


; 0  | 0  | 0  | 0
mov al, 0x7F
IMUL word[bx]

; 0  | 0  | 0  | 1
mov al, 0xFF
IMUL byte[bx]

; 0  | 0  | 1  | 0
mov al, 0x0
IMUL word[bx]

; 0  | 0  | 1  | 1
mov ax, 0x4FF
IMUL word[bx]

; 0  | 1  | 0  | 0
mov al, 0xFF
IMUL word[bx]

; 0  | 1  | 0  | 1
mov al, 0xFF
IMUL word[bx]

; 1  | 0  | 0  | 1
mov al, 0xFF
IMUL word[bx]

; 1  | 0  | 1  | 1
mov al, 0x80
IMUL word[bx]

; 1  | 1  | 0  | 0
mov al, 0x7F
IMUL word[bx]

; 0  | 0  | 0  | 0
mov ax, 0x7F
IMUL byte[bx]

; 0  | 0  | 0  | 1
mov ax, 0xFFF
IMUL word[bx]

; 0  | 0  | 1  | 0
mov ax, 0x0F
IMUL word[bx]

; 0  | 0  | 1  | 1
mov ax, 0xFFF
IMUL word[bx]

; 0  | 1  | 0  | 0
mov ax, 0xFFF
IMUL word[bx]

; 0  | 1  | 0  | 1
mov ax, 0xFF
IMUL word[bx]

; 1  | 0  | 0  | 1
mov ax, 0xFF
IMUL word[bx]

; 1  | 0  | 1  | 1
mov ax, 0x80
IMUL byte[bx]

; 1  | 1  | 0  | 0
mov ax, 0x7F
IMUL word[bx]

nop
nop

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

align 0xfff0
jmp start

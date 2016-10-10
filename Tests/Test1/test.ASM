org 100h

xor ax, ax

add al, 253
add al, 45
add al, 5bh
adc ah, 10

add	ax, 0b800h

sub ax, 100

add byte [02h], 'H'

add byte [04h], 'e'

ret
mov dx, 0x03F8    ; COM1 serial port
mov al, 5        ; set al to 10
add al, bl        ; AL = AL + BL
add al, 0x30      ; convert to ASCII digit
out dx, al        ; send to serial

mov al, 0x0A      ; newline ('\n')
out dx, al        ; send newline

hlt               ; halt CPU
$MODLP51RC2
org 0000H
   ljmp MainProgram

CLK  EQU 22118400
BAUD equ 115200
BRG_VAL equ (0x100-(CLK/(16*BAUD)))
$include(math32.inc)


CE_ADC EQU P2.0 ;enable for adc

;mosi: master out slave in
MY_MOSI EQU P2.1 ;connected to Din(SPI port serial data input pin, used to load	channel configuration data)
;miso: master in slave out
MY_MISO EQU P2.2 ;connected to Dout (The SPI serial data output pin is used to shift out the results of the A/D conversion. Data will always change	on the falling edge of each clock as the conversion takes place)
;serial clock
MY_SCLK EQU P2.3

dseg at 0x30
Result:     ds 2
x:			ds 4
y:			ds 4
BCD:		ds 5

BSEG
mf:			dbit 1


CSEG

;Read_ADC_Channel MAC
;	mov b, #%0
;	lcall _Read_ADC_Channel
;ENDMAC
;
;_Read_ADC_Channel:
;	clr CE_ADC
;	mov R0, #00000001B ; Start bit:1
;	lcall DO_SPI_G
;	mov a, b
;	swap a
;	anl a, #0F0H
;	setb acc.7 ; Single mode (bit 7).
;	mov R0, a
;	lcall DO_SPI_G
;	mov a, R1 ; R1 contains bits 8 and 9
;	anl a, #00000011B ; We need only the two least significant bits
;	mov R7, a ; Save result high.
;	mov R0, #55H ; It doesn't matter what we transmit...
;	lcall DO_SPI_G
;	mov R6, R1 ; R1 contains bits 0 to 7. Save result low.
;	setb CE_ADC
;ret


; Configure the serial port and baud rate
InitSerialPort:
    ; Since the reset button bounces, we need to wait a bit before
    ; sending messages, otherwise we risk displaying gibberish!
    mov R1, #222
    mov R0, #166
    djnz R0, $   ; 3 cycles->3*45.21123ns*166=22.51519us
    djnz R1, $-4 ; 22.51519us*222=4.998ms
    ; Now we can proceed with the configuration
	orl	PCON,#0x80
	mov	SCON,#0x52 ;Mode 1, REN = 1, TI = 1 
	;//REN: setting to 1 enables serial reception | TI: setting to 1 empties transmit buffer - writing to SBUF will initiate transmission
	
	mov	BDRCON,#0x00
	mov	BRL,#BRG_VAL
	mov	BDRCON,#0x1E ; BDRCON=BRR|TBCK|RBCK|SPD;
    ret 

;configure spi (thing to communicate ADC and microcontroller) and i think adc is putty
INIT_SPI:
	setb MY_MISO ; Make MISO an input pin //setting it to one is how that works
	clr MY_SCLK ; For mode (0,0) SCLK is zero
	ret
 
 
; Send a character using the serial port //send where? what character?
putchar:
    jnb TI, putchar
    clr TI
    mov SBUF, a
    ret

; Send a constant-zero-terminated string using the serial port
SendString:
    clr A
    movc A, @A+DPTR
    jz SendStringDone
    lcall putchar
    inc DPTR
    sjmp SendString
SendStringDone:
    ret
 
;Hello_World:
    ;DB  'Hello, World!', '\r', '\n', 0

WaitHalfSec:
    mov R6, #89
L9: mov R5, #250
L8: mov R4, #166
L7: djnz R4, L7 ; 3 cycles->3*45.21123ns*166=22.51519us
    djnz R5, L8 ; 22.51519us*250=5.629ms
    djnz R6, L9 ; 5.629ms*89=0.5s (approximately)
    ret

;send byte in R0, receive byte in R1
DO_SPI_G:
	push acc
	mov R1, #0 ; Received byte stored in R1
	mov R2, #8 ; Loop counter (8-bits)
DO_SPI_G_LOOP: ;one byte at a time i think
	mov a, R0 ; Byte to write is in R0
	rlc a ; Carry flag has bit to write
	mov R0, a
	mov MY_MOSI, c
	setb MY_SCLK ; Transmit 
	mov c, MY_MISO ; Read received bit
	mov a, R1 ; Save received bit in R1
	rlc a
	mov R1, a
	clr MY_SCLK
	djnz R2, DO_SPI_G_LOOP ;when is R2 decreasing though? like when would it get to 0?
	pop acc
	ret
	
;r0 is the data sent out from master -> slave, r1 is the data taken from slave->master 

;PrintVoltage:
	



	;lcall wait_for_P4_5


MainProgram:
    mov SP, #7FH ; Set the stack pointer to the begining of idata
    
    ;my code
    lcall InitSerialPort
    ;mov DPTR, #Hello_World
    ;lcall SendString
loop:    
    clr CE_ADC
    mov R0, #0b00000001 ;put 1 into r0, ie start bit = 1
    lcall DO_SPI_G ;send R0 content to R1 one byte at a time?
    
    mov R0, #0b10000000 ;"single ended, read channel 0"
    lcall DO_SPI_G ;etc
    mov a, R1 ;R1 contains bits 8 and 9
    anl a, #0b00000011 ;and-ed with dis will give us 01111100, ie only need two least significant digits
    mov Result+1, a ;"save result high"
    
    mov R0, #55H ;literally random doesn't matter
    lcall DO_SPI_G
    mov Result+0, R1 ;R1 contains bits 0 to 7, save result low (?)
    setb CE_ADC ;why
    ;my code
    
    lcall WaitHalfSec

	 
	mov x+0, Result+0
	mov x+1, Result+1
	mov x+2, #0
	mov x+3, #0
	lcall hex2bcd
	
	
	mov a, BCD+1
	anl a, #0xf0
	swap a
	orl a, #'0'
	lcall putchar
	
	mov a, BCD+1
	anl a, #0x0f
	orl a, #'0'
	lcall putchar

	mov a, BCD+0
	anl a, #0xf0
	swap a
	orl a, #'0'
	lcall putchar
	
	mov a, BCD+0
	anl a, #0x0f
	orl a, #'0'
	lcall putchar
		
	mov a, #'\r'
	lcall putchar
	   
	mov a, #'\n'
	lcall putchar

    ljmp loop 
    
END

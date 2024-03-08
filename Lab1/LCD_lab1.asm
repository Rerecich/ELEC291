; LCD_test_4bit.asm: Initializes and uses an LCD in 4-bit mode
; using the most common procedure found on the internet.
$NOLIST
$MODLP51RC2
$LIST

org 0000H
    ljmp setup

                ;just setting the wires in the right places

; These 'equ' must match the hardware wiring
LCD_RS equ P3.2 ;RS = register select
;LCD_RW equ PX.X ; Not used in this code, connect the pin to GND
LCD_E  equ P3.3 ;E = enable
LCD_D4 equ P3.4
LCD_D5 equ P3.5
LCD_D6 equ P3.6
LCD_D7 equ P3.7

RST equ RST#
;jnb BT, 
BT equ P4.5

; When using a 22.1184MHz crystal in fast mode
; one cycle takes 1.0/22.1184MHz = 45.21123 ns (timing stuff)

;string data
myName:
    DB 'Sarah Rerecich', 0

myNumber:
    DB '65693236', 0

welcome:
    DB 'select superior:.', 0

choose:
    DB 'elec(rs) cpen(bt)', 0

boo:
    DB 'boo', 0

correct:
    DB 'correct', 0

selectone:
    DB 'select one', 0

empty:
    DB '                 ', 0

setup:
    mov SP, #7FH ;moves SP to 7FH which is a position in memory
    lcall LCD_4BIT ;configure 4-bit mode

    lcall printCenter

;---------------------------------;
; Wait 40 microseconds            ;
;---------------------------------;
Wait40uSec:
    push AR0
    mov R0, #177
L0:
    nop
    nop
    djnz R0, L0 ; 1+1+3 cycles->5*45.21123ns*177=40us
    pop AR0
    ret

;---------------------------------;
; Wait 'R2' milliseconds          ;
;---------------------------------;
WaitmilliSec:
    push AR0
    push AR1
L3: mov R1, #45
L2: mov R0, #166
L1: djnz R0, L1 ; 3 cycles->3*45.21123ns*166=22.51519us
    djnz R1, L2 ; 22.51519us*45=1.013ms
    djnz R2, L3 ; number of millisecons to wait passed in R2
    pop AR1
    pop AR0
    ret

;---------------------------------;
; Toggles the LCD's 'E' pin       ;  enable pin
;---------------------------------;
LCD_pulse:
    setb LCD_E
    lcall Wait40uSec
    clr LCD_E
    ret

;---------------------------------;
; Writes data to LCD              ;
;---------------------------------;
WriteData:

    setb LCD_RS ;sets LCD_RS to 1
    ljmp LCD_byte

;---------------------------------;
; Writes command to LCD           ;
;---------------------------------;
WriteCommand:
    clr LCD_RS ;clear LCD_RS (register select)
    ljmp LCD_byte ;next writes accumulator to LCD in 4-bit mode

;---------------------------------;
; Writes acc to LCD in 4-bit mode ;
;---------------------------------;


LCD_string_print:
    clr a 
    movc a, @a+dptr
    jz LCD_string_return

    lcall WriteData

    inc dptr
    lcall Wait40uSec

    sjmp LCD_string_print

LCD_string_return:
    ret

LCD_byte:
    ; Write high 4 bits first
    mov c, ACC.7 ;i think c is just a temporary register
    mov LCD_D7, c ;makes LCD_D7 pin take value of 7th bit in accumulator

    mov c, ACC.6
    mov LCD_D6, c

    mov c, ACC.5
    mov LCD_D5, c

    mov c, ACC.4
    mov LCD_D4, c
    
    lcall LCD_pulse

    ; Write low 4 bits next
    mov c, ACC.3
    mov LCD_D7, c

    mov c, ACC.2
    mov LCD_D6, c

    mov c, ACC.1
    mov LCD_D5, c

    mov c, ACC.0
    mov LCD_D4, c

    lcall LCD_pulse
    ret ;return from subroutine

;---------------------------------;
; Configure LCD in 4-bit mode     ;
;---------------------------------;
LCD_4BIT:
    clr LCD_E   ; Resting state of LCD's enable is zero
    ; clr LCD_RW  ; Not used, pin tied to GND

    ; After power on, wait for the LCD start up time before initializing
    ; NOTE: the preprogrammed power-on delay of 16 ms on the AT89LP51RC2
    ; seems to be enough.  That is why these two lines are commented out.
    ; Also, commenting these two lines improves simulation time in Multisim.
    ; mov R2, #40
    ; lcall WaitmilliSec

    ; First make sure the LCD is in 8-bit mode and then change to 4-bit mode
    mov a, #0x33 ;0x just saying that the following number is in hex
    lcall WriteCommand
    mov a, #0x33 ;recall that a is accumulator
    lcall WriteCommand ;why twice?
    mov a, #0x32 ; change to 4-bit mode - changes value in accumulator to 0x32...why does this change to 4-bit?
    lcall WriteCommand

    ; Configure the LCD
    mov a, #0x28
    lcall WriteCommand
    mov a, #0x0c
    lcall WriteCommand
    mov a, #0x01 ;  Clear screen command (takes some time)
    lcall WriteCommand

    ;Wait for clear screen command to finish. Usually takes 1.52ms.
    mov R2, #2
    lcall WaitmilliSec
    ret

;scroll right: plan to print one extra letter each time but starting from last letter

check:
   ; mov a, BT
    cjne a, #0, check2
   ; sjmp check

check2:
   ; cjne  a, #0, wrong ;if boot is not 0 ie if cpen was chosen

    mov a, #0x80  ;else correct
    lcall WriteCommand 

    mov dptr, #correct
    lcall LCD_string_print

    ret

wrong:
    mov a, #0x80  ;else correct
    lcall WriteCommand 

    mov dptr, #boo
    lcall LCD_string_print

    ret

begin:
    mov a, #0x80 
    lcall WriteCommand 

    mov dptr, #myName
    lcall LCD_string_print
    lcall Wait40uSec

    mov a, #0xC4 
    lcall WriteCommand 

    mov dptr, #myNumber
    lcall LCD_string_print
    lcall printCenter
    



;---------------------------------;
; Main loop.  Initialize stack,   ;
; ports, LCD, and displays        ;
; letters on the LCD              ;
;---------------------------------;
myprogram:
    mov SP, #7FH ;moves SP to 7FH which is a position in memory
    lcall LCD_4BIT ;configure 4-bit mode

    

    mov a, #0x80 ; Move cursor to line 1 column 1 (this plus next instruction)
    lcall WriteCommand ;this and its following task clears pin then writes accumulator to screen (doesn't it have #0x01 right now?)
    mov a, #'S' ;self explanatory
    lcall WriteData ;this sets RS to 1 then writes accumulator to screen (just like WriteCommand but different starting task)

    mov a, #0x81 
    lcall WriteCommand 
    mov a, #'A' 
    lcall WriteData 

    mov a, #0x82 
    lcall WriteCommand 
    mov a, #'R' 
    lcall WriteData 

    mov a, #0x83
    lcall WriteCommand 
    mov a, #'A' 
    lcall WriteData 

    mov a, #0x84 
    lcall WriteCommand 
    mov a, #'H' 
    lcall WriteData 

    mov a, #0x85 
    lcall WriteCommand 
    mov a, #' ' 
    lcall WriteData 

    mov a, #0x86 
    lcall WriteCommand 
    mov a, #'R' 
    lcall WriteData 

    mov a, #0x87 
    lcall WriteCommand 
    mov a, #'E' 
    lcall WriteData 

    mov a, #0x88 
    lcall WriteCommand 
    mov a, myName
    lcall WriteData 


printCenter:

    mov a, #0x81 
    lcall WriteCommand 

    mov dptr, #myName
    lcall LCD_string_print
    lcall Wait40uSec

    mov a, #0xC4 
    lcall WriteCommand 

    mov dptr, #myNumber
    lcall LCD_string_print
    lcall Wait40uSec

   sjmp printCenter




forever:
    sjmp forever
END

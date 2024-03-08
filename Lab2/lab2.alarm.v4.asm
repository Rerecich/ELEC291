; ISR_example.asm: a) Increments/decrements a BCD variable every half second using
; an ISR for timer 2; b) Generates a 2kHz square wave at pin P1.1 using
; an ISR for timer 0; and c) in the 'main' loop it displays the variable
; incremented/decremented using the ISR for timer 2 on the LCD.  Also resets it to 
; zero if the 'BOOT' pushbutton connected to P4.5 is pressed.
$NOLIST
$MODLP51RC2
$LIST

CLK           EQU 22118400 ; Microcontroller system crystal frequency in Hz
TIMER0_RATE   EQU 4096     ; 2048Hz squarewave (peak amplitude of CEM-1203 speaker)
TIMER0_RELOAD EQU ((65536-(CLK/TIMER0_RATE)))
TIMER2_RATE   EQU 1000     ; 1000Hz, for a timer tick of 1ms
TIMER2_RELOAD EQU ((65536-(CLK/TIMER2_RATE)))

BOOT_BUTTON   equ P4.5
BUTTON_1      equ P2.4
BUTTON_2      equ P2.7
BUTTON_3      equ P0.6
BUTTON_4      equ P0.3

DEBUG         equ P2.0

SOUND_OUT     equ P1.1
UPDOWN        equ P0.0

; Reset vector
org 0x0000
    ljmp setup

; External interrupt 0 vector (not used in this code)
org 0x0003
	reti

; Timer/Counter 0 overflow interrupt vector
org 0x000B
	ljmp Timer0_ISR

; External interrupt 1 vector (not used in this code)
org 0x0013
	reti

; Timer/Counter 1 overflow interrupt vector (not used in this code)
org 0x001B
	reti

; Serial port receive/transmit interrupt vector (not used in this code)
org 0x0023 
	reti
	
; Timer/Counter 2 overflow interrupt vector
org 0x002B
	ljmp Timer2_ISR

; In the 8051 we can define direct access variables starting at location 0x30 up to location 0x7F
dseg at 0x30
Count1ms:     ds 2 ; Used to determine when half second has passed
BCD_counter:  ds 1 ; The BCD counter incrememted in the ISR and displayed in the main loop

second_count: ds 1
minute_count: ds 1
hour_count: ds 1

second_count_alarm: ds 1
minute_count_alarm: ds 1
hour_count_alarm: ds 1
; In the 8051 we have variables that are 1-bit in size.  We can use the setb, clr, jb, and jnb
; instructions with these variables.  This is how you define a 1-bit variable:
bseg
half_seconds_flag: dbit 1 ; Set to one in the ISR every time 500 ms had passed
AM_PM_flag: dbit 1
alarm_flag: dbit 1
am_pm_flag_alarm: dbit 1

cseg
; These 'equ' must match the hardware wiring
LCD_RS equ P3.2
;LCD_RW equ PX.X ; Not used in this code, connect the pin to GND
LCD_E  equ P3.3
LCD_D4 equ P3.4
LCD_D5 equ P3.5
LCD_D6 equ P3.6
LCD_D7 equ P3.7

$NOLIST
$include(LCD_4bit.inc) ; A library of LCD related functions and utility macros
$LIST

;                     1234567890123456    <- This helps determine the location of the counter
Initial_Message:  db 'Initial Message', 0
Time: db 'Time: --:--:-- -', 0
Alarm: db 'Alarm: --:--:--', 0 

On: db 'On', 0
Off: db 'Off       ', 0

Empty: db '               ', 0
Empty2: db '  ', 0

SettingTime: db 'SETTING TIME  ', 0
SettingAlarm: db 'SETTING ALARM ', 0

ConfirmAlarm: db 'Set alarm?       ', 0
ChooseAlarm: db 'Yes           No', 0

AlarmSet: db 'Alarm Set:      ', 0
AlarmCancelled: db 'Alarm Cancelled.   ', 0

RingRingRing: db 'Ring Ring     ', 0




AM_display: db 'A ', 0
PM_display: db 'P ', 0
Check: db 'check', 0

;---------------------------------;
; Routine to initialize the ISR   ;
; for timer 0                     ;
;---------------------------------;
Timer0_Init:
	mov a, TMOD
	anl a, #0xf0 ; 11110000 Clear the bits for timer 0
	orl a, #0x01 ; 00000001 Configure timer 0 as 16-timer
	mov TMOD, a ; timer mode
	mov TH0, #high(TIMER0_RELOAD) ; load timer
	mov TL0, #low(TIMER0_RELOAD)
	; Set autoreload value
	mov RH0, #high(TIMER0_RELOAD)
	mov RL0, #low(TIMER0_RELOAD)
	; Enable the timer and interrupts
    setb ET0  ; Enable timer 0 interrupt
    setb TR0  ; Start timer 0
	ret

;---------------------------------;
; ISR for timer 0.  Set to execute;
; every 1/4096Hz to generate a    ;
; 2048 Hz square wave at pin P1.1 ;
;---------------------------------;
Timer0_ISR:
	;clr TF0  ; According to the data sheet this is done for us already.
	cpl SOUND_OUT ; Connect speaker to P1.1!
	reti

;---------------------------------;
; Routine to initialize the ISR   ;
; for timer 2                     ;
;---------------------------------;
Timer2_Init:
	mov T2CON, #0 ; Stop timer/counter.  Autoreload mode.
	mov TH2, #high(TIMER2_RELOAD)
	mov TL2, #low(TIMER2_RELOAD)
	; Set the reload value
	mov RCAP2H, #high(TIMER2_RELOAD)
	mov RCAP2L, #low(TIMER2_RELOAD)
	; Init One millisecond interrupt counter.  It is a 16-bit variable made with two 8-bit parts
	clr a
	mov Count1ms+0, a
	mov Count1ms+1, a
	; Enable the timer and interrupts
    setb ET2  ; Enable timer 2 interrupt
    setb TR2  ; Enable timer 2
	ret

;---------------------------------;
; ISR for timer 2                 ;
;---------------------------------;
Timer2_ISR:
	clr TF2  ; Timer 2 doesn't clear TF2 automatically. Do it in ISR
	cpl P1.0 ; To check the interrupt rate with oscilloscope. It must be precisely a 1 ms pulse.
	
	; The two registers used in the ISR must be saved in the stack
	push acc
	push psw
	
	; Increment the 16-bit one mili second counter
	inc Count1ms+0    ; Increment the low 8-bits first
	mov a, Count1ms+0 ; If the low 8-bits overflow, then increment high 8-bits
	jnz Inc_Done
	inc Count1ms+1


Inc_Done:
	; Check if half second has passed
	mov a, Count1ms+0
	cjne a, #low(1), jump1 ; Warning: this instruction changes the carry flag!
	mov a, Count1ms+1
	cjne a, #high(1), jump1
	
	; 500 milliseconds have passed.  Set a flag so the main program knows
	setb half_seconds_flag ; Let the main program know half second had passed
    
    ; Check if half second has passed

    jnb alarm_flag, incSecondsNormal ;if alarm on, check time
    lcall checkAlarmTime

incSecondsNormal:
    ;inc seconds
     
	clr a
	mov Count1ms+0, a
	mov Count1ms+1, a
	; Increment the BCD counter
	mov a, second_count
	jnb UPDOWN, jump3
	add a, #0x01
	ljmp Timer2_ISR_da_second

jump1:
    ljmp Timer2_ISR_done
jump3:
    ljmp Timer2_ISR_decrement

TriggerAlarm:
   Set_Cursor(1,1)
   Send_Constant_String(#RingRingRing)

    jb BUTTON_4, AlarmOff  ; if the 'BOOT' button is not pressed skip
	Wait_Milli_Seconds(#50)	; Debounce delay.  This macro is also in 'LCD_4bit.inc'
	jb BUTTON_4, AlarmOff  ; if the 'BOOT' button is not pressed skip
	jnb BUTTON_4, $	

    sjmp TriggerAlarm

AlarmOff:
    clr alarm_flag
    ljmp Timer2_ISR_done

  
checkAlarmTime:
    ;compare time with alarm time
    Set_Cursor(1,1)
    Send_Constant_String(#Check)
    Wait_Milli_Seconds(#250) 
    Set_Cursor(1,1)
    Send_Constant_String(#Check)
    Wait_Milli_Seconds(#250)
    
    clr DEBUG

   ;mov a, hour_count
   ;cjne a, hour_count_alarm, incSecondsNormal 
   ;mov a, minute_count
   ;cjne a, minute_count_alarm, incSecondsNormal
   ;mov a, second_count
   ;cjne a, second_count_alarm, incSecondsNormal
	;
   ;clr a
   ;mov Count1ms+0, a
	;mov Count1ms+1, a
	;;; Increment the BCD counter
	;ljmp TriggerAlarm
;
   ; ;sjmp checkAlarmTime
   ret


Timer2_ISR_decrement:
	add a, #0x99 ; Adding the 10-complement of -1 is like subtracting 1.


Timer2_ISR_da_second:
	da a ; Decimal adjust instruction.  Check datasheet for more details!
	mov second_count, a

Check_minute:
    cjne a, #0x60, Timer2_ISR_done
    sjmp Timer2_ISR_Minute

Timer2_ISR_da_minute:
	da a ; Decimal adjust instruction.  Check datasheet for more details!
	mov minute_count, a

Check_hour:
    cjne a, #0x60, Timer2_ISR_done
    sjmp Timer2_ISR_Hour

Timer2_ISR_da_hour:
	da a ; Decimal adjust instruction.  Check datasheet for more details!
	mov hour_count, a

Check_AMPM:
    cjne a, #0x13, Timer2_ISR_done
    sjmp Timer2_ISR_AMPM

Timer2_ISR_done:
	pop psw
	pop acc
	reti

Timer2_ISR_Minute:
    mov a, #0x00
    mov second_count, a
    clr a
	mov Count1ms+0, a
	mov Count1ms+1, a
	; Increment the BCD counter
	mov a, minute_count
	jnb UPDOWN, Timer2_ISR_decrement
	add a, #0x01
	sjmp Timer2_ISR_da_minute

Timer2_ISR_Hour:
    mov a, #0x00
    mov minute_count, a
    clr a
	mov Count1ms+0, a
	mov Count1ms+1, a
	; Increment the BCD counter
	mov a, hour_count
	jnb UPDOWN, Timer2_ISR_decrement
	add a, #0x01
	sjmp Timer2_ISR_da_hour

Timer2_ISR_AMPM:
    cpl AM_PM_flag    

    mov a, #0x01
    mov hour_count, a
    clr a 
    mov Count1ms+0, a
	mov Count1ms+1, a
    

    jb AM_PM_flag, PM ;if not pm, still displays AM

    Set_Cursor(1, 16)
    Send_Constant_String(#AM_display)
    
    sjmp Timer2_ISR_done


PM:
    Set_Cursor(1, 16)
    Send_Constant_String(#PM_display)


    sjmp Timer2_ISR_done


flash_clock:
    clr TR2                 ; Stop timer 2
	clr a


    Set_Cursor(1, 7)
    Send_Constant_String(#Empty)

    Set_Cursor(2, 1)
    Send_Constant_String(#Empty)

    Set_Cursor(2, 3)
    Send_Constant_String(#SettingTime)

    Wait_Milli_Seconds(#100)
    Wait_Milli_Seconds(#100)

    Set_Cursor(1, 1)
    Send_Constant_String(#Time)

    Set_Cursor(1, 7)
	Display_BCD(hour_count)

	Set_Cursor(1, 10)
	Display_BCD(minute_count)

	Set_Cursor(1, 13)
	Display_BCD(second_count)

    Wait_Milli_Seconds(#100)
    Wait_Milli_Seconds(#100)

    jb BUTTON_1, flash_clock1 
	;Wait_Milli_Seconds(#5)	
	jb BUTTON_1, flash_clock1
	jnb BUTTON_1, $	

    lcall flash_hours
    ret


flash_clock1:
    
    lcall flash_clock

edit_hours:
    mov a, hour_count
    add a, #0x01
    da a 
    mov hour_count, a 
    cjne a, #0x13, flash_hours

    mov a, #0x01
    mov hour_count, a
    ljmp flash_hours

flash_hours:
    

    Set_Cursor(1, 7)
    Send_Constant_String(#Empty2)

    Wait_Milli_Seconds(#100)
    Wait_Milli_Seconds(#100)

    Set_Cursor(1, 7)
    Display_BCD(hour_count)

    Wait_Milli_Seconds(#100)
    Wait_Milli_Seconds(#100)

    jb BUTTON_2, flash_hours1
	;Wait_Milli_Seconds(#5)	
	;jb BUTTON_2, flash_hours1
	;jnb BUTTON_2, $	

    ljmp edit_hours


flash_hours1:
    jb BUTTON_1, flash_hours
	;Wait_Milli_Seconds(#5)	
	;jb BUTTON_1, flash_hours
	;jnb BUTTON_1, $	

    ljmp flash_minutes

edit_minutes:
    mov a, minute_count
    add a, #0x01
    da a 
    mov minute_count, a 
    cjne a, #0x60, flash_minutes

    mov a, #0x00
    mov minute_count, a
    ljmp flash_minutes

flash_minutes:
    Set_Cursor(1, 10)
    Send_Constant_String(#Empty2)

    Wait_Milli_Seconds(#100)
    Wait_Milli_Seconds(#100)

    Set_Cursor(1, 10)
    Display_BCD(hour_count)

    Wait_Milli_Seconds(#100)
    Wait_Milli_Seconds(#100)

    jb BUTTON_2, flash_minutes1
	;Wait_Milli_Seconds(#5)	
	;jb BUTTON_2, flash_minutes1
	;jnb BUTTON_2, $	

    ljmp edit_minutes

flash_minutes1:
    jb BUTTON_1, flash_minutes
	;Wait_Milli_Seconds(#5)	
	;jb BUTTON_1, flash_minutes
	;jnb BUTTON_1, $	

    ljmp flash_seconds

edit_seconds:
    mov a, second_count
    add a, #0x01
    da a 
    mov second_count, a 
    cjne a, #0x60, flash_seconds

    mov a, #0x00
    mov second_count, a
    ljmp flash_seconds

flash_seconds:
    Set_Cursor(1, 13)
    Send_Constant_String(#Empty2)

    Wait_Milli_Seconds(#100)
    Wait_Milli_Seconds(#100)

    Set_Cursor(1, 13)
    Display_BCD(second_count)

    Wait_Milli_Seconds(#100)
    Wait_Milli_Seconds(#100)

    jb BUTTON_2, flash_seconds1
	;Wait_Milli_Seconds(#5)	
	;jb BUTTON_2, flash_seconds1
	;jnb BUTTON_2, $	

    ljmp edit_seconds

flash_seconds1:
    jb BUTTON_1, flash_seconds
	;Wait_Milli_Seconds(#5)	
	;jb BUTTON_1, flash_seconds
	;jnb BUTTON_1, $	

    jb am_pm_flag,flash_AM
    ljmp flash_PM

edit_AMPM:
    cpl am_pm_flag
    jb am_pm_flag, flash_AM
    ljmp flash_PM

flash_AM:
    Set_Cursor(1, 16)
    Send_Constant_String(#Empty2)

    Wait_Milli_Seconds(#100)
    Wait_Milli_Seconds(#100)

    Set_Cursor(1, 16)
    Send_Constant_String(#AM_display)

    Wait_Milli_Seconds(#100)
    Wait_Milli_Seconds(#100)

    jb BUTTON_2, flash_AM1	

    ljmp edit_AMPM

flash_PM:
    Set_Cursor(1, 16)
    Send_Constant_String(#Empty2)

    Wait_Milli_Seconds(#100)
    Wait_Milli_Seconds(#100)

    Set_Cursor(1, 16)
    Send_Constant_String(#PM_display)

    Wait_Milli_Seconds(#100)
    Wait_Milli_Seconds(#100)

    jb BUTTON_2, flash_PM1
	;Wait_Milli_Seconds(#5)	
	;jb BUTTON_2, flash_seconds1
	;jnb BUTTON_2, $	

    ljmp edit_AMPM
jump4:
    ljmp flash_AM

flash_AM1:
    jb BUTTON_1, jump4

    lcall flash_alarm

flash_PM1:
    jb BUTTON_1, flash_PM

    lcall flash_alarm



flash_alarm:
    clr TR2                 ; Stop timer 2
	clr a

    Set_Cursor(1, 1)
    Send_Constant_String(#Empty)

    Set_Cursor(1, 3)
    Send_Constant_String(#SettingAlarm)

    Set_Cursor(2, 7)
    Send_Constant_String(#Empty)

    Set_Cursor(2, 1)
    Send_Constant_String(#Alarm)

    Wait_Milli_Seconds(#1)

    Wait_Milli_Seconds(#100)
    Wait_Milli_Seconds(#100)

    Set_Cursor(2, 1)
    Send_Constant_String(#Alarm)

    Set_Cursor(2, 8)
	Display_BCD(hour_count_alarm)

	Set_Cursor(2, 11)
	Display_BCD(minute_count_alarm)

	Set_Cursor(2, 14)
	Display_BCD(second_count_alarm)

    Wait_Milli_Seconds(#100)
    Wait_Milli_Seconds(#100)

    jb BUTTON_1, flash_alarm1 
	;Wait_Milli_Seconds(#5)	
	jb BUTTON_1, flash_alarm1
	jnb BUTTON_1, $	

    lcall flash_hours_alarm
    ret

flash_alarm1:
    
    lcall flash_alarm


edit_hours_alarm:
    mov a, hour_count_alarm
    add a, #0x01
    da a 
    mov hour_count_alarm, a 
    cjne a, #0x13, flash_hours_alarm

    mov a, #0x01
    mov hour_count_alarm, a
    ljmp flash_hours_alarm

flash_hours_alarm:
    

    Set_Cursor(2, 8)
    Send_Constant_String(#Empty2)

    Wait_Milli_Seconds(#100)
    Wait_Milli_Seconds(#100)

    Set_Cursor(2, 8)
    Display_BCD(hour_count_alarm)

    Wait_Milli_Seconds(#100)
    Wait_Milli_Seconds(#100)

    jb BUTTON_2, flash_hours_alarm1
	;Wait_Milli_Seconds(#5)	
	jb BUTTON_2, flash_hours_alarm1
	jnb BUTTON_2, $	

    ljmp edit_hours_alarm
    


flash_hours_alarm1:
    jb BUTTON_1, flash_hours_alarm
	;Wait_Milli_Seconds(#5)	
	jb BUTTON_1, flash_hours_alarm
	jnb BUTTON_1, $	

    ljmp flash_minutes_alarm

edit_minutes_alarm:
    mov a, minute_count_alarm
    add a, #0x01
    da a 
    mov minute_count, a 
    cjne a, #0x59, flash_minutes_alarm

    mov a, #0x00
    mov minute_count_alarm, a
    ljmp flash_minutes_alarm

flash_minutes_alarm:
    Set_Cursor(2, 11)
    Send_Constant_String(#Empty2)

    Wait_Milli_Seconds(#100)
    Wait_Milli_Seconds(#100)

    Set_Cursor(2, 11)
    Display_BCD(hour_count_alarm)

    Wait_Milli_Seconds(#100)
    Wait_Milli_Seconds(#100)

    jb BUTTON_2, flash_minutes_alarm1
	;Wait_Milli_Seconds(#5)	
	jb BUTTON_2, flash_minutes_alarm1
	jnb BUTTON_2, $	

    clr a

    ljmp edit_minutes_alarm

flash_minutes_alarm1:
    jb BUTTON_1, flash_minutes_alarm
	;Wait_Milli_Seconds(#5)	
	jb BUTTON_1, flash_minutes_alarm
	jnb BUTTON_1, $	

    ljmp flash_seconds_alarm

edit_seconds_alarm:
    mov a, second_count_alarm
    add a, #0x01
    da a 
    mov second_count_alarm, a 
    cjne a, #0x59, flash_seconds_alarm

    mov a, #0x00
    mov second_count_alarm, a
    ljmp flash_seconds_alarm

flash_seconds_alarm:
    Set_Cursor(2, 14)
    Send_Constant_String(#Empty2)

    Wait_Milli_Seconds(#100)
    Wait_Milli_Seconds(#100)

    Set_Cursor(2, 14)
    Display_BCD(second_count_alarm)

    Wait_Milli_Seconds(#100)
    Wait_Milli_Seconds(#100)

    jb BUTTON_2, flash_seconds1_alarm
	;Wait_Milli_Seconds(#5)	
	jb BUTTON_2, flash_seconds1_alarm
	jnb BUTTON_2, $	

    ljmp edit_seconds_alarm

flash_seconds1_alarm:
    jb BUTTON_1, flash_seconds_alarm
	;Wait_Milli_Seconds(#5)	
	jb BUTTON_1, flash_seconds_alarm
	jnb BUTTON_1, $	

edit_AMPM_alarm:
    cpl am_pm_flag_alarm
    jb am_pm_flag_alarm, flash_PM_alarm
    ljmp flash_AM_alarm

flash_AM_alarm:
    Set_Cursor(2, 16)
    Send_Constant_String(#Empty2)

    Wait_Milli_Seconds(#100)
    Wait_Milli_Seconds(#100)

    Set_Cursor(2, 16)
    Send_Constant_String(#AM_display)

    Wait_Milli_Seconds(#100)
    Wait_Milli_Seconds(#100)

    jb BUTTON_2, flash_AM1_alarm	

    ljmp edit_AMPM_alarm

flash_PM_alarm:
    Set_Cursor(2, 16)
    Send_Constant_String(#Empty2)

    Wait_Milli_Seconds(#100)
    Wait_Milli_Seconds(#100)

    Set_Cursor(2, 16)
    Send_Constant_String(#PM_display)

    Wait_Milli_Seconds(#100)
    Wait_Milli_Seconds(#100)

    jb BUTTON_2, flash_PM1_alarm	

    ljmp edit_AMPM_alarm
jump4_alarm:
    ljmp flash_AM_alarm

flash_AM1_alarm:
    jb BUTTON_1, jump4_alarm

    ljmp confirm

flash_PM1_alarm:
    jb BUTTON_1, flash_PM_alarm

    ljmp confirm


edit_confirm:
    Set_Cursor(1, 1)
  
    ljmp edit_confirm

confirm:
    Set_Cursor(1, 1)
    Send_Constant_String(#ConfirmAlarm)
    
    Set_Cursor(2, 1)
    Send_Constant_String(#ChooseAlarm)
   
	jnb BUTTON_4, jump2
    jnb BUTTON_1, alarmIsSet
    sjmp confirm
;notcancel:
;    jb BUTTON_1, confirm ;checking for confirm, if not then go check from beginning
;	;Wait_Milli_Seconds(#5)	
;	jb BUTTON_1, confirm
;	jnb BUTTON_1, $	
;   
;    ljmp alarmisSet 
;
;confirm1:
;
;    jb BUTTON_1, confirm
;	;Wait_Milli_Seconds(#5)	
;	jb BUTTON_1, confirm
;	jnb BUTTON_1, $	
;
;    setb alarm_flag
;
;    ljmp confirm
jump2:
    ljmp cancelAlarm

alarmisSet:
    setb alarm_flag
    Set_Cursor(1, 1)
    ;Send_Constant_String(#Empty)
    Send_Constant_String(#AlarmSet)

    Set_Cursor(2, 1)
    Send_Constant_String(#Alarm)

    Set_Cursor(2, 8)
	Display_BCD(hour_count_alarm)

	Set_Cursor(2, 11)
	Display_BCD(minute_count_alarm)

	Set_Cursor(2, 14)
	Display_BCD(second_count_alarm)

    
    Wait_Milli_Seconds(#250)
    Wait_Milli_Seconds(#250)
    Wait_Milli_Seconds(#250)
    Wait_Milli_Seconds(#250)



    ljmp Timer2_ISR_done

cancelAlarm:
    Set_Cursor(1, 1)
    ;clr alarm_flag
    ;Send_Constant_String(#Empty)
    Send_Constant_String(#AlarmCancelled)
    
    Wait_Milli_Seconds(#250)
    Wait_Milli_Seconds(#250)
    Wait_Milli_Seconds(#250)

    ljmp Timer2_ISR_done





;---------------------------------;
; Main program. Includes hardware ;
; initialization and 'forever'    ;
; loop.                           ;
;---------------------------------;
setup:
	; Initialization
    mov SP, #0x7F
    clr alarm_flag
    lcall Timer0_Init
    lcall Timer2_Init
    ; In case you decide to use the pins of P0, configure the port in bidirectional mode:
    mov P0M0, #0
    mov P0M1, #0

    mov P2M0, #0
    mov P2M1, #0

    setb DEBUG

    setb EA   ; Enable Global interrupts
    lcall LCD_4BIT

    mov hour_count, #0x01
    mov minute_count, #0x00 
    mov second_count, #0x00
    


    ; For convenience a few handy macros are included in 'LCD_4bit.inc':
	Set_Cursor(1, 1)
    Send_Constant_String(#Time)
    Set_Cursor(2, 1)
    Send_Constant_String(#Alarm)

    Set_Cursor(2, 8)
	Display_BCD(hour_count_alarm)

	Set_Cursor(2, 11)
	Display_BCD(minute_count_alarm)

	Set_Cursor(2, 14)
	Display_BCD(second_count_alarm)

    jb am_pm_flag_alarm, main2
    Set_Cursor(2, 16)
	Send_Constant_String(#AM_display)

    Set_Cursor(1, 7)
	Display_BCD(hour_count)

	Set_Cursor(1, 10)
	Display_BCD(minute_count)

	Set_Cursor(1, 13)
	Display_BCD(second_count)

    Set_Cursor(1, 16)
    Send_Constant_String(#AM_display)

    setb half_seconds_flag ;set to 1
    ;clr AM_PM_flag
    clr TR0

    ljmp mainloop
    
main2:
    Set_Cursor(2, 16)
	Send_Constant_String(#PM_display)

    Set_Cursor(1, 7)
	Display_BCD(hour_count)

	Set_Cursor(1, 10)
	Display_BCD(minute_count)

	Set_Cursor(1, 13)
	Display_BCD(second_count)

    Set_Cursor(1, 16)
    Send_Constant_String(#AM_display)

    setb half_seconds_flag ;set to 1
    clr TR0
   


	;mov BCD_counter, #0x01

	
	; After initialization the program stays in this 'forever' loop
mainloop:
	jb BOOT_BUTTON, loop_a  ; if the 'BOOT' button is not pressed skip
	Wait_Milli_Seconds(#50)	; Debounce delay.  This macro is also in 'LCD_4bit.inc'
	jb BOOT_BUTTON, loop_a  ; if the 'BOOT' button is not pressed skip
	jnb BOOT_BUTTON, $		; Wait for button release.  The '$' means: jump to same instruction.
	; A valid press of the 'BOOT' button has been detected, reset the BCD counter.
	; But first stop timer 2 and reset the milli-seconds counter, to resync everything.
    ;Wait_Milli_Seconds(#5)


    
	clr TR2                 ; Stop timer 2
	clr a
	mov Count1ms+0, a
	mov Count1ms+1, a
	; Now clear the BCD counter
	mov BCD_counter, a
	setb TR2                ; Start timer 2
	sjmp loop_b             ; Display the new value


loop_a:
	jnb half_seconds_flag, mainloop ; jump to loop if not yet 1 second

loop_b:
    clr half_seconds_flag ; We clear this flag in the main loop, but it is set in the ISR for timer 2
	Set_Cursor(1, 7)
	Display_BCD(hour_count)

	Set_Cursor(1, 10)
	Display_BCD(minute_count)

	Set_Cursor(1, 13)
	Display_BCD(second_count)

    jb BUTTON_1, loop_a  ; if the 'BOOT' button is not pressed skip
	;Wait_Milli_Seconds(#5)	; Debounce delay.  This macro is also in 'LCD_4bit.inc'
	jb BUTTON_1, loop_a  ; if the 'BOOT' button is not pressed skip
	jnb BUTTON_1, $	

    lcall flash_clock
    sjmp mainloop



END

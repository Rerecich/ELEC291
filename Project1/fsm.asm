;define inputs
$include(LCD_class_demo.inc)
$include(LCD_4BIT.inc)
$include(math32.inc)
;include button fsm

;this might not be in the  right ordee i don't completely understand the dseg cseg stuff
dseg
state:      ds 1 ;will make debugging easier
temp_soak:  ds 1
time_soak:  ds 1
temp_refl:  ds 1
time_refl:  ds 1

sec:        ds 1

START_BUTTON   EQU P2.0 ;update as needed

;reset
org 0x0000
    ljmp setup


setup:
    mov SP, #7FH
    mov state, #0 
    mov a, state
  

;------------------------;
;  reset state, stays    ;
;  until start active    ;
;------------------------;
state0:
    cjne a, #0, state1           ;if not state 0, move to state 1
    mov pwm, #0                  ;power = 0%

    jb START_BUTTON, state0_done ;if start button pressed...(if not, skip to done then repeat)
    jnb START_BUTTON, $          ;wait for key release
    mov state, #1                ;change state variable

state0_done:
    ;should jump to harris's button stuff each "state_done" branch to check if parameters changed
    ljmp state0 
   

;------------------------;
;   stays for temp under ;
;   150 deg cel, 100%    ;
;   power                ;
;------------------------;       
state1:
    cjne a, #1, state2          ;state check       
    mov pwm, #100               ;power = 100%
    mov sec, #0                 ;time reset

state1_done:
    ;next 3: for calculating current temp
    mov a, temp_soak            ;from 150 deg (soak temp)
    clr c                       ;clr carry flag
    subb a, temp                ;a - temp = a

    jnc state1_done             ;if it's still a normal number, jump to done and repeat
    mov state, #2               ;else, state 2


;------------------------;
;  for time under 60     ;
;  power = 20%           ;
;------------------------;
state2:
    cjne a, #2, state3          ;state check
    mov pwm, #20                ;power = 20%

state2_done:
    ;next 3: figure out if at time soak yet
    mov a, time_soak            ;obv
    clr c                       ;clear carry flag
    subb a, sec                 ;a - sec = a

    jnc state2_done             ;if it's still a normal number, jump to done and repeat
    mov state, #3               ;else, state 3


;-------------------------;
;  temp under 220 deg,    ;
;  power = 100% and sec   ;
;  reset                  ;
;-------------------------;
state3:
    lcall check_abort           ;check if at 50 deg

    cjne a, #3, state4          ;state check       
    mov pwm, #100               ;power = 100%
    mov sec, #0                 ;time reset

state3_done:
    ;next 3: for calculating current temp
    mov a, temp_refl            ;from 220 deg (reflow temp)
    clr c                       ;clr carry flag
    subb a, temp                ;a - temp = a

    jnc state3_done             ;if it's still a normal number, jump to done and repeat
    mov state, #4               ;else, state 4


;--------------------------;
;  time under 45 sec,      ;
;  power = 20%             ;
;--------------------------;
state4:
    cjne a, #4, state5          ;state check
    mov pwm, #20                ;power = 20%

state4_done:
    ;next 3: figure out if at time soak yet
    mov a, time_refl            ;obv
    clr c                       ;clear carry flag
    subb a, sec                 ;a - sec = a

    jnc state4_done             ;if it's still a normal number, jump to done and repeat
    mov state, #5               ;else, state 5




;--------------------------;
;  temp over 60 deg,       ;
;  power = 0%              ;
;--------------------------;
state5:
    cjne a, #5, state0          ;state check       
    mov pwm, #0                 ;power = 0%

state5_done:
    ;next 3: for calculating current temp
    mov a, #60                  ;from 60 deg, but not sure what this called
    clr c                       ;clr carry flag
    subb a, temp                ;a - temp = a

    jc state5_done              ;if it's still a NOT normal number, jump to done and repeat
    mov state, #0               ;else, state 0


;--------------------------;
; subroutine for checking  ;
; to abort                 ;
;--------------------------;
check_abort:
    mov a, temp
    clr c
    subb a, #50

    jc abort

    ret

abort:
    ljmp state0






 
; timer - like lab 2






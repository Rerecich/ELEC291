#define F_CPU 22118400UL
#include <stdio.h>
#include <stdlib.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include "usart.h"


#include "lcd.h"

/* Pinout for DIP28 ATMega328P:

                           -------
     (PCINT14/RESET) PC6 -|1    28|- PC5 (ADC5/SCL/PCINT13)
       (PCINT16/RXD) PD0 -|2    27|- PC4 (ADC4/SDA/PCINT12)
       (PCINT17/TXD) PD1 -|3    26|- PC3 (ADC3/PCINT11)
      (PCINT18/INT0) PD2 -|4    25|- PC2 (ADC2/PCINT10)
 (PCINT19/OC2B/INT1) PD3 -|5    24|- PC1 (ADC1/PCINT9)
    (PCINT20/XCK/T0) PD4 -|6    23|- PC0 (ADC0/PCINT8)
                     VCC -|7    22|- GND
                     GND -|8    21|- AREF
(PCINT6/XTAL1/TOSC1) PB6 -|9    20|- AVCC
(PCINT7/XTAL2/TOSC2) PB7 -|10   19|- PB5 (SCK/PCINT5)
   (PCINT21/OC0B/T1) PD5 -|11   18|- PB4 (MISO/PCINT4)
 (PCINT22/OC0A/AIN0) PD6 -|12   17|- PB3 (MOSI/OC2A/PCINT3)
      (PCINT23/AIN1) PD7 -|13   16|- PB2 (SS/OC1B/PCINT2)
  (PCINT0/CLKO/ICP1) PB0 -|14   15|- PB1 (OC1A/PCINT1)
                           -------
*/

/**************************** D E C L A R A T I O N S **************************/

#define FORWARD 0.20
#define BACKWARD 0.30
#define LEFT 0.40
#define RIGHT 0.50
#define NOSIGNAL 5000

#define VOLTAGE_INPUT (PINB & 0b00000010) //will move this up as long as it doesn't mess with anything



#define ISR_FREQ 100000L // Interrupt service routine tick is 10 us
#define OCR1_RELOAD ((F_CPU/ISR_FREQ)-1)



/************************* I N I T I A L I Z A T I O N S***********************/

ISR(TIMER1_COMPA_vect)
{
// 'Timer 1 output compare A' Interrupt Service Routine
// This ISR happens at a rate of 100kHz.  It is used
// to generate the standard hobby servo 50Hz signal with
// a pulse width of 0.6ms to 2.4ms.

	//OCR1A = OCR1A + OCR1_RELOAD;
	ISR_cnt++;
	if(ISR_cnt<ISR_pw)
	{
		PORTB |= 0b00000001; // PB0=1
	}
	else
	{
		PORTB &= ~0b00000001; // PB0=0
	}
	if(ISR_cnt>=2000)
	{
		ISR_cnt=0; // 2000 * 10us=20ms
		ISR_frc++;
	}
}

ISR(TIMER0_OVF_vect) {
  // Increment the overflow counter every time Timer 0 overflows
  timer0_overflow_count++;
}

void timer_init0 (void)
{
     /*cli();// disable global interupt
    TCCR0A = 0;// set entire TCCR1A register to 0
    TCCR0B = 0;// same for TCCR1B
    TCNT0  = 0;//initialize counter value to 0
    // set compare match register for 100khz increments
    OCR0A = OCR0_RELOAD;// = (16*10^6) / (1*100000) - 1 (must be <255)   
    TCCR0B |= (1 << WGM12); // turn on CTC mode   
    TCCR0B |= (1 << CS10); // Set CS10 bits for 1 prescaler  
    TIMSK0 |= (1 << OCIE0A); // enable timer compare interrupt    

    sei(); // enable global interupt
	*/
	 
	// Set Timer 0 to normal mode and enable interrupts
  	TCCR0A = 0;
  	TCCR0B = 1 << CS00; // Clock Select: clk_io / 1
  	TIMSK0 = 1 << TOIE0; // Enable Timer 0 overflow interrupt

  	// Enable global interrupt
  	sei();
}

void timer_init1 (void)
{
	// Turn on timer with no prescaler on the clock.  We use it for delays and to measure period.
	TCCR1B |= _BV(CS10); // Check page 110 of ATmega328P datasheet
}

unsigned int TIM16_ReadTCNT1( void )
{
	// Atomic read of TCNT1. This comes from page 93 of the ATmega328P datasheet.

	unsigned char sreg;
	unsigned int i;
	/* Save global interrupt flag */
	sreg = SREG;
	/* Disable interrupts */
	cli();
	/* Read TCNT1 into i */
	i = TCNT1;
	/* Restore global interrupt flag */
	SREG = sreg;
	return i;
}

void adc_init(void)
{
    ADMUX = (1<<REFS0);
    ADCSRA = (1<<ADEN)|(1<<ADPS2)|(1<<ADPS1)|(1<<ADPS0);
}


void Configure_Pins(void)
{
    DDRB|=0b00000001; // PB0 is output.
    DDRD|=0b11111000; // PD3, PD4, PD5, PD6, and PD7 are outputs.
}


/******************************** T O O L S **********************************/

void delay_ms (int msecs)
{	
	int ticks;
	ISR_frc=0;
	ticks=msecs/20;
	while(ISR_frc<ticks);
}

long int GetDelayTimer1 (void)
{
	int overflow;
    unsigned int saved_TCNT1a, saved_TCNT1b;
	long int time_diff;

    overflow = 0;
    TIFR1 = 1; // Clear Timer/Counter1 Overflow Flag
    
	while (VOLTAGE_INPUT != 0) // Wait for square wave to be zero
    {
        if (TIFR1 & 1) { 
			TIFR1 = 1; 
			overflow++; 
			
			if (overflow > 5) //not sure why the overflow is 5 specifically
				return 0; 
		}
    }

    overflow = 0;
    TIFR1 = 1;
    saved_TCNT1a = TCNT1;
    
	while (VOLTAGE_INPUT == 0) // Wait for square wave to be 1
    {
        if (TIFR1 & 1) { 
			TIFR1 = 1; 
			overflow++; 
			
			if (overflow > 5) 
				return 0; 
		}
    }
    saved_TCNT1b = TCNT1;

	time_diff = saved_TCNT1b - saved_TCNT1a;
    
	return (time_diff);

}

long int GetDelayTimer0 (void)
{
	int overflow;
    unsigned int saved_TCNT0
	long int time_diff;

    overflow = 0;
    TIFR0 = 1; // Clear Timer/Counter0 Overflow Flag
    
	while (VOLTAGE_INPUT != 0) // Wait for square wave to be zero
    {
        if (TIFR0 & 1) { 
			TIFR0 = 1; 
			overflow++; 
			
			if (overflow > 5) //not sure why the overflow is 5 specifically
				return 0; 
		}
    }

    overflow = 0;
    TIFR1 = 1;
    saved_TCNT1a = TCNT1;
    
	while (VOLTAGE_INPUT == 0) // Wait for square wave to be 1
    {
        if (TIFR1 & 1) { 
			TIFR1 = 1; 
			overflow++; 
			
			if (overflow > 5) 
				return 0; 
		}
    }
    saved_TCNT1b = TCNT1;

	time_diff = saved_TCNT1b - saved_TCNT1a;
    
	return (time_diff);

}

uint16_t adc_read(int channel)
{
    channel &= 0x7;
    ADMUX = (ADMUX & 0xf8)|channel;
     
    ADCSRA |= (1<<ADSC);
     
    while(ADCSRA & (1<<ADSC)); //as long as ADSC pin is 1 just wait.
     
    return (ADCW);
}

void PrintNumber(long int N, int Base, int digits)
{ 
	char HexDigit[]="0123456789ABCDEF";
	int j;
	#define NBITS 32
	char buff[NBITS+1];
	buff[NBITS]=0;

	j=NBITS-1;
	while ( (N>0) | (digits>0) )
	{
		buff[j--]=HexDigit[N%Base];
		N/=Base;
		if(digits!=0) digits--;
	}
	usart_pstr(&buff[j+1]);
}


/******************************* R U N N I N G ********************************/

// /////////////////////////////////// //
// interprets signals from peak detec. //
// outputs translated instr. for serv. //
// /////////////////////////////////// //
int main(void)
{
    long int delay;
	//setup();

	Setup:

	usart_init(); // Configure the usart and baudrate
	Configure_Pins();
	timer_init0();
	timer_init1();

	LCD_4BIT(); //initialize 4-bit
	delay_ms(50);
	
	//need to declare inputs and outputs still

	// Turn on timer with no prescaler on the clock.  We use it for delays and to measure period.
	TCCR1B |= _BV(CS10); // Check page 110 of ATmega328P datasheet

	loop:
	while(1)
	{
		delay = GetDelay();

		switch(delay)
		{
			case FORWARD:
				goto moveForward;
				break;

			case BACKWARD;
				goto moveBackward;
				break;

			case LEFT;
				goto moveLeft;
				break;

			case RIGHT;
				goto moveRight;
				break;
			
			case NOSIGNAL;
				goto displayError;
				break;

			default:
				goto loop;
		}
		
	}

	moveForward:
		//servo commands
		goto loop;

	moveBackward:
		//servo commands
		goto loop;

	moveLeft:
		//servo commands
		goto loop;

	moveRight:
		//servo commands
		goto loop;
	
	displayError:
		LCDprint("Error: no signal.", 1,1);



	return 0; 
}

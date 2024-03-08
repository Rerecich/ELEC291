#define F_CPU 22118400UL

#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdio.h>
#include <stdbool.h>
#include <util/delay.h>
#include "usart.h"

void timer_init0 (void)
{
    cli();// disable global interupt
    TCCR0A = 0;// set entire TCCR1A register to 0
    TCCR0B = 0;// same for TCCR1B
    TCNT0  = 0;//initialize counter value to 0
    // set compare match register for 100khz increments
    OCR0A = OCR0_RELOAD;// = (16*10^6) / (1*100000) - 1 (must be <255)   
    TCCR0B |= (1 << WGM12); // turn on CTC mode   
    TCCR0B |= (1 << CS10); // Set CS10 bits for 1 prescaler  
    TIMSK0 |= (1 << OCIE0A); // enable timer compare interrupt    
    sei(); // enable global interupt
}

void timer_init1 (void)
{
	// Turn on timer with no prescaler on the clock.  We use it for delays and to measure period.
	TCCR1B |= _BV(CS10); // Check page 110 of ATmega328P datasheet
}

void wait_1ms(void)
{
	unsigned int saved_TCNT1;
	
	saved_TCNT1=TCNT1;
	
	while((TCNT1-saved_TCNT1)<(F_CPU/1000L)); // Wait for 1 ms to pass
}

void waitms(int ms)
{
	while(ms--) wait_1ms();
}

#define PIN_PERIOD (PINB & (1<<1)) // PB1

// GetPeriod() seems to work fine for frequencies between 30Hz and 300kHz.
long int GetPeriod (int n)
{
	int i, overflow;
	unsigned int saved_TCNT1a, saved_TCNT1b;
	
	overflow=0;
	TIFR1=1; // TOV1 can be cleared by writing a logic one to its bit location.  Check ATmega328P datasheet page 113.
	while (PIN_PERIOD!=0) // Wait for square wave to be 0
	{
		if(TIFR1&1)	{ TIFR1=1; overflow++; if(overflow>5) return 0;}
	}
	overflow=0;
	TIFR1=1;
	while (PIN_PERIOD==0) // Wait for square wave to be 1
	{
		if(TIFR1&1)	{ TIFR1=1; overflow++; if(overflow>5) return 0;}
	}
	
	overflow=0;
	TIFR1=1;
	saved_TCNT1a=TCNT1;
	for(i=0; i<n; i++) // Measure the time of 'n' periods
	{
		while (PIN_PERIOD!=0) // Wait for square wave to be 0
		{
			if(TIFR1&1)	{ TIFR1=1; overflow++; if(overflow>1024) return 0;}
		}
		while (PIN_PERIOD==0) // Wait for square wave to be 1
		{
			if(TIFR1&1)	{ TIFR1=1; overflow++; if(overflow>1024) return 0;}
		}
	}
	saved_TCNT1b=TCNT1;
	if(saved_TCNT1b<saved_TCNT1a) overflow--; // Added an extra overflow.  Get rid of it.

	return overflow*0x10000L+(saved_TCNT1b-saved_TCNT1a);
}

void adc_init(void)
{
    ADMUX = (1<<REFS0);
    ADCSRA = (1<<ADEN)|(1<<ADPS2)|(1<<ADPS1)|(1<<ADPS0);
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

void ConfigurePins (void)
{
	DDRB  &= 0b11111101; // Configure PB1 as input
	PORTB |= 0b00000010; // Activate pull-up in PB1
	
	DDRD  |= 0b11111100; // PD[7..2] configured as outputs
	PORTD &= 0b00000011; // PD[7..2] = 0
	
	DDRB  |= 0b00000001; // PB0 configured as output
	PORTB &= 0x11111110; // PB0 = 0
}

// In order to keep this as nimble as possible, avoid
// using floating point or printf() on any of its forms!
int main (void)
{
	unsigned int adc;
	unsigned long int v;
	long int count, f;
	unsigned char LED_toggle=0;
	
	usart_init(); // configure the usart and baudrate
	adc_init();
	ConfigurePins();
	timer_init0();
	timer_init1();

	waitms(500); // Wait for putty to start

	usart_pstr("\x1b[2J\x1b[1;1H"); // Clear screen using ANSI escape sequence.
	
	//DEBUG = 1;

	while(1)
	{
	
	
		// Now toggle the pins on/off to see if they are working.
		// First turn all off:
		//PORTD &= 0b11000011; // clear bits 2-6 of PORTD
		PORTD &= 0b10011111; // clear bits 5 and 6 of PORTD
		//PORTB &= 0x11111110; // PB0 = 0

		waitms(1000);

		//PORTD |= 0b00111100; // set bits 2-6 of PORTD
		PORTD |= 0b01100000; // set bits 5 and 6 of PORTD

		//PORTB |= (1<<0); // PB0 = 1


		waitms(500);


		//PORTD &= ~(1<<5); // PD5=0  

		//waitms(500);

		//PORTD |= (1<<5);

		//waitms(10);

		
	}
}

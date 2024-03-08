// PeriodEFM8.c: Measure the period of a signal on pin P0.1.
//
// By:  Jesus Calvino-Fraga (c) 2008-2018
//
// The next line clears the "C51 command line options:" field when compiling with CrossIDE
//  ~C51~  

#include <EFM8LB1.h> //why is this underlined?
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>


#define SYSCLK      72000000L  // SYSCLK frequency in Hz
#define BAUDRATE      115200L  // Baud rate of UART in bps

#define LCD_RS P2_6
// #define LCD_RW Px_x // Not used in this code.  Connect to GND
#define LCD_E  P2_5
#define LCD_D4 P2_4
#define LCD_D5 P2_3
#define LCD_D6 P2_2
#define LCD_D7 P2_1
#define CHARS_PER_LINE 16

#define PB_1 P1_5
#define PB_2 P0_5

#define ERROR_LIGHT P1_2
#define DEBUG_LIGHT P1_7


unsigned char overflow_count; //apparently this is a number

//setup shite
char _c51_external_startup (void)
{
	// Disable Watchdog with key sequence
	SFRPAGE = 0x00;
	WDTCN = 0xDE; //First key
	WDTCN = 0xAD; //Second key
  
	VDM0CN |= 0x80;
	RSTSRC = 0x02;

	#if (SYSCLK == 48000000L)	
		SFRPAGE = 0x10;
		PFE0CN  = 0x10; // SYSCLK < 50 MHz.
		SFRPAGE = 0x00;
	#elif (SYSCLK == 72000000L)
		SFRPAGE = 0x10;
		PFE0CN  = 0x20; // SYSCLK < 75 MHz.
		SFRPAGE = 0x00;
	#endif
	
	#if (SYSCLK == 12250000L)
		CLKSEL = 0x10;
		CLKSEL = 0x10;
		while ((CLKSEL & 0x80) == 0);
	#elif (SYSCLK == 24500000L)
		CLKSEL = 0x00;
		CLKSEL = 0x00;
		while ((CLKSEL & 0x80) == 0);
	#elif (SYSCLK == 48000000L)	
		// Before setting clock to 48 MHz, must transition to 24.5 MHz first
		CLKSEL = 0x00;
		CLKSEL = 0x00;
		while ((CLKSEL & 0x80) == 0);
		CLKSEL = 0x07;
		CLKSEL = 0x07;
		while ((CLKSEL & 0x80) == 0);
	#elif (SYSCLK == 72000000L)
		// Before setting clock to 72 MHz, must transition to 24.5 MHz first
		CLKSEL = 0x00;
		CLKSEL = 0x00;
		while ((CLKSEL & 0x80) == 0);
		CLKSEL = 0x03;
		CLKSEL = 0x03;
		while ((CLKSEL & 0x80) == 0);
	#else
		#error SYSCLK must be either 12250000L, 24500000L, 48000000L, or 72000000L
	#endif
	
	P0MDOUT |= 0x10; // Enable UART0 TX as push-pull output
	XBR0     = 0x01; // Enable UART0 on P0.4(TX) and P0.5(RX)                     
	XBR1     = 0X00;
	XBR2     = 0x40; // Enable crossbar and weak pull-ups

	#if (((SYSCLK/BAUDRATE)/(2L*12L))>0xFFL)
		#error Timer 0 reload value is incorrect because (SYSCLK/BAUDRATE)/(2L*12L) > 0xFF
	#endif
	// Configure Uart 0
	SCON0 = 0x10;
	CKCON0 |= 0b_0000_0000 ; // Timer 1 uses the system clock divided by 12.
	TH1 = 0x100-((SYSCLK/BAUDRATE)/(2L*12L));
	TL1 = TH1;      // Init Timer1
	TMOD &= ~0xf0;  // TMOD: timer 1 in 8-bit auto-reload
	TMOD |=  0x20;                       
	TR1 = 1; // START Timer1
	TI = 1;  // Indicate TX0 ready
	
	return 0;
}

// Uses Timer3 to delay <us> micro-seconds. 
void Timer3us(unsigned char us)
{
	unsigned char i;               // usec counter
	
	// The input for Timer 3 is selected as SYSCLK by setting T3ML (bit 6) of CKCON0:
	CKCON0|=0b_0100_0000;
	
	TMR3RL = (-(SYSCLK)/1000000L); // Set Timer3 to overflow in 1us.
	TMR3 = TMR3RL;                 // Initialize Timer3 for first overflow
	
	TMR3CN0 = 0x04;                 // Sart Timer3 and clear overflow flag
	for (i = 0; i < us; i++)       // Count <us> overflows
	{
		while (!(TMR3CN0 & 0x80));  // Wait for overflow
		TMR3CN0 &= ~(0x80);         // Clear overflow indicator
	}
	TMR3CN0 = 0 ;                   // Stop Timer3 and clear overflow flag
}

//wait function
void waitms (unsigned int ms)
{
	unsigned int j;
	for(j=ms; j!=0; j--)
	{
		Timer3us(249);
		Timer3us(249);
		Timer3us(249);
		Timer3us(250);
	}
}

//initialize timer 0
void TIMER0_Init(void)
{
	TMOD&=0b_1111_0000; // Set the bits of Timer/Counter 0 to zero
	TMOD|=0b_0000_0001; // Timer/Counter 0 used as a 16-bit timer
	TR0=0; // Stop Timer/Counter 0
}

//toggles LCD_E, the enable pin
void LCD_pulse (void)
{
LCD_E=1;
Timer3us(40);
LCD_E=0;
}

//not sure what this does
void LCD_byte (unsigned char x)
{
// The accumulator in the C8051Fxxx is bit addressable!
ACC=x; //Send high nible
LCD_D7=ACC_7;
LCD_D6=ACC_6;
LCD_D5=ACC_5;
LCD_D4=ACC_4;
LCD_pulse();
Timer3us(40);
ACC=x; //Send low nible
LCD_D7=ACC_3;
LCD_D6=ACC_2;
LCD_D5=ACC_1;
LCD_D4=ACC_0;
LCD_pulse();
}

void WriteData (unsigned char x)
{
LCD_RS=1; //reset?
LCD_byte(x); //function call using x... why
waitms(2);
}
void WriteCommand (unsigned char x)
{
LCD_RS=0; //reset?
LCD_byte(x);
waitms(5);
}
void LCD_4BIT (void) //ensure 4-bit mode
{
LCD_E=0; // Resting state of LCD's enable is zero
// LCD_RW=0; // We are only writing to the LCD in this program
waitms(20);
// First make sure the LCD is in 8-bit mode and then change to 4-bit mode
WriteCommand(0x33);
WriteCommand(0x33);
WriteCommand(0x32); // Change to 4-bit mode
// Configure the LCD
WriteCommand(0x28);
WriteCommand(0x0c);
WriteCommand(0x01); // Clear screen command (takes some time)
waitms(20); // Wait for clear screen command to finsih.
}
void LCDprint(char * string, unsigned char line, bit clear) //uses WriteCommand and WriteData
{
int j;
WriteCommand(line==2?0xc0:0x80); //WriteCommand takes unsigned character as parameter, this is choosing from... somewhere?
waitms(5);
for(j=0; string[j]!=0; j++) WriteData(string[j]);// Write the message
if(clear) for(; j<CHARS_PER_LINE; j++) WriteData(' '); // Clear the rest of the line
}

//for deciding length of putty input?
int getsn (char * buff, int len)
{
	int j;
	char c;
	for(j=0; j<(len-1); j++)
	{
		c=getchar();
			if ( (c=='\n') || (c=='\r') )
	{
	buff[j]=0;
	return j;
	}
		else
	{
	buff[j]=c;
	}
	}
	buff[j]=0;
return len;
}

//measure capacitance
//float capacitance_measure(void) 
//{
//	float period;
//	float capacitance;
//	float micro_capacitance;
//
//	while (1)
//    {
//    	// Reset the counter
//		TL0=0; 
//		TH0=0;
//		TF0=0;
//		overflow_count=0;
//
//        
//		
//		while(P0_1!=0); // Wait for the signal to be zero
//		LCDprint("debug",2,1);
//		while(P0_1!=1); // Wait for the signal to be one
//		
//		TR0=1; // Start the timer
//		while(P0_1!=0) // Wait for the signal to be zero
//		{
//           
//			if(TF0==1) // Did the 16-bit timer overflow?
//			{
//				TF0=0;
//				overflow_count++;
//                
//			}
//		}
//		while(P0_1!=1) // Wait for the signal to be one
//		{
//			
//			if(TF0==1) // Did the 16-bit timer overflow?
//			{
//				TF0=0;
//				overflow_count++;
//			}
//		}
//
//		TR0=0; // Stop timer 0, the 24-bit number [overflow_count-TH0-TL0] has the period!
//		period=(overflow_count*65536.0+TH0*256.0+TL0)*(12.0/SYSCLK);
//		
//		capacitance = (1.44*period)/3000;
//		
//		// Send the period to the serial port
//		//printf( "\rT=%f ms    \n", period*1000.0);
//		micro_capacitance = capacitance*1000000;
//	}
//	return capacitance;
//}

//calculate capacitance
float calc_cap (void)
{
	float period;
	float capacitance;
	//DEBUG_LIGHT = 0; good
   
    	// Reset the counter
		TL0=0; 
		TH0=0;
		TF0=0;
		overflow_count=0;

//DEBUG_LIGHT = 0; good

		while(P0_1!=0); // Wait for the signal to be zero
		while(P0_1!=1); // Wait for the signal to be one
		TR0=1; // Start the timer
		
		while(P0_1!=0) // Wait for the signal to be zero
		{
		
			if(TF0==1) // Did the 16-bit timer overflow?
			{
				TF0=0;
				overflow_count++;

			}
		}
		while(P0_1!=1) // Wait for the signal to be one
		{

			if(TF0==1) // Did the 16-bit timer overflow?
			{
				TF0=0;
				overflow_count++;
			}
		}

		TR0=0; // Stop timer 0, the 24-bit number [overflow_count-TH0-TL0] has the period!
		period=(overflow_count*65536.0+TH0*256.0+TL0)*(12.0/SYSCLK);

		capacitance = (1.44*period)/3000;

		// Send the period to the serial port
		//printf( "\rT=%f ms    \n", period*1000.0);
		//micro_capacitance = capacitance*1000000;
	return capacitance;
}

double calc_error(float actual, float expected)
{
	float percent_error;

	percent_error = (((actual-expected)/expected)*100);

	return percent_error*(-1);
}

void main (void) 
{
	float capacitance;
	float micro_capacitance;
	char str[10];
	float cap_input_int;
	float percent_error;

	char input[17];
	char cap_input[17];
	char check1[] = "uF";
	char check2[] = "F";

	ERROR_LIGHT = 1; 

	LCD_4BIT(); //initialize 4-bit
	TIMER0_Init(); //initialize timer 0

	waitms(500); // Give PuTTY a chance to start.
	printf("\x1b[2J"); // Clear screen using ANSI escape sequence.

	printf ("EFM8 Capacitance measurement at pin P0.1 using Timer 0.\n"
	        "File: %s\n"
	        "Compiled: %s, %s\n\n",
	        __FILE__, __DATE__, __TIME__);
	
	printf("Once you have selected mode and units (if applicaple), use button 1 to toggle modes and button 2 to toggle units \n");
	printf("See LCD to select mode");
	       
	LCDprint("Choose mode:",1,1);
	LCDprint("Measure    Error",2,1);

	while((PB_1 ^ PB_2) == 0); //wait for press
	while((PB_1 ^ PB_2) == 1) //wait for release
	{
		//this works fine
		if(PB_1 == 0)
		{
			CapacitanceMeasurement:
			//ERROR_LIGHT = 0; 
			printf("\n");
			printf("Measuring capacitance");
            LCDprint("Selecting units",1,1);
			LCDprint("(see PuTTY)",2,1);

			printf("\n");

			EnterUnits:
			//ERROR_LIGHT = 0; 
			printf("Enter units - uF or F: ");
			//get units input 
			getsn(input, sizeof(input));
			printf("\n");
			//printf("Click button 2 to toggle units or button 1 to toggle modes");
			printf("\n");
			//check units input
			if (strcmp(check1,input) == 0) //if it's uF
			{
				MicroFarads:
				//ERROR_LIGHT = 0; 
				LCDprint("Capacitance (uF)",1,1);
				while(1) {
				capacitance = calc_cap();
				micro_capacitance = 1000000*capacitance;
				printf( "\rC=%f uF", micro_capacitance);

				sprintf(str, "%f",micro_capacitance);
				LCDprint(str, 2,1);

				if (PB_2 == 0) {
					while (PB_2 == 1);
					goto Farads;
				}

				if (PB_1 == 0) {
					while (PB_1 == 1);
					goto PercentError;
				}
				}
			}
		
			else if (strcmp(check2,input) == 0) //if it's F
			{
				Farads:
				
				LCDprint("Capacitance (F)",1,1);
				
				while (1) { 
					
    				capacitance = calc_cap();
					DEBUG_LIGHT = 0; //not passing this function whyyy
					micro_capacitance = 1000000*capacitance;
					printf( "\rC=%f F", capacitance);
					

					sprintf(str, "%f",capacitance);
					LCDprint(str, 2,1);

					if (PB_2 == 0) {
						while(PB_2==1);
						goto MicroFarads;
				}
				if (PB_1 == 0) {
					while (PB_1 == 1);
					LCDprint("MODE SWITCHED",1,1);
					waitms(1000);
					goto PercentError;
				}
				}
			}

			else //invalid
			{
				ERROR_LIGHT = 0; 
				printf("Invalid input.");
				printf("\n");
				waitms(1000);
				ERROR_LIGHT = 1;
				goto EnterUnits;
			}
		}

		//percent error mode
		else if(PB_2 == 0){
			PercentError:
			LCDprint("Enter capacitance",2, 1);
			//ERROR_LIGHT = 0; 
			printf("\n");
			printf("Measuring capacitance percent error");
			printf("\n");
			printf("Enter capacitor value in uF: ");
			
			//not working for some reason
			getsn(cap_input, sizeof(cap_input));
			

			printf("\n");
			
			//printf(cap_input);
			//LCDprint(cap_input,2,1);

			//cap_input_int = atof(cap_input);
			cap_input_int = 10;

			//printf("\n");
			//printf("cap_input_int = %f",cap_input_int);
			//waitms(2000);
			LCDprint("Percent error: ",1,1);
			while(1) {
				capacitance = calc_cap();
				micro_capacitance = 1000000*capacitance;
				waitms(200);
				percent_error = calc_error(micro_capacitance, cap_input_int);
				printf( "\r%% error = %f %%", percent_error);

				sprintf(str, "%lf",percent_error);
				LCDprint(str, 2,1);

				if (PB_1 == 0) {
					while (PB_1 == 1);
					LCDprint("MODE SWITCHED",1,1);
					goto CapacitanceMeasurement;
				}
			
			}
		}
	}



	
//	printf("Choose units: uF [1] or F [2]");
//	//get units input 
//		getsn(input, sizeof(input));
//		printf("\n");
//	//check units input
//		while (strcmp(check1,input) == 0) //if it's 1
//		{
//			LCDprint("Capacitance (uF)",1,1);
//			micro_capacitance = capacitance*1000000;
//
//			printf( "\rC=%f uF    ", micro_capacitance);
//		
//			sprintf(str, "%f", micro_capacitance);
//			LCDprint(str, 2,1);
//		}
//		
//		while (strcmp(check2,input) == 0) //if it's 2 (or technically anything else but shh)
//		{
//			LCDprint("Capacitance (F)",1,1);
//			printf( "\rC=%f F    ", capacitance);
//		
//			sprintf(str, "%f",capacitance);
//			LCDprint(str, 2,1);
//		}
	
	
	

	
}


 
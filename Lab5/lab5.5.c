// ADC.c:  Shows how to use the 14-bit ADC.  This program
// measures the voltage from some pins of the EFM8LB1 using the ADC.
//
// (c) 2008-2018, Jesus Calvino-Fraga
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <EFM8LB1.h>


// ~C51~  

#define SYSCLK 72000000L
#define BAUDRATE 115200L
#define SARCLK 18000000L

#define LCD_RS P2_6
// #define LCD_RW Px_x // Not used in this code.  Connect to GND
#define LCD_E  P2_5
#define LCD_D4 P2_4
#define LCD_D5 P2_3
#define LCD_D6 P2_2
#define LCD_D7 P2_1
#define CHARS_PER_LINE 16


#define PB_1 !P1_5
#define PB_2 !P0_5

#define REFERENCE QFP32_MUX_P1_0
#define TEST QFP32_MUX_P1_1

//Regular text
#define BLK "\033[0;30m"
#define RED "\033[0;31m"
#define GRN "\033[0;32m"
#define YEL "\033[0;33m"
#define BLU "\033[0;34m"
#define MAG "\033[0;35m"
#define CYN "\033[0;36m"
#define WHT "\033[0;37m"

//Regular bold text
#define BBLK "\033[1;30m"
#define BRED "\033[1;31m"
#define BGRN "\033[1;32m"
#define BYEL "\033[1;33m"
#define BBLU "\033[1;34m"
#define BMAG "\033[1;35m"
#define BCYN "\033[1;36m"
#define BWHT "\033[1;37m"

//Regular underline text
#define UBLK "\033[4;30m"
#define URED "\033[4;31m"
#define UGRN "\033[4;32m"
#define UYEL "\033[4;33m"
#define UBLU "\033[4;34m"
#define UMAG "\033[4;35m"
#define UCYN "\033[4;36m"
#define UWHT "\033[4;37m"

//Regular background
#define BLKB "\033[40m"
#define REDB "\033[41m"
#define GRNB "\033[42m"
#define YELB "\033[43m"
#define BLUB "\033[44m"
#define MAGB "\033[45m"
#define CYNB "\033[46m"
#define WHTB "\033[47m"

//High intensty background 
#define BLKHB "\033[0;100m"
#define REDHB "\033[0;101m"
#define GRNHB "\033[0;102m"
#define YELHB "\033[0;103m"
#define BLUHB "\033[0;104m"
#define MAGHB "\033[0;105m"
#define CYNHB "\033[0;106m"
#define WHTHB "\033[0;107m"

//High intensty text
#define HBLK "\033[0;90m"
#define HRED "\033[0;91m"
#define HGRN "\033[0;92m"
#define HYEL "\033[0;93m"
#define HBLU "\033[0;94m"
#define HMAG "\033[0;95m"
#define HCYN "\033[0;96m"
#define HWHT "\033[0;97m"

//Bold high intensity text
#define BHBLK "\033[1;90m"
#define BHRED "\033[1;91m"
#define BHGRN "\033[1;92m"
#define BHYEL "\033[1;93m"
#define BHBLU "\033[1;94m"
#define BHMAG "\033[1;95m"
#define BHCYN "\033[1;96m"
#define BHWHT "\033[1;97m"

//Reset
#define reset "\033[0m"
#define CRESET "\033[0m"
#define COLOR_RESET "\033[0m"




unsigned char overflow_count; //a number apparently
float period;
float half_period;
float quarter_period;

float frequency;
float phase_difference;

float vrmsRef;
float vrmsTest;

char _c51_external_startup (void)
{
	// Disable Watchdog with key sequence
	SFRPAGE = 0x00;
	WDTCN = 0xDE; //First key
	WDTCN = 0xAD; //Second key
  
	VDM0CN=0x80;       // enable VDD monitor
	RSTSRC=0x02|0x04;  // Enable reset on missing clock detector and VDD

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

	// Configure Uart 0
	#if (((SYSCLK/BAUDRATE)/(2L*12L))>0xFFL)
		#error Timer 0 reload value is incorrect because (SYSCLK/BAUDRATE)/(2L*12L) > 0xFF
	#endif
	SCON0 = 0x10;
	TH1 = 0x100-((SYSCLK/BAUDRATE)/(2L*12L));
	TL1 = TH1;      // Init Timer1
	TMOD &= ~0xf0;  // TMOD: timer 1 in 8-bit auto-reload
	TMOD |=  0x20;                       
	TR1 = 1; // START Timer1
	TI = 1;  // Indicate TX0 ready
  	
	return 0;
}

void InitADC (void)
{
	SFRPAGE = 0x00;
	ADEN=0; // Disable ADC
	
	ADC0CN1=
		(0x2 << 6) | // 0x0: 10-bit, 0x1: 12-bit, 0x2: 14-bit
        (0x0 << 3) | // 0x0: No shift. 0x1: Shift right 1 bit. 0x2: Shift right 2 bits. 0x3: Shift right 3 bits.		
		(0x0 << 0) ; // Accumulate n conversions: 0x0: 1, 0x1:4, 0x2:8, 0x3:16, 0x4:32
	
	ADC0CF0=
	    ((SYSCLK/SARCLK) << 3) | // SAR Clock Divider. Max is 18MHz. Fsarclk = (Fadcclk) / (ADSC + 1)
		(0x0 << 2); // 0:SYSCLK ADCCLK = SYSCLK. 1:HFOSC0 ADCCLK = HFOSC0.
	
	ADC0CF1=
		(0 << 7)   | // 0: Disable low power mode. 1: Enable low power mode.
		(0x1E << 0); // Conversion Tracking Time. Tadtk = ADTK / (Fsarclk)
	
	ADC0CN0 =
		(0x0 << 7) | // ADEN. 0: Disable ADC0. 1: Enable ADC0.
		(0x0 << 6) | // IPOEN. 0: Keep ADC powered on when ADEN is 1. 1: Power down when ADC is idle.
		(0x0 << 5) | // ADINT. Set by hardware upon completion of a data conversion. Must be cleared by firmware.
		(0x0 << 4) | // ADBUSY. Writing 1 to this bit initiates an ADC conversion when ADCM = 000. This bit should not be polled to indicate when a conversion is complete. Instead, the ADINT bit should be used when polling for conversion completion.
		(0x0 << 3) | // ADWINT. Set by hardware when the contents of ADC0H:ADC0L fall within the window specified by ADC0GTH:ADC0GTL and ADC0LTH:ADC0LTL. Can trigger an interrupt. Must be cleared by firmware.
		(0x0 << 2) | // ADGN (Gain Control). 0x0: PGA gain=1. 0x1: PGA gain=0.75. 0x2: PGA gain=0.5. 0x3: PGA gain=0.25.
		(0x0 << 0) ; // TEMPE. 0: Disable the Temperature Sensor. 1: Enable the Temperature Sensor.

	ADC0CF2= 
		(0x0 << 7) | // GNDSL. 0: reference is the GND pin. 1: reference is the AGND pin.
		(0x1 << 5) | // REFSL. 0x0: VREF pin (external or on-chip). 0x1: VDD pin. 0x2: 1.8V. 0x3: internal voltage reference.
		(0x1F << 0); // ADPWR. Power Up Delay Time. Tpwrtime = ((4 * (ADPWR + 1)) + 2) / (Fadcclk)
	
	ADC0CN2 =
		(0x0 << 7) | // PACEN. 0x0: The ADC accumulator is over-written.  0x1: The ADC accumulator adds to results.
		(0x0 << 0) ; // ADCM. 0x0: ADBUSY, 0x1: TIMER0, 0x2: TIMER2, 0x3: TIMER3, 0x4: CNVSTR, 0x5: CEX5, 0x6: TIMER4, 0x7: TIMER5, 0x8: CLU0, 0x9: CLU1, 0xA: CLU2, 0xB: CLU3

	ADEN=1; // Enable ADC
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

void waitms (unsigned int ms)
{
	unsigned int j;
	unsigned char k;
	for(j=0; j<ms; j++)
		for (k=0; k<4; k++) Timer3us(250);
}


//initialize timer 0
void TIMER0_Init(void)
{
	TMOD&=0b_1111_0000; // Set the bits of Timer/Counter 0 to zero
	TMOD|=0b_0000_0001; // Timer/Counter 0 used as a 16-bit timer
	TR0=0; // Stop Timer/Counter 0
}

///////////////////////////////////////////////////////////
void LCD_pulse (void)
{
	LCD_E=1;
	Timer3us(40);
	LCD_E=0;
}

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
	LCD_RS=1;
	LCD_byte(x);
	waitms(2);
}

void WriteCommand (unsigned char x)
{
	LCD_RS=0;
	LCD_byte(x);
	waitms(5);
}

void LCD_4BIT (void)
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

void LCDprint(char * string, unsigned char line, bit clear)
{
	int j;

	WriteCommand(line==2?0xc0:0x80);
	waitms(5);
	for(j=0; string[j]!=0; j++)	WriteData(string[j]);// Write the message
	if(clear) for(; j<CHARS_PER_LINE; j++) WriteData(' '); // Clear the rest of the line
}

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
///////////////////////////////////////////////////////////
#define VDD 3.3035 // The measured value of VDD in volts

void InitPinADC (unsigned char portno, unsigned char pin_num)
{
	unsigned char mask;
	
	mask=1<<pin_num;

	SFRPAGE = 0x20;
	switch (portno)
	{
		case 0:
			P0MDIN &= (~mask); // Set pin as analog input
			P0SKIP |= mask; // Skip Crossbar decoding for this pin
		break;
		case 1:
			P1MDIN &= (~mask); // Set pin as analog input
			P1SKIP |= mask; // Skip Crossbar decoding for this pin
		break;
		case 2:
			P2MDIN &= (~mask); // Set pin as analog input
			P2SKIP |= mask; // Skip Crossbar decoding for this pin
		break;
		default:
		break;
	}
	SFRPAGE = 0x00;
}

unsigned int ADC_at_Pin(unsigned char pin)
{
	ADC0MX = pin;   // Select input from pin
	ADINT = 0;
	ADBUSY = 1;     // Convert voltage at the pin
	while (!ADINT); // Wait for conversion to complete
	return (ADC0);
}

float Volts_at_Pin(unsigned char pin)
{
	 return ((ADC_at_Pin(pin)*VDD)/16383.0);
}

//for reference: using timer 0 to measure period
unsigned int Measure_Period_Timer0(void)
{
	unsigned int Period;

	// Measure half period at pin P1.0 using timer 0
	TR0=0; // Stop timer 0 (TR0 is control bit)
	TMOD=0B_0000_0001; // Set timer 0 as 16-bit timer
	TH0=0; TL0=0; // Reset the timer
	while (P1_0==1); // Wait for the signal to be zero
	while (P1_0==0); // Wait for the signal to be one
	TR0=1; // Start timing
	while (P1_0==1); // Wait for the signal to be zero
	TR0=0; // Stop timer 0
	// [TH0,TL0] is half the period in multiples of 12/CLK, so:
	Period=(TH0*0x100+TL0)*2; // Assume Period is unsigned int

	return Period;
}

unsigned int Get_ADC (void)
{
	ADINT = 0;
	ADBUSY = 1;
	while (!ADINT); // Wait for conversion to complete
	return (ADC0);
}

void Period_Frequency_ADC(void)
{
	float half_period;
	// Start tracking the reference signal
	TF0=0; 
	overflow_count=0;
	
	ADC0MX = REFERENCE;
	ADINT = 0; //interrupt flag
	ADBUSY=1; //
	
	while (!ADINT); // Wait for conversion to complete
	
	// Reset the timer
	TL0=0; 
	TH0=0;
	
	while (Get_ADC()>150); // Wait for the signal to be zero
	
	while (Get_ADC()<150); // Wait for the signal to be positive
	TR0=1; // Start the timer 0
	while (Get_ADC()>150); // Wait for the signal to be zero again
	TR0=0; // Stop timer 0

	period=2*(overflow_count*65536.0+TH0*256.0+TL0)*(12.0/SYSCLK);

	half_period = period/2; //pretty sure this can be accessed globally
	frequency = 1/period; //this too
    quarter_period = period/4;

	// Time from the beginning of the sine wave to its peak
	//overflow_count=65536-(quarter_period);
	
}

void Peak_Voltage(void) 
{
    TF0 = 0;
    overflow_count = 0;
    ADC0MX = REFERENCE; 
    ADINT = 0; //interrupt flag
	ADBUSY=1; 

	while (!ADINT); // Wait for conversion to complete
    while(Get_ADC()>150);
    while(Get_ADC()<150);
    
    overflow_count=0; //starting fresh, timers have both been reset
    //TF0 = 0; //to start set flag to 0
    
   // ADC0MX = REFERENCE; 
    //ADINT = 0; //interrupt flag
	//ADBUSY=1; //
	//while (!ADINT); // Wait for conversion to complete
	
	// Reset the timer
	TL0=0; 
    TH0=0;
	//while (Get_ADC()>150); // Wait for the signal to be zero
	//while (Get_ADC()<150); // Wait for the signal to be positive
	
   

    TR0 = 1; //start timer
   
    //time difference
    while ((overflow_count*65536.0+TH0*256.0+TL0)*(12.0/SYSCLK)<quarter_period)     {
        if(TF0==1) // Did the 16-bit timer overflow?
			{
				TF0=0;
				overflow_count++;
			}
    }

    TR0 = 0; //stop timer
    vrmsRef = Volts_at_Pin(REFERENCE)/1.414;

    ///////////////////////////////////////////////////////////////////
    TF0 = 0;
    overflow_count = 0;
    ADC0MX = TEST; 
    ADINT = 0; //interrupt flag
	ADBUSY=1; //
	while (!ADINT); // Wait for conversion to complete
	
	// Reset the timer
	TL0=0; 
    TH0=0;
	while (Get_ADC()<150); // Wait for the signal to be zero
	while (Get_ADC()>150); // Wait for the signal to be positive
	
    overflow_count=0; //starting fresh, timers have both been reset

    TL0=0; 
    TH0=0;

    TR0 = 1; //start timer
   
    //time difference
    while ((overflow_count*65536.0+TH0*256.0+TL0)*(12.0/SYSCLK)<quarter_period) 
    {
        if(TF0==1) // Did the 16-bit timer overflow?
			{
				TF0=0;
				overflow_count++;
			}
    }

    TR0 = 0; //stop timer
    vrmsTest = Volts_at_Pin(TEST)/1.414;
    
}

void PhaseDifference(void)
{
    float time_1;
    //get time that reference crosses 0//////////////////////////////////
    ADC0MX=REFERENCE;
	ADINT = 0;
	ADBUSY=1;
	while (!ADINT); // Wait for conversion to complete
    overflow_count = 0;
	
	TL0=0; 
	TH0=0;
    while (Get_ADC()>150); // Wait for the signal to be zero
	while (Get_ADC()<150); // Wait for the signal to be positive
	
    //overflow_count=0; //starting fresh, timers have both been reset

    TR0 = 1; //start timer

    //get time that test crosses 0//////////////////////////////////////
    ADC0MX=TEST;
	ADINT = 0;
	ADBUSY=1;
	while (!ADINT); // Wait for conversion to complete
   
   while(Get_ADC()>150)
   { 
			if(TF0==1) // Did the 16-bit timer overflow?
			{
				TF0=0;
				overflow_count++;
			}
		}
	while(Get_ADC()<150)
    {  
			if(TF0==1) // Did the 16-bit timer overflow?
			{
				TF0=0;
				overflow_count++;
			}

	}
    TR0 = 0;
    time_1 = (overflow_count*65536.0+TH0*256.0+TL0)*(12.0/SYSCLK);
    phase_difference = time_1*(360.0/period);
}

void main (void)
{
    char input[18];
    char str[17];

    
    Setup:
    
    //printf(BKF_WTB);
    LCDprint("    ---RESET---   ", 1,1);
    LCD_4BIT(); //initialize 4-bit
	TIMER0_Init(); //initialize timer 0

    
	InitPinADC(1, 0); // P1.0 reference - doesn't seem to work
	InitPinADC(1, 1); // P1.1 test
    InitADC();


    waitms(500); // Give PuTTy a chance to start before sending
	printf("\x1b[2J"); // Clear screen using ANSI escape sequence.
	
    

	printf(BBLK"ADC test program\n"
	        "File: %s\n"
	        "Compiled: %s, %s\n\n",
	        __FILE__, __DATE__, __TIME__);
    printf(reset);

    //all the calcs


    Basics:
    while(1) {
		
		Period_Frequency_ADC();
        Peak_Voltage();
        PhaseDifference();
		//LCDprint("test",1,1);
		

        printf("test: %5.3f V || ref: %5.3f V|| period: %5.3f s|| phase shift: %5.3f\r", vrmsTest, vrmsRef, period, phase_difference);

        if (PB_1 == 1) {
            goto InputMode;
        }
    }

    InputMode:
    printf("What would you like to display on the LCD?\n");
    printf("Choices: test voltage, reference voltage, or phase shift\n");

    getsn(input, sizeof(input));
    printf("\n");

    if (strcmp(input,"test voltage")==0) //if test voltage
    {
        DisplayTestVoltage:
        LCDprint("Test voltage:     \n",1,1);
        while(1) {
            Period_Frequency_ADC();
            Peak_Voltage();

            sprintf(str, "%2.5f V", vrmsTest);
            LCDprint(str, 2,1);

            if(PB_1 == 1) {
                goto DisplayReferenceVoltage;
            }
        }
        
    }

    else if (strcmp(input,"reference voltage")==0) //if reference voltage
    {
        DisplayReferenceVoltage:
        LCDprint("Reference voltage:     \n",1,1);
        while(1) {
            Period_Frequency_ADC();
            Peak_Voltage();

            sprintf(str, "%2.5f V", vrmsRef);
            LCDprint(str, 2,1);

            if(PB_1 == 1) {
                goto DisplayPhaseShift;
            }
        }
        
    }

    
    else if (strcmp(input,"phase shift")==0) //if reference voltage
    {
        DisplayPhaseShift:
        LCDprint("Phase Shift:     \n",1,1);
        while(1) {
            Period_Frequency_ADC();
            Peak_Voltage();
            PhaseDifference();

            sprintf(str, "%3.5f deg", phase_difference);
            LCDprint(str, 2,1);

            if(PB_1 == 1) {
                goto DisplayTestVoltage;
            }
        }
        
    }
//spelling mistakes
    else if(strcmp(input,"phase")==0)
    {
            printf(YEL"Did you mean phase shift? [Y/N]");
            getsn(input, sizeof(input));

            if(strcmp(input,"Y")==0) {
                goto DisplayPhaseShift;
            }

            else {
                goto Error;
            }
        } 
    
    else if(strcmp(input,"reference")==0) {
            printf(YEL"Did you mean reference voltage? [Y/N]");

            getsn(input, sizeof(input));

            if(strcmp(input,"Y")==0) {
                goto DisplayReferenceVoltage;
            }

            else {
                goto Error;
            }
        } 

    else if(strcmp(input,"test")==0) {
            printf(YEL"Did you mean test voltage? [Y/N]");

            getsn(input, sizeof(input));

            if(strcmp(input,"Y")==0) {
                goto DisplayTestVoltage;
            }

            else {
                goto Error;
            }
        } 

//automatic adjustments
    else if(strcmp(input,"phaseshift")==0) {
            goto DisplayPhaseShift;
        } 
    else if(strcmp(input,"PhaseShift")==0) {
            goto DisplayPhaseShift;
        } 
    else if(strcmp(input,"Phase Shift")==0) {
            goto DisplayPhaseShift;
        }     
    else if(strcmp(input," phase shift")==0) {
            goto DisplayPhaseShift;
        }  
    else if(strcmp(input,"referencevoltage")==0) {
            goto DisplayReferenceVoltage;
        } 
    else if(strcmp(input," referencevoltage")==0) {
            goto DisplayReferenceVoltage;
        } 
    else if(strcmp(input," reference voltage")==0) {
            goto DisplayReferenceVoltage;
        } 
    
    else if(strcmp(input,"testvoltage")==0) {
            printf(RED"Did you mean test voltage? [Y/N]");

            getsn(input, sizeof(input));

            if(strcmp(input,"Y")==0) {
                goto DisplayTestVoltage;
            }

            else {
                goto Error;
            }
        } 

//error
    else {
            Error:
            printf(RED"ERROR: invalid input. Resetting.");
            waitms(1000);

            goto Setup;
        } 

  

}



   

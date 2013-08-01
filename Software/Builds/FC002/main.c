#include <msp430.h> 
#include "textconv.h"

/*
 * Frequency Counter project - 002
 * main.c
 */

unsigned long counter;
unsigned char bcd[8];

int main(void)
{
	// Setup Watchdog timer to provide the 1s measurement time
	
    // WDTSSEL -  1 - Source ACLK (32.768 MHz XCO)
    // WDTISx -  00 - Freq 1Hz (clock/32768)
    // WDTTMSEL - 1 - Intervall mode
    WDTCTL = WDTPW | WDTSSEL | WDTTMSEL;
    IE1 |= WDTIE; // Enable WDT interrupt

    // Set 16MHz clock
	BCSCTL1 = CALBC1_16MHZ;
	DCOCTL = CALDCO_16MHZ;


	// initialize long counter
	counter = 0;

	// Initialize frequency input
	P1DIR &= ~BIT0;
	P1SEL |= BIT0;
	P1SEL2 &= ~BIT0;

	// Initialize Multiplex output
	P1SEL &= ~(BIT3 + BIT4 + BIT5);
	P1SEL2 &= ~(BIT3 + BIT4 + BIT5);
	P1DIR |= BIT3 + BIT4 + BIT5;

	// Initialize 7 Segment output
	P2SEL &= ~(BIT0 + BIT1 + BIT2 + BIT3 + BIT4);
	P2SEL2 &= ~(BIT0 + BIT1 + BIT2 + BIT3 + BIT4);
	P2DIR |= BIT0 + BIT1 + BIT2 + BIT3 + BIT4;

	// Initialize Timer_A0
	// TACTL register
	// BIT 9-8 - 00 - Source: TACLK (TASSEL_0)
	// BIT 7-6 - 00 - Input divider: /1
	// BIT 5-4 - 10 - Mode: Continuous (MC_2)
	// BIT 1   -  1 - Enable TAIV interrupt (TAIE)
	TA0CTL = TASSEL_0 + MC_2 + TAIE;

    _EINT();	// Enable interrupts

    unsigned char i;
    // i2c_init(0x39, 12);
    // infinite loop
    while (1)
    {
    	for(i=0;i<8;i++)
    	{
    		P2OUT &= 0xE0;
    		P1OUT &= 0xC7;
    		// __delay_cycles(1500);
    		P1OUT |= i << 3;
    		P2OUT |= (bcd[i] ^ 0x10);
    		__delay_cycles(2000);
    	}
    }

	return 0;
}

// Watchdog timer interrupt
// Display the count result
#pragma vector=WDT_VECTOR
__interrupt void WDT_ISR(void)
{
	// capture timer
	unsigned int tarvalue;
	tarvalue = TAR;
	// process data
	unsigned long freq;
	freq = counter + tarvalue;
	LongToBCD(8,bcd,freq,0);
	// clear master counter
	counter = 0;
	// reset WDT
	WDTCTL = WDTPW | WDTSSEL | WDTTMSEL | WDTCNTCL;
	// reset timer
	TACTL |= TACLR;
	// Clear interrupt
	IFG1 &= ~WDTIFG;
}

// Timer_A3 Interrupt Vector (TA0IV) handler
#pragma vector=TIMER0_A1_VECTOR
__interrupt void TA0IV_ISR(void)
{
	if(TAIV == 10)
	{
		counter += 0x10000;
	}
}

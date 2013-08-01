#include <msp430.h> 
#include "textconv.h"

/*
 * Frequency Counter project - 002A
 * main.c
 */

const char segments[] = {0x3F,0x06, 0x5B, 0x4F, 0x66, 0x6D, 0x7D, 0x07, 0x7F, 0x6F, 0x77, 0x7C, 0x39, 0x5E, 0x79, 0x71};

unsigned long prevccr; // Previous value of the capture register
unsigned long counter;
unsigned char bcd[8];
unsigned char pps1;
char pps1_count; // last DP blink - On intervall

int main(void) {
    WDTCTL = WDTPW | WDTHOLD;	// Stop watchdog timer
	
    // Set 16MHz clock
	BCSCTL1 = CALBC1_16MHZ;
	DCOCTL = CALDCO_16MHZ;

	// initialize long counter
	counter = 0;

	// initialize GPIO (Optimalized)
	P1DIR = 0xFC;
	P1SEL = 0x03;
	P1SEL2 = 0;
	P2DIR = 0xFF;
	P2SEL = 0;
	P2SEL2 = 0;

	// Initialize Timer_A0
	// TACTL register
	// BIT 9-8 - 00 - Source: TACLK (TASSEL_0)
	// BIT 7-6 - 00 - Input divider: /1
	// BIT 5-4 - 10 - Mode: Continuous (MC_2)
	// BIT 1   -  1 - Enable TAIV interrupt (TAIE)
	TA0CTL = TASSEL_0 + MC_2 + TAIE;

	// Initialize compare block 0
	// CAP = 1 - Use capture mode
	// SCS = 0 - Asynchronous capture
	// CM1 - Capture on rising edge
	// CCIE - External input
	TA0CCTL0 = CM1 + CAP + CCIE;

    _EINT();	// Enable interrupts

    pps1 = 0;

    unsigned char i;
    // infinite loop
    while (1)
    {
    	for(i=0;i<8;i++)
    	{
    		P2OUT = 0x00;
    		P1OUT &= 0x03;
    		P1OUT |= (i << 3) + 4;	// Select Digit + Enable digit decoder output

    		// (bcd[i] & 0x10) ^ 0x10) - Enable segment outputs if the suppress zero bit off
    		// (i ? 0 : 1) - Enable segment outputs if it is the LS digit (this overwrite the suppress when the total output is 0)
    		// segments[bcd[i] & 0x0F] - Segment display
    		// (i ? 0 : pps1) - Last DP blink
    		P2OUT = ((((bcd[i] & 0x10) ^ 0x10) | (i ? 0 : 1)) ? segments[bcd[i] & 0x0F] : 0) + (i ? 0 : pps1);
    		__delay_cycles(2000);
    	}
    	if(pps1_count > 0)
    	{
    		pps1_count--;
    	}
    	else
    	{
    		pps1 = 0; // switch of the last DP
    	}
    }
}

// Timer capture interrupt
#pragma vector=TIMER0_A0_VECTOR
__interrupt void TACCR0_ISR(void)
{
	// The execution of the overflow interrupt blocked from here
	unsigned long freq;
	// store the actual value of the counter
	freq = counter;
	// clear master counter
	counter = 0;
	// Clear interrupt
	// The execution of the overflow interrupt allowed from here
	TA0CCTL0 &= ~CCIFG;

	// get the capture data
	unsigned long ccrvalue;
	ccrvalue = TA0CCR0;

	// process data
	//
	//          || <--------------------------------------------------- freq ------------------------------------------------> ||
	//  prevccr || 0x10000 - prevccr | 0x10000 overflow | 0x10000 overflow | ..... | 0x10000 overflow | 0x10000 overflow | ccr ||
	freq += ccrvalue - prevccr;
	// store current ccr for the next count
	prevccr = ccrvalue;
	// covert the binary frequency value to BCD for display
	LongToBCD(8,bcd,freq,0);
	pps1 = 0x80; // switch on the last DP
	pps1_count = 100; // Keep Last DP on for 100 display cicles
}

// Timer_A3 Interrupt Vector (TA0IV) handler
#pragma vector=TIMER0_A1_VECTOR
__interrupt void TA0IV_ISR(void)
{
	// after the counter overflow increment the counter with 65536
	if(TAIV == 10)
	{
		counter += 0x10000;
	}
}


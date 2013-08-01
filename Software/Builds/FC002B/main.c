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

int main(void) {
    WDTCTL = WDTPW | WDTHOLD;	// Stop watchdog timer
	
    // Set 16MHz clock
	// BCSCTL1 = CALBC1_16MHZ;
	// DCOCTL = CALDCO_16MHZ;

	// initialize long counter
	counter = 0;
	// initialize capture input
	P1DIR &= ~BIT1;
	P1SEL |= BIT1;
	P1SEL2 &= ~BIT1;

	// Initialize frequency input
	P1DIR &= ~BIT0;
	P1SEL |= BIT0;
	P1SEL2 &= ~BIT0;

	// Initialize Multiplex output
	P1SEL &= ~(BIT2 + BIT3 + BIT4 + BIT5 + BIT6);
	P1SEL2 &= ~(BIT2 + BIT3 + BIT4 + BIT5 + BIT6);
	P1DIR |= BIT2 + BIT3 + BIT4 + BIT5 + BIT6;

	// Initialize 7 Segment output
	P2SEL &= ~(BIT0 + BIT1 + BIT2 + BIT3 + BIT4 + BIT5  + BIT6);
	P2SEL2 &= ~(BIT0 + BIT1 + BIT2 + BIT3 + BIT4 + BIT5  + BIT6);
	P2DIR |= BIT0 + BIT1 + BIT2 + BIT3 + BIT4 + BIT5  + BIT6;

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
	// CCIS0 ?

	TA0CCTL0 = CM1 + CAP + CCIE;

    _EINT();	// Enable interrupts

    unsigned char i;
    // i2c_init(0x39, 12);
    // infinite loop
    while (1)
    {
    	for(i=0;i<8;i++)
    	{
    		P2OUT &= 0xC0;

    		P1OUT &= 0x83;
    		// __delay_cycles(1500);
    		P1OUT |= i << 3 | (((bcd[i] & 0x10) ^ 0x10) >> 2 );

    		P2OUT &= ~0x7F;
    		P2OUT |= segments[bcd[i] & 0x0F] & 0x7F;

    		__delay_cycles(500);
    	}
    }

	return 0;
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


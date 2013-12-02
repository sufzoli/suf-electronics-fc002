#include <msp430.h> 
#include "textconv.h"

/*
 * Frequency Counter project - 002A
 * main.c
 */

#define CTR_CLK	0x08
#define CTR_RST 0x20

const char segments[] = {0x3F,0x06, 0x5B, 0x4F, 0x66, 0x6D, 0x7D, 0x07, 0x7F, 0x6F, 0x77, 0x7C, 0x39, 0x5E, 0x79, 0x71};

unsigned long prevccr; // Previous value of the capture register
unsigned long counter;
unsigned char bcd[8];
unsigned char pps1;
unsigned int tb_count; // timebase count

unsigned char count_mode = 0; // Mode of the counter 0 - Frequency 1 - Interval
unsigned int gate_division = 256;

char pps1_count; // last DP blink - On interval


void set_countmode()
{
	if(count_mode == 0)
	{
		TA0CCTL1 |= CM1;
		TA0CCTL2 &= ~CM1;
		TA0CTL &= ~(TASSEL0 + TASSEL1);
	}
	else
	{
		TA0CCTL2 |= CM1;
		TA0CCTL1 &= ~CM1;
		TA0CTL |= TASSEL0;
	}
}

int main(void) {
    WDTCTL = WDTPW | WDTHOLD;	// Stop watchdog timer

    // Set 16MHz clock
	BCSCTL1 = CALBC1_16MHZ;
	DCOCTL = CALDCO_16MHZ;

	// initialize long counter
	counter = 0;

	// initialize GPIO (Optimized)
	P1DIR = 0xE8;
	P1SEL = 0x17;
	/*
	P1SEL = 0x03;	// If we in interval mode the P1.0 should be changed from TA0.TACLK to GPIN, P1.1 from TA0.0 to GPIN and P1.2 from GPIN to TA0.1
	// P1SEL = 0x04; // interval mode
	 */
	P1SEL2 = 0;


	P2DIR = 0x3F;
	P2SEL = 0xC0;
	P2SEL2 = 0;

	// Set ACLK to the external high frequency XTAL Oscillator (4.194 MHz)

	// Switch on the oscillator
	_bic_SR_register(OSCOFF);
	// XTAL HF Mode
	BCSCTL1 |= XTS;

	// Bit 5-4 = 11 Select Low frequency clock to External source
	BCSCTL3 &= ~(LFXT1S1 + LFXT1S0);
	BCSCTL3 |= LFXT1S1 + LFXT1S0;


	// Initialize Timer_A0
	// TACTL register
	// BIT 9-8 - 00 - Source: TACLK
	// BIT 7-6 - 00 - Input divider: /1
	// BIT 5-4 - 10 - Mode: Continuous (MC_2)
	// BIT 1   -  1 - Enable TAIV interrupt (TAIE)
	TA0CTL = MC_2 + TAIE;

	// Intervall mode
	// BIT 9-8 - 01 - Source ACLK
	// TA0CTL = TASSEL0 + MC_2 + TAIE;


	// Initialize compare block 0
	// CAP = 1 - Use capture mode
	// SCS = 0 - Asynchronous capture
	// CM1 - Capture on rising edge
	// CCIE - External input
	// TA0CCTL0 = CAP + CCIE;
	TA0CCTL1 = CM1 + CAP + CCIE;

	// Initialize compare block 1
	// CAP = 1 - Use capture mode
	// SCS = 0 - Asynchronous capture
	// CM1 - Capture on rising edge
	// CCIE - External input
	// TA0CCTL2 = CAP + CCIE;

	// set_countmode();

    _EINT();	// Enable interrupts

    pps1 = 0;

    unsigned char i;
    // infinite loop
    while (1)
    {
    	for(i=0;i<8;i++)
    	{
    		// switch of the digit
    		P1OUT &= 0x3F;
    		P2OUT &= 0xC0;

    		// digit counter
    		// if i=0 then counter reset if i>0 then counter step
    		P1OUT |= (i == 0) ? CTR_RST : CTR_CLK;

    		if((bcd[i] & 0x10) == 0 || i == 0)
    		{
    			P1OUT |= (segments[bcd[i] & 0x0F] | (i ? 0 : pps1)) & 0xC0;
    			P2OUT |= segments[bcd[i] & 0x0F] & 0x3F;
    		}


    		// (bcd[i] & 0x10) ^ 0x10) - Enable segment outputs if the suppress zero bit off
    		// (i ? 0 : 1) - Enable segment outputs if it is the LS digit (this overwrite the suppress when the total output is 0)
    		// segments[bcd[i] & 0x0F] - Segment display
    		// (i ? 0 : pps1) - Last DP blink
    		P2OUT = ((((bcd[i] & 0x10) ^ 0x10) | (i ? 0 : 1)) ? segments[bcd[i] & 0x0F] : 0) + (i ? 0 : pps1);


    		__delay_cycles(1000);
    		// counter signal falling edge (the normal CMOS 4017 can't handle the 16MHz clocks impulse width)
    		P1OUT &= ~(CTR_RST + CTR_CLK);
    		__delay_cycles(1000);
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
/*
// Timer capture interrupt
#pragma vector=TIMER0_A0_VECTOR
__interrupt void TACCR0_ISR(void)
{
	// The execution of the overflow interrupt blocked from here
	unsigned long freq;
	unsigned long ccrvalue;
	if(tb_count >= (gate_division - 1))
	{

		// store the actual value of the counter
		freq = counter;
		// clear master counter
		counter = 0;
		// Clear interrupt
		// The execution of the overflow interrupt allowed from here
		TA0CCTL0 &= ~CCIFG;
		TA0CCTL1 &= ~CCIFG;

		// get the capture data
		ccrvalue = count_mode ? TA0CCR1: TA0CCR0;

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
		tb_count=0;
	}
	else
	{
		// The execution of the overflow interrupt allowed from here
		TA0CCTL0 &= ~CCIFG;
		TA0CCTL1 &= ~CCIFG;
		tb_count++;
	}
}
*/
// Timer_A3 Interrupt Vector (TA0IV) handler
#pragma vector=TIMER0_A1_VECTOR
__interrupt void TA0IV_ISR(void)
{
	unsigned long freq;
	unsigned long ccrvalue;
	unsigned int taiv;

	taiv = TAIV;

	// after the counter overflow increment the counter with 65536
	if(taiv == TA0IV_TAIFG)
	{
		counter += 0x10000;
	}
	if(taiv == TA0IV_TACCR1 || taiv == TA0IV_TACCR2)
	{
		// The execution of the overflow interrupt blocked from here
		if(tb_count >= (gate_division - 1))
		{

			// store the actual value of the counter
			freq = counter;
			// clear master counter
			counter = 0;
			// Clear interrupt
			// The execution of the overflow interrupt allowed from here
			TA0CCTL1 &= ~CCIFG;
			// TA0IV &= ~TA0IV_TACCR1;
			// TA0CCTL2 &= ~CCIFG;

			// get the capture data
			ccrvalue = count_mode ? TA0CCR2: TA0CCR1;

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
			tb_count=0;
		}
		else
		{
			// The execution of the overflow interrupt allowed from here
			TA0CCTL1 &= ~CCIFG;
			// TA0IV &= ~TA0IV_TACCR1;
			// TA0CCTL2 &= ~CCIFG;
			tb_count++;
		}
	}
	// TA0IV &= ~TA0IV_TAIFG;
	// TA0CTL &= ~TAIFG;
}



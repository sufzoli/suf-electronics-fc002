#include <msp430.h> 
#include "textconv.h"

/*
 * Frequency Counter project - 002A
 * main.c
 */

#define CTR_CLK	0x08
#define CTR_RST 0x20

#define FLAG_COUNT_MODE 0x10
#define FLAG_DISP_MODE 0x08

const char segments[] = {0x3F, 0x06, 0x5B, 0x4F, 0x66, 0x6D, 0x7D, 0x07, 0x7F, 0x6F, 0x77, 0x7C, 0x39, 0x5E, 0x79, 0x71};
const unsigned long multiplication_limits[] = { 429496729, 42949672, 4294967, 429496, 42949, 4294, 429, 42, 4 };
const unsigned long powerten[] = { 0, 10, 100, 1000, 10000, 100000, 1000000, 10000000, 100000000 };

unsigned long prevccr; // Previous value of the capture register
unsigned long counter;
unsigned long count_result;
unsigned char result_ready;
unsigned char bcd[8];
unsigned char pps1;
unsigned char dp;
unsigned int tb_count; // timebase count

unsigned char switches;
unsigned char switch_read;

unsigned char count_mode = 0; // Mode of the counter 0 - Frequency 1 - Interval
unsigned char disp_mode = 0; // display mode of the Interval counter 0 - Frequency 1 - Time
unsigned int gate_division = 256;

char pps1_count; // last DP blink - On interval


/*
 * Port pin usage
 * 	P1.0 - Input
 * 	P1.1 - HF Clock
 *  P1.2 - LF Clock
 *  P1.3 - MPX CLK
 *  P1.4 - Input
 *  P1.5 - MPX RST
 *  P1.6 - Segment G
 *  P1.7 - Segment DP
 *  P2.0 - Segment A
 *  P2.1 - Segment B
 *  P2.2 - Segment C
 *  P2.3 - Segment D
 *  P2.4 - Segment E
 *  P2.5 - Segment F
 *  P2.6 - HF Clock (4.194304 MHz)
 *  P2.7 - NC
 */


void set_countmode()
{
	_DINT();
	prevccr = 0;
	counter = 0;
	count_result = 0;
	result_ready = 0;
	TA0CTL &= ~(TASSEL0 + TASSEL1);
	if(count_mode == 0)
	{
		// In frequency counting mode the Capture/Compare Block 2 not used (no interrupt)
		TA0CCTL2 &= ~CM1;
		// The counter input is set to external input
		// TACTL register
		// BIT 9-8 - 00 - Source: TACLK
		// TA0CTL &= ~(TASSEL0 + TASSEL1);
	}
	else
	{
		// In interval counting mode the Capture/Compare Block 2 will gate with external input signal
		TA0CCTL2 |= CM1;
		// The counter input is set to external input
		// TACTL register
		// BIT 9-8 - 01 - Source: ACLK
		TA0CTL |= TASSEL0;
	}
	_EINT();
}

int main(void) {
    WDTCTL = WDTPW | WDTHOLD;	// Stop watchdog timer

    // Set 16MHz clock
	BCSCTL1 = CALBC1_16MHZ;
	DCOCTL = CALDCO_16MHZ;

	// initialize long counter
	// counter = 0;

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


	// Initialize compare block 1
	// CAP = 1 - Use capture mode
	// SCS = 0 - Asynchronous capture
	// CM1 - Capture on rising edge
	// CCIE - External input
	// TA0CCTL0 = CAP + CCIE;
//	TA0CCTL1 = CAP + CCIE;
	TA0CCTL1 = CM1 + CAP + CCIE;	// The CM1 is added here because the 1 Hz tick always needed. It used for the value calculation

	// Initialize compare block 1
	// CAP = 1 - Use capture mode
	// SCS = 0 - Asynchronous capture
	// CCIE - External input
	TA0CCTL2 = CAP + CCIE;

	set_countmode();

    // _EINT();	// Enable interrupts

    pps1 = 0;

    unsigned char i;
    // infinite loop
    while (1)
    {
        for(i=0;i<9;i++)
    	{
    		// switch of the digit
    		P1OUT &= 0x3F;
    		P2OUT &= 0xC0;

    		// digit counter
    		// if i=0 then counter reset if i>0 then counter step
    		P1OUT |= (i == 0) ? CTR_RST : CTR_CLK;

    		if(i<8)
    		{
				if((bcd[i] & 0x10) == 0 || i == 0)
				{
					P1OUT |= (segments[bcd[i] & 0x0F] | (i ? 0 : pps1) | (dp & (1 << i) ? 0x80 : 0)) & 0xC0;
					P2OUT |= segments[bcd[i] & 0x0F] & 0x3F;
				}
    		}
        	if(i==8)
    		{
    			// Reconfigure ports
    			// Set Input
    			P1DIR &= 0x3F;
    			P2DIR &= 0xC0;
    			// Enable Resistors
    			P1REN = 0xC0;
    			P2REN = 0x3F;

    			// Pull-Down
    			P1OUT &= ~0xC0;
    			P2OUT &= ~0x3F;

    			// Read
    			__delay_cycles(100);
    			switch_read = (P1IN & 0xC0) | (P2IN & 0x3F);
    			if(switches != switch_read)
    			{
    				// Something changed
    				switches = switch_read;
    				count_mode = (switches & FLAG_COUNT_MODE) > 0 ? 1:0;
    				disp_mode = (switches & FLAG_DISP_MODE) > 0 ? 1:0;
    				set_countmode();
    			}

    			// switches = (~P1IN & 0xC0) | (~P2IN & 0x3F);
    			// Reconfigure ports
    	    	// Disable Resistors
    	    	P1REN = 0;
    	    	P2REN = 0;
    			// Set Output
    			P1DIR |= 0xC0;
    			P2DIR |= 0x3F;
    		}

/*
    		// (bcd[i] & 0x10) ^ 0x10) - Enable segment outputs if the suppress zero bit off
    		// (i ? 0 : 1) - Enable segment outputs if it is the LS digit (this overwrite the suppress when the total output is 0)
    		// segments[bcd[i] & 0x0F] - Segment display
    		// (i ? 0 : pps1) - Last DP blink
    		P2OUT = ((((bcd[i] & 0x10) ^ 0x10) | (i ? 0 : 1)) ? segments[bcd[i] & 0x0F] : 0) + (i ? 0 : pps1);
*/

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

// Timer_A3 Interrupt Vector (TA0IV) handler
#pragma vector=TIMER0_A1_VECTOR
__interrupt void TA0IV_ISR(void)
{
	// unsigned long count_result;
	unsigned long ccrvalue;
	unsigned int taiv;

	int i;

	taiv = TAIV;

	// Overflow handling
	// after the counter overflow increment the counter with 65536
	if(taiv == TA0IV_TAIFG)
	{
		counter += 0x10000;
	}

	// Capture/Compare Block 2 handling. Only active in interval mode
	if(taiv == TA0IV_TACCR2)
	{
		// store the actual value of the counter
		count_result = counter;
		// clear master counter
		counter = 0;
		// get the capture data
		ccrvalue = TA0CCR2;
		// process data
		//
		//          || <--------------------------------------------------- count_result ------------------------------------------------> ||
		//  prevccr || 0x10000 - prevccr | 0x10000 overflow | 0x10000 overflow | ..... | 0x10000 overflow | 0x10000 overflow | ccr ||
		count_result += ccrvalue - prevccr;
		// store current ccr for the next count
		prevccr = ccrvalue;
		// indicate that we have a valid count result
		result_ready = 1;
	}


	if(taiv == TA0IV_TACCR1)
	{
		if(tb_count >= (gate_division - 1))
		{
			if(count_mode == 0)
			{
				// store the actual value of the counter
				count_result = counter;
				// clear master counter
				counter = 0;
				// Clear interrupt
				// The execution of the overflow interrupt allowed from here
				// TA0CCTL1 &= ~CCIFG;
				// TA0IV &= ~TA0IV_TACCR1;
				// TA0CCTL2 &= ~CCIFG;

				// get the capture data
				ccrvalue = TA0CCR1;

				// process data
				//
				//          || <--------------------------------------------------- count_result ------------------------------------------------> ||
				//  prevccr || 0x10000 - prevccr | 0x10000 overflow | 0x10000 overflow | ..... | 0x10000 overflow | 0x10000 overflow | ccr ||
				count_result += ccrvalue - prevccr;
				// store current ccr for the next count
				prevccr = ccrvalue;
				result_ready = 1;
			}
			if(result_ready)
			{
				dp = 0; // switch of the decimal points
				if(count_mode == 1)
				{
					switch(disp_mode)
					{
						case 0: // frequency
							count_result = 4194304000 / count_result;
							dp = 8;
							break;
						case 1: // time
							for( i = 0; i < 9 && multiplication_limits[i] > count_result; i++);
							count_result *= powerten[i];
							count_result /= 4194304;
							break;
					}
					// handle the decimal point!!!
				}
				// covert the binary frequency value to BCD for display
				LongToBCD(8,bcd,count_result,0);
				pps1 = 0x80; // switch on the last DP
				pps1_count = 100; // Keep Last DP on for 100 display cycles
				result_ready = 0;
			}
			tb_count=0;
		}
		else
		{
			// The execution of the overflow interrupt allowed from here
			// TA0CCTL1 &= ~CCIFG;
			// TA0IV &= ~TA0IV_TACCR1;
			// TA0CCTL2 &= ~CCIFG;
			tb_count++;
		}
	}
}



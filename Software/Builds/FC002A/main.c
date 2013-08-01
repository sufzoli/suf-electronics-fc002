#include <msp430.h> 
#include "textconv.h"

/*
 * Frequency Counter project - 002A
 * main.c
 */

unsigned long prevccr; // Previous value of the capture register
unsigned long counter;
unsigned char bcd[8];

int main(void) {
    WDTCTL = WDTPW | WDTHOLD;	// Stop watchdog timer
	
    // Set 16MHz clock
	BCSCTL1 = CALBC1_16MHZ;
	DCOCTL = CALDCO_16MHZ;

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
//	P1SEL &= ~(BIT3 + BIT4 + BIT5);
//	P1SEL2 &= ~(BIT3 + BIT4 + BIT5);
//	P1DIR |= BIT3 + BIT4 + BIT5;

	P1SEL &= ~(BIT2 + BIT3 + BIT4 + BIT5);
	P1SEL2 &= ~(BIT2 + BIT3 + BIT4 + BIT5);
	P1DIR |= BIT2 + BIT3 + BIT4 + BIT5;


	// Initialize 7 Segment output
//	P2SEL &= ~(BIT0 + BIT1 + BIT2 + BIT3 + BIT4);
//	P2SEL2 &= ~(BIT0 + BIT1 + BIT2 + BIT3 + BIT4);
//	P2DIR |= BIT0 + BIT1 + BIT2 + BIT3 + BIT4;

	P2SEL &= ~(BIT0 + BIT1 + BIT2 + BIT3);
	P2SEL2 &= ~(BIT0 + BIT1 + BIT2 + BIT3);
	P2DIR |= BIT0 + BIT1 + BIT2 + BIT3;


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
//    		P2OUT &= 0xE0;
    		P2OUT &= 0xF0;
//    		P1OUT &= 0xC7;
    		P1OUT &= 0xC3;
    		// __delay_cycles(1500);
//    		P1OUT |= i << 3;
    		P1OUT |= i << 3 | (((bcd[i] & 0x10) ^ 0x10) >> 2 );
//    		P2OUT |= (bcd[i] ^ 0x10);
    		P2OUT |= (bcd[i] & 0x0F);
    		__delay_cycles(2000);
    	}
    }

	return 0;
}

// Watchdog timer interrupt
// Display the count result

// ezt át kell írni a capture interruptra
#pragma vector=TIMER0_A0_VECTOR
__interrupt void TACCR0_ISR(void)
{

	unsigned long freq;
	// Szinkronizálni kell az overflow interruptot a capture interrupthoz
	// Az overflow interrupt addig nem inkrementálhatja a counter értékét amíg a capture interrupt nem törli a counter-t
	freq = counter;
	// clear master counter
	counter = 0;
	// szinronizációs pont

	// Clear interrupt
	// Itt cserélni kell a capture interruptra (ennek kéne lennie a szinkronizációs adatnak)
	// CCIFG flag lesz a miénk
	TA0CCTL0 &= ~CCIFG;

	// get the capture data
	unsigned long ccrvalue;
	ccrvalue = TA0CCR0;
	// Itt törölni kell a COV bitet - ez jelzhi, hogy kiolvastuk a CCR tartalmát
	// ha az interrupt szinkron nem mûködik akkor ez használható szinkronnak, hiszen az interrupt trigger pillanatában kerül beállításra

	// process data
	//
	//          || <--------------------------------------------------- freq ------------------------------------------------> ||
	//  prevccr || 0x10000 - prevccr | 0x10000 overflow | 0x10000 overflow | ..... | 0x10000 overflow | 0x10000 overflow | ccr ||
	freq += ccrvalue - prevccr;
	// store current ccr for the next count
	prevccr = ccrvalue;
	LongToBCD(8,bcd,freq,0);
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


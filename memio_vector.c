
#include "memio_vector.h"

//----------------------------------------------------------------------------
// High priority interrupt routine
#pragma	tmpdata	ISRHtmpdata
#pragma interrupt InterruptHandlerHigh   nosave=section (".tmpdata")

void InterruptHandlerHigh(void)
{
	static uint8_t b_dummy;
	static union Timers timer;

	/* Z80 Control routines */

	/*
	 * MREQ
	 */
	if (INTCONbits.INT0IF) {
		INTCONbits.INT0IF = LOW;
		DLED2 = HIGH;
		Z.MREQ = HIGH;
		Z.RUN = TRUE;
		Z.maddr = PORTA;
		//		Z.maddr += ((uint16_t) PORTE << 8);
		Z.ISRAM = A10;
		Z.WR = ZRD;
		Z.M1 = ZM1;
		if (Z.M1 == LOW) {
			Z.paddr = Z.maddr;
		}
		if (Z.WR) { //write to pic
			TRISD = 0xff; // output to memory or io from Z80
		} else {
			TRISD = 0x00; // output from memory or io to Z80
		}
	}
	/*
	 * IORQ
	 */
	if (INTCON3bits.INT1IF) {
		INTCON3bits.INT1IF = LOW;
		Z.IORQ = HIGH;
		Z.RUN = TRUE;
		Z.maddr = PORTA;
		Z.WR = ZRD;
		if (Z.WR) { //write to pic
			TRISD = 0xff; // output to memory or io from Z80
		} else {
			TRISD = 0x00; // output from memory or io to Z80
		}
	}
	/*
	 * WR
	 */
	if (INTCON3bits.INT2IF) {
		INTCON3bits.INT2IF = LOW;
		TRISD = 0xff; // output to memory or io from Z80
	}

	if (Z.RUN) {
		Z.RUN = FALSE;
		if (Z.ISRAM) {
			if (Z.MREQ) {
				if (Z.WR) {
					z80_ram[Z.maddr & 0xff] = PORTD;
				} else {
					ZDATA = z80_ram[Z.maddr & 0xff];
					DLED7 = !DLED7;
				}
			}
		} else {
			if (Z.MREQ) {
				ZDATA = z80_rom[Z.paddr];
				if (Z.M1 == LOW) {
					SSP1BUF = Z.paddr;
					while (!SSP1STATbits.BF);
					b_dummy = SSP1BUF;
					SSP1BUF = ZDATA;
					while (!SSP1STATbits.BF);
					b_dummy = SSP1BUF;
				}
			}
		}
		WAIT = HIGH;
		Nop();
		Nop();
		Nop();
		Nop();
		Nop();
		Z.ISRAM = LOW;
		Z.MREQ = LOW;
		Z.IORQ = LOW;
		Z.WR = LOW;
		Z.M1 = LOW;
		WAIT = LOW;
	}

	if (INTCONbits.TMR0IF) { // check timer0 irq 1 second timer int handler
		INTCONbits.TMR0IF = LOW; //clear interrupt flag
		timer.lt = TIMEROFFSET; // Copy timer value into union
		TMR0H = timer.bt[HIGH]; // Write high byte to Timer0
		TMR0L = timer.bt[LOW]; // Write low byte to Timer0
		DLED7 = !DLED7;
	}

	if (PIR1bits.SSPIF) { // SPI port receiver
		PIR1bits.SSPIF = LOW;
		data_in2 = SSP1BUF; // read the buffer quickly
	}
	DLED2 = LOW;
}
#pragma	tmpdata

// Low priority interrupt routine
#pragma	tmpdata	ISRLtmpdata
#pragma interruptlow work_handler   nosave=section (".tmpdata")

/*
 *  This is the low priority ISR routine, the high ISR routine will be called during this code section
 */
void work_handler(void)
{
	if (PIR1bits.TMR1IF) {
		PIR1bits.TMR1IF = LOW; // clear TMR1 interrupt flag
		WriteTimer1(PDELAY);
		DLED2 = !DLED2;
	}
}
#pragma	tmpdata
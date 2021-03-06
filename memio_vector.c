
#include "memio_vector.h"
#include <spi.h>

/* High priority interrupt routine
 * handles the signal and data requests from the Z80
 */
#pragma	tmpdata	ISRHtmpdata
#pragma interrupt InterruptHandlerHigh   nosave=section (".tmpdata")

void InterruptHandlerHigh(void)
{
	static uint8_t b_dummy, b_data;
	static union Timers timer;

	/* Z80 Control routines */
	DLED2 = HIGH;
	E.int_count++;
	/*
	 * MREQ from Z80
	 */
	if (INTCONbits.INT0IF) {
		INTCONbits.INT0IF = LOW;
		E.mem++;
		Z.MREQ = HIGH;
		Z.RUN = TRUE;
		Z.maddr = ADDR_LOW;
		Z.maddr += ((uint16_t) (ADDR_HIGH & 0x03) << 8);
		Z.ISRAM = A10;
		Z.WR = ZRD;
		Z.M1 = !ZM1;
		Z.RFSH = !ZRFSH;
		Z.paddr = Z.maddr;
		if (Z.WR) { //write to pic
			TRISD = 0xff; // output to memory or io from Z80
			E.mem_wr++;
		} else {
			TRISD = 0x00; // output from memory or io to Z80
		}
	}
	/*
	 * IORQ from Z80
	 */
	if (INTCON3bits.INT1IF) {
		INTCON3bits.INT1IF = LOW;
		E.io++;
		Z.IORQ = HIGH;
		Z.RUN = TRUE;
		Z.maddr = ADDR_LOW;
		Z.WR = ZRD;
		if (Z.WR) { //write to pic
			TRISD = 0xff; // output to memory or io from Z80
			E.io_wr++;
		} else {
			TRISD = 0x00; // output from memory or io to Z80
		}
	}
	/*
	 * WR line went low from Z80
	 */
	if (INTCON3bits.INT2IF) {
		INTCON3bits.INT2IF = LOW;
		TRISD = 0xff; // output to memory or io from Z80
		Z.WR = TRUE;
	}

	/*
	 * use the Z80 state structure to talk to the chip
	 */
	if (Z.RUN) {
		Z.RUN = FALSE;

#ifdef SLOW_STEP
		/* slow the instruction cycle down to ~1 per second */
		INTCONbits.TMR0IF = LOW; //clear interrupt flag
		timer.lt = Z80_STEP; // Copy timer value into union so we don't call a function in the ISR
		TMR0H = timer.bt[HIGH]; // Write high byte to Timer0
		TMR0L = timer.bt[LOW]; // Write low byte to Timer0
		while (!INTCONbits.TMR0IF);
#endif	

		DLED7 = !DLED7;
		if (Z.ISRAM) { /* RAM access */
			if (Z.MREQ) {
				E.RAM++;
				if (Z.WR) {
					z80_ram[Z.maddr & 0xff] = ZDATA_I;
				} else {
					ZDATA_O = z80_ram[Z.maddr & 0xff];
				}
			}
		} else { /* ROM access */
			if (Z.MREQ) {
				E.ROM++;
				ZDATA_O = z80_rom[Z.paddr];
				if (DEBUG_MEM && Z.M1) { /* opcode */
					/*
					 * send the address and opcode via SPI for debug
					 */
					SSPCON1bits.SSPM = SPI_FOSC_4; // set clock to high speed
					EDCS = HIGH;
					SSP1BUF = Z.paddr;
					while (!SSP1STATbits.BF);
					b_dummy = SSP1BUF;
					SSP1BUF = z80_rom[Z.paddr];
					while (!SSP1STATbits.BF);
					b_dummy = SSP1BUF;
				}
			}
			if (Z.IORQ) {
				if (READ_IO_SPI) {
					/*
					 * send the port and data via SPI for debug
					 */
					SSPCON1bits.SSPM = SPI_FOSC_4; // set clock to high speed
					EDCS = HIGH;
					SSP1BUF = Z.maddr;
					while (!SSP1STATbits.BF);
					b_data = SSP1BUF;
				}
				/*
				 * Z80 IO port address space
				 */

				if (Z.WR) {
					switch (Z.maddr) {
					case PORT_BIT1:
						SPARE1 = ZDATA_I;
						break;
					case PORT_BIT2:
						SPARE2 = ZDATA_I;
						break;
					case SPI_DATA:
						SSPCON1bits.SSPM = SPI_FOSC_16; // set clock to slower speed
						EDRS = HIGH;
						EDCS = LOW;
						SSP1BUF = ZDATA_I;
						while (!SSP1STATbits.BF);
						b_dummy = SSP1BUF;
						break;
					case SPI_CMD:
						SSPCON1bits.SSPM = SPI_FOSC_16; // set clock to slower speed
						EDRS = LOW;
						EDCS = LOW;
						SSP1BUF = ZDATA_I;
						while (!SSP1STATbits.BF);
						b_dummy = SSP1BUF;
						break;
					default:
						break;
					}
				} else {
					switch (Z.maddr) {
					case RNGL:
						ZDATA_O = puf_bits.seed;
						break;
					case RNGH:
						ZDATA_O = puf_bits.seed >> 8;
						break;
					case M_PS0:
						E.mode = Z.maddr;
						E.dump = TRUE;
						ZDATA_O = b_data;
						break;
					case M_PS1:
						E.mode = Z.maddr;
						E.dump = TRUE;
						ZDATA_O = b_data;
						break;
					case M_PS2:
						E.mode = Z.maddr;
						E.dump = TRUE;
						ZDATA_O = b_data;
						break;
					case M_PS3:
						E.mode = Z.maddr;
						E.dump = TRUE;
						ZDATA_O = b_data;
						break;
					default:
						ZDATA_O = b_data;
						break;
					}
				}
			}
		}

		/*
		 * toggle out of wait so Z80 can continue running
		 * this needs to be in the window of one Z80 clk
		 * Too LONG or SHORT will cause misreads of data
		 * 10 to 13 nop opcodes seems to work
		 */

		WAIT = HIGH; /* clear the wait signal so the Z80 can process the data */
		Nop();
		Nop();
		Nop();
		Nop();
		Nop();
		Nop();
		Nop();
		Nop();
		Nop();
		Nop();
		//		Nop();
		//		Nop();
		WAIT = LOW; /* keep the wait signal on to slow down the Z80 until we can process its next signal */

		/*
		 * reset the state machine
		 */
		Z.ISRAM = LOW;
		Z.MREQ = LOW;
		Z.IORQ = LOW;
		Z.WR = LOW;
		Z.M1 = LOW;
		Z.RFSH = LOW;
		/*
		 * switch the PIC data port to input if needed
		 */
		if (ZRD) TRISD = 0xff; // output to memory or io from Z80
	}

	if (INTCONbits.TMR0IF) { // check timer0 irq 1 second timer int handler
		INTCONbits.TMR0IF = LOW; //clear interrupt flag
		timer.lt = TIMEROFFSET; // Copy timer value into union so we don't call a function in the ISR
		TMR0H = timer.bt[HIGH]; // Write high byte to Timer0
		TMR0L = timer.bt[LOW]; // Write low byte to Timer0
		DLED7 = !DLED7;
		E.runtime++;
	}

	if (PIR1bits.SSPIF) { // SPI port receiver
		PIR1bits.SSPIF = LOW;
		data_in2 = SSP1BUF; // read the buffer quickly
		E.spi_rd++;
	}
	DLED2 = LOW;
}
#pragma	tmpdata

// Low priority interrupt routine
#pragma	tmpdata	ISRLtmpdata
#pragma interruptlow work_handler   nosave=section (".tmpdata")

/*
 * This is the low priority ISR routine, the high ISR routine will be called during this code section
 * NOT USED
 */
void work_handler(void)
{
	if (PIR1bits.TMR1IF) {
		PIR1bits.TMR1IF = LOW; // clear TMR1 interrupt flag
		WriteTimer1(PDELAY);
	}
}
#pragma	tmpdata
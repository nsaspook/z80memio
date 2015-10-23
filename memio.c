

//#define DLED_DEBUG	// comment out to send real PORT data


/* 
 *  
 * 
 *
 * memory/io emulator for Z80
 *
 * nsaspook@nsaspook.com    2015
 */

#define P46K22

// PIC18F46K22 Configuration Bit Settings

// 'C' source line config statements

#include <p18f46k22.h>

// CONFIG1H
#pragma config FOSC = INTIO7    // Oscillator Selection bits (Internal oscillator block, CLKOUT function on OSC2)
#pragma config PLLCFG = ON      // 4X PLL Enable (Oscillator multiplied by 4)
#pragma config PRICLKEN = ON    // Primary clock enable bit (Primary clock is always enabled)
#pragma config FCMEN = OFF      // Fail-Safe Clock Monitor Enable bit (Fail-Safe Clock Monitor disabled)
#pragma config IESO = OFF       // Internal/External Oscillator Switchover bit (Oscillator Switchover mode disabled)

// CONFIG2L
#pragma config PWRTEN = OFF     // Power-up Timer Enable bit (Power up timer disabled)
#pragma config BOREN = SBORDIS  // Brown-out Reset Enable bits (Brown-out Reset enabled in hardware only (SBOREN is disabled))
#pragma config BORV = 190       // Brown Out Reset Voltage bits (VBOR set to 1.90 V nominal)

// CONFIG2H
#pragma config WDTEN = OFF       // Watchdog Timer Enable bits (WDT is always enabled. SWDTEN bit has no effect)
#pragma config WDTPS = 32768    // Watchdog Timer Postscale Select bits (1:32768)

// CONFIG3H
#pragma config CCP2MX = PORTC1  // CCP2 MUX bit (CCP2 input/output is multiplexed with RC1)
#pragma config PBADEN = OFF     // PORTB A/D Enable bit (PORTB<5:0> pins are configured as digital I/O on Reset)
#pragma config CCP3MX = PORTE0  // P3A/CCP3 Mux bit (P3A/CCP3 input/output is mulitplexed with RE0)
#pragma config HFOFST = ON      // HFINTOSC Fast Start-up (HFINTOSC output and ready status are not delayed by the oscillator stable status)
#pragma config T3CMX = PORTC0   // Timer3 Clock input mux bit (T3CKI is on RC0)
#pragma config P2BMX = PORTC0   // ECCP2 B output mux bit (P2B is on RC0)
#pragma config MCLRE = EXTMCLR  // MCLR Pin Enable bit (MCLR pin enabled, RE3 input pin disabled)

// CONFIG4L
#pragma config STVREN = ON      // Stack Full/Underflow Reset Enable bit (Stack full/underflow will cause Reset)
#pragma config LVP = ON         // Single-Supply ICSP Enable bit (Single-Supply ICSP enabled if MCLRE is also 1)
#pragma config XINST = OFF      // Extended Instruction Set Enable bit (Instruction set extension and Indexed Addressing mode disabled (Legacy mode))

// CONFIG5L
#pragma config CP0 = OFF        // Code Protection Block 0 (Block 0 (000800-001FFFh) not code-protected)
#pragma config CP1 = OFF        // Code Protection Block 1 (Block 1 (002000-003FFFh) not code-protected)

// CONFIG5H
#pragma config CPB = OFF        // Boot Block Code Protection bit (Boot block (000000-0007FFh) not code-protected)
#pragma config CPD = OFF        // Data EEPROM Code Protection bit (Data EEPROM not code-protected)

// CONFIG6L
#pragma config WRT0 = OFF       // Write Protection Block 0 (Block 0 (000800-001FFFh) not write-protected)
#pragma config WRT1 = OFF       // Write Protection Block 1 (Block 1 (002000-003FFFh) not write-protected)

// CONFIG6H
#pragma config WRTC = OFF       // Configuration Register Write Protection bit (Configuration registers (300000-3000FFh) not write-protected)
#pragma config WRTB = OFF       // Boot Block Write Protection bit (Boot Block (000000-0007FFh) not write-protected)
#pragma config WRTD = OFF       // Data EEPROM Write Protection bit (Data EEPROM not write-protected)

// CONFIG7L
#pragma config EBTR0 = OFF      // Table Read Protection Block 0 (Block 0 (000800-001FFFh) not protected from table reads executed in other blocks)
#pragma config EBTR1 = OFF      // Table Read Protection Block 1 (Block 1 (002000-003FFFh) not protected from table reads executed in other blocks)

// CONFIG7H
#pragma config EBTRB = OFF      // Boot Block Table Read Protection bit (Boot Block (000000-0007FFh) not protected from table reads executed in other blocks)


#include <spi.h>
#include <timers.h>
#include <adc.h>
#include <usart.h>
#include <delays.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <GenericTypeDefs.h>

/* ADC master command format
 *
 * hardware pins
 * SPI config
 * Pin 24 SDO
 * Pin 25 SDI
 * Pin 18 SCK clock
 */

#define	TIMEROFFSET	32000           // timer0 16bit counter value for 1 second to overflow
#define SLAVE_ACTIVE	5		// Activity counter level


/* DIO defines */
#define LOW		(unsigned char)0        // digital output state levels, sink
#define	HIGH            (unsigned char)1        // digital output state levels, source
#define	ON		LOW       		//
#define OFF		HIGH			//
#define	S_ON            LOW       		// low select/on for chip/led
#define S_OFF           HIGH			// high deselect/off chip/led
#define	R_ON            HIGH       		// control relay states, relay is on when output gate is high, uln2803,omron relays need the CPU at 5.5vdc to drive
#define R_OFF           LOW			// control relay states
#define R_ALL_OFF       0x00
#define R_ALL_ON	0xff
#define NO		LOW
#define YES		HIGH
#define IN		HIGH
#define OUT		LOW

#define	A10		LATCbits.LATC0 // RAM/ROM select LOW for ROM starting at address 0
#define WAIT		LATCbits.LATC1  // memory access delay
#define	ZDATA		LATD
#define ZRD		LATBbits.LATB4
#define ZM1		LATBbits.LATB5

#ifdef DLED_DEBUG
#ifdef P46K22
#define DLED0		LATDbits.LATD0
#define DLED1		LATDbits.LATD1
#define DLED2		LATDbits.LATD2
#define DLED3		LATDbits.LATD3
#define DLED4		LATDbits.LATD4
#define DLED5		LATDbits.LATD5
#define DLED6		LATAbits.LATA6
#define DLED7		LATAbits.LATA7
#endif
#else
#define DLED2		LATCbits.LATC2
#define DLED7		LATCbits.LATC7
#endif

#ifdef INTTYPES
#include <stdint.h>
#else
#define INTTYPES
/*unsigned types*/
typedef unsigned char uint8_t;
typedef unsigned int uint16_t;
typedef unsigned long uint32_t;
typedef unsigned long long uint64_t;
/*signed types*/
typedef signed char int8_t;
typedef signed int int16_t;
typedef signed long int32_t;
typedef signed long long int64_t;
#endif

struct z80_type { // internal state table
	uint8_t RUN : 1;
	uint8_t MREQ : 1;
	uint8_t IORQ : 1;
	uint8_t RD : 1;
	uint8_t WR : 1;
	uint8_t ISRAM : 1;
	uint8_t S1 : 1;
	uint8_t S2 : 1;
	uint16_t paddr;
	uint16_t maddr;
	uint8_t data;
};

#pragma udata gpr6
uint8_t z80_ram[256];
const rom int8_t z80_rom[1024] = {0xc7};

#pragma udata 
void work_handler(void);
#define	PDELAY		28000	// refresh for I/O

const rom int8_t *build_date = __DATE__, *build_time = __TIME__;
volatile uint8_t data_in2;
volatile uint8_t WDT_TO = FALSE, EEP_ER = FALSE;
volatile struct z80_type Z = {0};

void InterruptHandlerHigh(void);
//High priority interrupt vector
#pragma code InterruptVectorHigh = 0x08

void InterruptVectorHigh(void)
{
	_asm
		goto InterruptHandlerHigh //jump to interrupt routine
		_endasm
}
#pragma code

#pragma code work_interrupt = 0x18

void work_int(void)
{
	_asm goto work_handler _endasm // low pri interrupt
}
#pragma code

//----------------------------------------------------------------------------
// High priority interrupt routine
#pragma	tmpdata	ISRHtmpdata
#pragma interrupt InterruptHandlerHigh   nosave=section (".tmpdata")

void InterruptHandlerHigh(void)
{
	static uint8_t b_dummy;

	if (INTCONbits.RBIF) { // PORT B int handler
		INTCONbits.RBIF = LOW;
		b_dummy = PORTB;
	}

	/* Z80 Control routines */

	/*
	 * MREQ
	 */
	if (INTCONbits.INT0IF) {
		INTCONbits.INT0IF = LOW;
		Z.MREQ = HIGH;
		Z.RUN = TRUE;
		//		Z.maddr = 0;
		Z.maddr = PORTA;
		//		Z.maddr += ((uint16_t) PORTE << 8);
		//		Z.ISRAM = A10;
		Z.ISRAM = FALSE;
		Z.WR = ZRD;
		if (ZM1) {
			Z.paddr = Z.maddr;
		}
		if (!Z.WR) {
			TRISD = 0x00; // output from memory or io to Z80
		} else {
			TRISD = 0xff; // output to memory or io from Z80
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
		if (!Z.WR) {
			TRISD = 0x00; // output from memory or io to Z80
		} else {
			TRISD = 0xff; // output to memory or io from Z80
		}
	}
	/*
	 * WR
	 */
	if (INTCON3bits.INT2IF) {
		INTCON3bits.INT2IF = LOW;
	}

	if (Z.RUN) {
		DLED2 = HIGH;
		Z.RUN = FALSE;
		if (Z.ISRAM) {
			if (Z.MREQ) {
				if (Z.WR) {
					z80_ram[Z.maddr & 0xff] = ZDATA;
				} else {
					ZDATA = z80_ram[Z.maddr & 0xff];
				}
				DLED7 = !DLED7;
			}
		} else {
			if (Z.MREQ) {
				//ZDATA = z80_rom[Z.maddr];
				ZDATA = z80_rom[0];
				SSP1BUF = Z.maddr;
				while (!SSP1STATbits.BF);
				b_dummy = SSP1BUF;
				SSP1BUF = ZDATA;
				while (!SSP1STATbits.BF);
				b_dummy = SSP1BUF;
			}
		}
		WAIT = HIGH;
		Nop();
		Nop();
		Nop();
		Nop();
		Nop();
		Nop();
		Nop();
		Nop();
		Z.MREQ = LOW;
		Z.IORQ = LOW;
		Z.WR = LOW;
		DLED2 = LOW;
		WAIT = LOW;
	}

	if (INTCONbits.TMR0IF) { // check timer0 irq 1 second timer int handler
		INTCONbits.TMR0IF = LOW; //clear interrupt flag
		WriteTimer0(TIMEROFFSET);
		DLED7 = !DLED7;
	}

	if (PIR1bits.ADIF) { // ADC conversion complete flag
		PIR1bits.ADIF = LOW;
	}

	if (PIR1bits.SSPIF) { // SPI port receiver
		PIR1bits.SSPIF = LOW;
		data_in2 = SSPBUF; // read the buffer quickly
	}
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

void wdtdelay(unsigned long delay, unsigned char clearit)
{
	static uint32_t dcount;
	for (dcount = 0; dcount <= delay; dcount++) { // delay a bit
		Nop();
		if (clearit) ClrWdt(); // reset the WDT timer
	};
}

void config_pic_io(void)
{
	int z;

	if (RCONbits.TO == (uint8_t) LOW) WDT_TO = TRUE;
	if (EECON1bits.WRERR && (EECON1bits.EEPGD == (uint8_t) LOW)) EEP_ER = TRUE;
	/*
	 * default operation mode
	 */

	OSCCON = 0x70; // internal osc 16mhz, CONFIG OPTION 4XPLL for 64MHZ
	OSCTUNE = 0b01000000; // 4x pll
	SLRCON = 0x00; // all slew rates to max
	/*
	 * all analog off
	 */
	ANSELA = 0;
	ANSELB = 0;
	ANSELC = 0;
	ANSELD = 0;
	ANSELE = 0;
	ANSELA = 0;
	TRISA = 0xff; // all inputs
	TRISB = 0xff; // all inputs
	TRISC = 0b00000001;
	TRISD = 0x00; // all outputs
	//	PADCFG1bits.RDPU = HIGH;
	TRISE = 0xff;
	//	PADCFG1bits.REPU = HIGH;

	LATC = 0xff;

	/* SPI pins setup */
	TRISCbits.TRISC3 = OUT; // SCK 
	TRISCbits.TRISC4 = IN; // SDI
	TRISCbits.TRISC5 = OUT; // SDO

	/* setup the SPI interface for Master */
	OpenSPI1(SPI_FOSC_4, MODE_00, SMPEND);
	SSPCON1 |= SPI_FOSC_4; // set clock to high speed
	PIE1bits.SSP1IE = HIGH; // enable interrupts

	/* System activity timer */
	OpenTimer0(TIMER_INT_ON & T0_16BIT & T0_SOURCE_INT & T0_PS_1_256);
	WriteTimer0(TIMEROFFSET); //      start timer0 at ~1 second ticks

	/* event timer */
	OpenTimer1(T1_SOURCE_FOSC_4 & T1_16BIT_RW & T1_PS_1_8 & T1_OSC1EN_OFF & T1_SYNC_EXT_OFF, 0);
	IPR1bits.TMR1IP = LOW; // set timer2 low pri interrupt
	WriteTimer1(PDELAY);

	/* clear SPI module possible flag */
	PIR1bits.SSPIF = LOW;


	/*
	 * PORTB config
	 */
	INTCON2bits.RBPU = LOW; // turn on weak pullups
	INTCONbits.RBIE = LOW; // disable PORTB interrupts
	INTCONbits.INT0IE = HIGH; //
	INTCON2bits.INTEDG0 = LOW; // falling edge
	INTCONbits.INT0IF = LOW; // clean possible flag

	INTCON3bits.INT1IE = HIGH; //
	INTCON2bits.INTEDG1 = LOW; // falling edge
	INTCON3bits.INT1IF = LOW; // clean possible flag

	INTCON3bits.INT2IE = HIGH; //
	INTCON2bits.INTEDG2 = LOW; // falling edge
	INTCON3bits.INT2IF = LOW; // clean possible flag
	IOCB = 0x00;
	z = PORTB; // dummy read 

	/* Enable interrupt priority */
	RCONbits.IPEN = HIGH;
	/* Enable priority interrupts */
	INTCONbits.GIEH = HIGH;
	INTCONbits.GIEL = LOW;

	/* clear any SSP error bits */
	SSPCON1bits.WCOL = SSPCON1bits.SSPOV = LOW;
}

void check_config(void)
{
	INTCONbits.GIEH = LOW;
	INTCONbits.GIEL = LOW;
	config_pic_io();
}

void main(void) /* SPI Master/Slave loopback */
{
	check_config();

	while (1) { // just loop
		wdtdelay(600000, TRUE);
	}
}

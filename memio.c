/* 
 * 1024 byte ROM and 256 byte RAM
 * currently limited to 256 bytes for testing
 *
 * memory/io emulator for Z80, slow but works
 *
 * nsaspook    2015
 * 
 * edge connector
 * D0--D7 1--8, A0--A10 9--19
 * A +5, Z GND, F WAIT_, P MREQ_, K IORQ_, N RD_, M WR_, J M!_, L RFSH_
 * S BUSREQ_, C INT_, D NMI
 * 
 * debug port pins
 * 1 wait, 2 gmreq, 3 dled2, 4 mreq, 5 rfsh, 6 sck, 7 sdo, 8 m1
 */

#define P46K22

// PIC18F46K22 Configuration Bit Settings

// 'C' source line config statements

#include <p18f46k22.h>

// CONFIG1H
#pragma config FOSC = INTIO67    // Oscillator Selection bits (Internal oscillator block)
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
#pragma config XINST = ON      // Extended Instruction Set Enable bit (Instruction set extension and Indexed Addressing mode)

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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <spi.h>
#include <timers.h>
#include <EEP.h>
#include <GenericTypeDefs.h>
#include "memio_vector.h"

/* Z80 SPI debug port
 *
 * hardware pins
 * SPI1 config
 * Pin 24 SDO
 * Pin 25 SDI
 * Pin 18 SCK clock
 */

#pragma udata gpr6
/* Z80 memory variables */
uint8_t volatile z80_ram[256];

/* short test program of opcodes, memory and io using the SDCC -mz80 compiler

// 
// Z80 to PIC18f46k22 MEMIO demo program 
//
// setup the IO ports to send data
__sfr __at 0x80 IoPort_data; // send display data text
__sfr __at 0x81 IoPort_cmd; // send commands to program the display
__sfr __at 0x01 IoPort_bit; // set or clear bit RC6 on the PIC
__sfr __at 0x10 IoPort_rngl; // random number generator bits, low byte
__sfr __at 0x11 IoPort_rngh; // random number generator bits, high byte
__sfr __at 0x00 IoPort_dummy; // SPI dummy read

// MEMIO SRAM memory space 
volatile __at(0x0400) unsigned char ram_space;

// start the program
static const char __at 0x00b0 z80_text0[] = " Shall we play a game? ";
static const char __at 0x00d0 z80_text[] = " All work and no play makes Jack a dull boy ";

// display functions
void config_display(void);
void putc(unsigned char);

volatile static unsigned char __at(0x0400) b;
volatile static unsigned char __at(0x0401) d;
char z;

void main(void)
{
	unsigned char c, i = 0, *ram_ptr = &ram_space;
	unsigned int a;

	// setup ram stack pointer
	__asm;
	ld sp,#0x04ff;
		__endasm;

	config_display();

	// send the text string to the data port
	while (1) {
		b = IoPort_rngl;
		for (i = 0; i < 80; i++) {
			if (b < 128) {
				c = z80_text[i];
			} else {
				c = z80_text0[i];
			}
			if (!c) break;
			putc(c);
			a = 2000;
			if (IoPort_data < 200) a = 0;
			while (a) a--;
		}
	};
}

// turn off the cursor

void config_display(void)
{
	IoPort_cmd = 0b00001100;
}

// send one character

void putc(unsigned char c)
{
	IoPort_data = c;
}

 
 * rom data created using the make_c_header script in the XideSDCC directory
 * ADD 'rom' to the included variable
 */

// File lo converted using cpct_bin2c
const rom unsigned char z80_rom[512] = {
	0x31, 0xff, 0x04, 0xcd, 0x7e, 0x00, 0x21, 0x00, 0x00, 0x22, 0x02, 0x04, 0xdb, 0x10, 0x32, 0x00,
	0x04, 0x0e, 0x00, 0x3a, 0x00, 0x04, 0xd6, 0x80, 0x30, 0x09, 0x21, 0xd0, 0x01, 0x06, 0x00, 0x09,
	0x7e, 0x18, 0x07, 0x21, 0xb0, 0x01, 0x06, 0x00, 0x09, 0x7e, 0xb7, 0x28, 0x21, 0xc5, 0xf5, 0x33,
	0xcd, 0x83, 0x00, 0x33, 0xc1, 0x11, 0xd0, 0x07, 0xdb, 0x80, 0xd6, 0xc8, 0x30, 0x03, 0x11, 0x00,
	0x00, 0x7a, 0xb3, 0x28, 0x03, 0x1b, 0x18, 0xf9, 0x0c, 0x79, 0xd6, 0x50, 0x38, 0xc5, 0xed, 0x5b,
	0x02, 0x04, 0xfd, 0x21, 0x02, 0x04, 0xfd, 0x34, 0x00, 0x20, 0x07, 0xfd, 0x21, 0x02, 0x04, 0xfd,
	0x34, 0x01, 0x7b, 0xd6, 0xc8, 0x20, 0xa5, 0xb2, 0x20, 0xa2, 0x21, 0x00, 0x00, 0x22, 0x02, 0x04,
	0xdb, 0xf0, 0x21, 0x88, 0x13, 0xe5, 0xcd, 0x8b, 0x00, 0xf1, 0xdb, 0xf1, 0x18, 0x8e, 0x3e, 0x0c,
	0xd3, 0x81, 0xc9, 0x21, 0x02, 0x00, 0x39, 0x7e, 0xd3, 0x80, 0xc9, 0xc1, 0xd1, 0xd5, 0xc5, 0x7a,
	0xb3, 0xc8, 0x1b, 0x18, 0xfa, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0x20, 0x53, 0x68, 0x61, 0x6c, 0x6c, 0x20, 0x77, 0x65, 0x20, 0x70, 0x6c, 0x61, 0x79, 0x20, 0x61,
	0x20, 0x67, 0x61, 0x6d, 0x65, 0x3f, 0x20, 0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0x20, 0x41, 0x6c, 0x6c, 0x20, 0x77, 0x6f, 0x72, 0x6b, 0x20, 0x61, 0x6e, 0x64, 0x20, 0x6e, 0x6f,
	0x20, 0x70, 0x6c, 0x61, 0x79, 0x20, 0x6d, 0x61, 0x6b, 0x65, 0x73, 0x20, 0x4a, 0x61, 0x63, 0x6b,
	0x20, 0x61, 0x20, 0x64, 0x75, 0x6c, 0x6c, 0x20, 0x62, 0x6f, 0x79, 0x20, 0x00, 0xff, 0xff, 0xff
};

/******************************************************************************
 *  CRC 16 Look-up Table.                                            *
 ******************************************************************************/

const rom unsigned int far crc_table[0x100] = {
	0x0000, 0x1021, 0x2042, 0x3063, 0x4084, 0x50A5, 0x60C6, 0x70E7,
	0x8108, 0x9129, 0xA14A, 0xB16B, 0xC18C, 0xD1AD, 0xE1CE, 0xF1EF,
	0x1231, 0x0210, 0x3273, 0x2252, 0x52B5, 0x4294, 0x72F7, 0x62D6,
	0x9339, 0x8318, 0xB37B, 0xA35A, 0xD3BD, 0xC39C, 0xF3FF, 0xE3DE,
	0x2462, 0x3443, 0x0420, 0x1401, 0x64E6, 0x74C7, 0x44A4, 0x5485,
	0xA56A, 0xB54B, 0x8528, 0x9509, 0xE5EE, 0xF5CF, 0xC5AC, 0xD58D,
	0x3653, 0x2672, 0x1611, 0x0630, 0x76D7, 0x66F6, 0x5695, 0x46B4,
	0xB75B, 0xA77A, 0x9719, 0x8738, 0xF7DF, 0xE7FE, 0xD79D, 0xC7BC,
	0x48C4, 0x58E5, 0x6886, 0x78A7, 0x0840, 0x1861, 0x2802, 0x3823,
	0xC9CC, 0xD9ED, 0xE98E, 0xF9AF, 0x8948, 0x9969, 0xA90A, 0xB92B,
	0x5AF5, 0x4AD4, 0x7AB7, 0x6A96, 0x1A71, 0x0A50, 0x3A33, 0x2A12,
	0xDBFD, 0xCBDC, 0xFBBF, 0xEB9E, 0x9B79, 0x8B58, 0xBB3B, 0xAB1A,
	0x6CA6, 0x7C87, 0x4CE4, 0x5CC5, 0x2C22, 0x3C03, 0x0C60, 0x1C41,
	0xEDAE, 0xFD8F, 0xCDEC, 0xDDCD, 0xAD2A, 0xBD0B, 0x8D68, 0x9D49,
	0x7E97, 0x6EB6, 0x5ED5, 0x4EF4, 0x3E13, 0x2E32, 0x1E51, 0x0E70,
	0xFF9F, 0xEFBE, 0xDFDD, 0xCFFC, 0xBF1B, 0xAF3A, 0x9F59, 0x8F78,
	0x9188, 0x81A9, 0xB1CA, 0xA1EB, 0xD10C, 0xC12D, 0xF14E, 0xE16F,
	0x1080, 0x00A1, 0x30C2, 0x20E3, 0x5004, 0x4025, 0x7046, 0x6067,
	0x83B9, 0x9398, 0xA3FB, 0xB3DA, 0xC33D, 0xD31C, 0xE37F, 0xF35E,
	0x02B1, 0x1290, 0x22F3, 0x32D2, 0x4235, 0x5214, 0x6277, 0x7256,
	0xB5EA, 0xA5CB, 0x95A8, 0x8589, 0xF56E, 0xE54F, 0xD52C, 0xC50D,
	0x34E2, 0x24C3, 0x14A0, 0x0481, 0x7466, 0x6447, 0x5424, 0x4405,
	0xA7DB, 0xB7FA, 0x8799, 0x97B8, 0xE75F, 0xF77E, 0xC71D, 0xD73C,
	0x26D3, 0x36F2, 0x0691, 0x16B0, 0x6657, 0x7676, 0x4615, 0x5634,
	0xD94C, 0xC96D, 0xF90E, 0xE92F, 0x99C8, 0x89E9, 0xB98A, 0xA9AB,
	0x5844, 0x4865, 0x7806, 0x6827, 0x18C0, 0x08E1, 0x3882, 0x28A3,
	0xCB7D, 0xDB5C, 0xEB3F, 0xFB1E, 0x8BF9, 0x9BD8, 0xABBB, 0xBB9A,
	0x4A75, 0x5A54, 0x6A37, 0x7A16, 0x0AF1, 0x1AD0, 0x2AB3, 0x3A92,
	0xFD2E, 0xED0F, 0xDD6C, 0xCD4D, 0xBDAA, 0xAD8B, 0x9DE8, 0x8DC9,
	0x7C26, 0x6C07, 0x5C64, 0x4C45, 0x3CA2, 0x2C83, 0x1CE0, 0x0CC1,
	0xEF1F, 0xFF3E, 0xCF5D, 0xDF7C, 0xAF9B, 0xBFBA, 0x8FD9, 0x9FF8,
	0x6E17, 0x7E36, 0x4E55, 0x5E74, 0x2E93, 0x3EB2, 0x0ED1, 0x1EF0
};


#pragma udata 
void work_handler(void);

const rom int8_t *build_date = __DATE__, *build_time = __TIME__;
volatile uint8_t data_in2;
volatile uint8_t WDT_TO = FALSE, EEP_ER = FALSE;
volatile struct z80_type Z = {0};
volatile struct spi_link_type spi_link;
volatile struct memio_type E = {0};
volatile struct ringBufS_t ring_buf5, ring_buf6;

/*
 * HWRND
 */

volatile struct btype puf_bits = {0x1957, 0, 0, 0, 0, 0, 0, 0x5719};
#pragma udata rnddata
far unsigned char sram_key[PUF_SIZE], sram_key_masked[PUF_SIZE];
#pragma udata

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

void int_enable(void)
{
	/* Enable priority interrupts */
	INTCONbits.GIEH = HIGH;
	INTCONbits.GIEL = LOW; // not used	
}

/*
 * setup the basic hardware config
 */
void config_pic_io(void)
{
	int z;

	if (RCONbits.TO == (uint8_t) LOW) WDT_TO = TRUE;
	if (EECON1bits.WRERR && (EECON1bits.EEPGD == (uint8_t) LOW)) EEP_ER = TRUE;
	/*
	 * default operation mode
	 */

	OSCCON = 0x70; // internal osc 16mhz, CONFIG OPTION 4XPLL for 64MHZ
	OSCTUNE = 0b01011111; // 4x pll
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
	TRISA = 0xff; // all inputs for Z80 address
	TRISB = 0b11110111; // all inputs but RB3 for Z80 logic inputs
	TRISC = 0x01; // C0 for A10 Z80 address
	TRISD = 0xff; // all inputs for the ZDATA default of WR_
	TRISE = 0xff; // all inputs for Z80 address

	LATC = 0x00;
	LATD = 0x00;

	/* SPI pins setup */
	TRISCbits.TRISC3 = OUT; // SCK 
	TRISCbits.TRISC4 = IN; // SDI
	TRISCbits.TRISC5 = OUT; // SDO

	/* setup the SPI interface for Master */
	OpenSPI1(SPI_FOSC_64, MODE_00, SMPEND);
	SSPCON1bits.SSPM = SPI_FOSC_64; // set clock to low speed
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

	INTCON3bits.INT2IE = LOW; //
	INTCON2bits.INTEDG2 = LOW; // falling edge
	INTCON3bits.INT2IF = LOW; // clean possible flag
	IOCB = 0x00;
	z = PORTB; // dummy read 

	spi_link.tx1b = &ring_buf5;
	spi_link.tx1a = &ring_buf6;
	ringBufS_init(spi_link.tx1b);
	ringBufS_init(spi_link.tx1a);

	/* Enable interrupt priority */
	RCONbits.IPEN = HIGH;

	/* clear any SSP error bits */
	SSPCON1bits.WCOL = SSPCON1bits.SSPOV = LOW;
}

void check_config(void)
{
	INTCONbits.GIEH = LOW;
	INTCONbits.GIEL = LOW;
	config_pic_io();
}

/*
 * value of 1 for 3.6us, 10 for 15us, 100 = 126us, 1000 for 1256us
 */
void wdtdelay(uint32_t delay)
{
	static uint32_t dcount;
	for (dcount = 0; dcount <= delay; dcount++) { // delay a bit
		Nop();
		ClrWdt(); // reset the WDT timer
	};
}

void ed_chr(uint8_t chr)
{
	uint8_t b_dummy;

	SSPCON1bits.SSPM = SPI_FOSC_16; // set clock to slow speed
	EDRS = HIGH;
	EDCS = LOW;
	SSP1BUF = chr;
	while (!SSP1STATbits.BF);
	b_dummy = SSP1BUF;
	wdtdelay(2500); // short display delay
	EDCS = HIGH;
}

void ed_cmd(uint8_t inst)
{
	uint8_t b_dummy;

	SSPCON1bits.SSPM = SPI_FOSC_16; // set clock to slow speed
	EDRS = LOW;
	EDCS = LOW;
	SSP1BUF = inst;
	while (!SSP1STATbits.BF);
	b_dummy = SSP1BUF;
	wdtdelay(3500); // > 4ms command delay
	EDRS = HIGH;
	EDCS = HIGH;
}

/*
 * STDOUT user handler function
 */
int _user_putc(char c)
{
	ed_chr(c);
}

/*
 * Init the EA DOGM163 in 8bit serial mode
 */
void init_ed_display(void)
{
	wdtdelay(350000); // > 400ms power up delay

	INTCONbits.GIEH = LOW;
	stdout = _H_USER; // use our STDOUT handler
	SSPCON1bits.SSPM = SPI_FOSC_64; // set clock to slow speed
	ed_cmd(0x39);
	ed_cmd(0x1d);
	ed_cmd(0x50);
	ed_cmd(0x6c);
	ed_cmd(0x76); // contrast last 4 bits
	ed_cmd(0x38);
	ed_cmd(0x0f);
	ed_cmd(0x01);
	ed_cmd(0x02);
	ed_cmd(0x06);
	PIR1bits.SSPIF = LOW;
}

/*
 * 	Count bits 	
 */

unsigned char bit_count(unsigned char input)
{
	unsigned char counter = 0, i;

	for (i = 0; i < 8; i++) {
		if ((input & (1 << i)) != 0) counter++;
	}
	return(counter);
}

/******************************************************************************
 *  Make CRC 16 Calculation Function.                                     *
 ******************************************************************************/

unsigned int Make_Crc16(unsigned char *data, unsigned int length)
{
	unsigned int accum, len;

	/* Pre-conditioning */
	accum = 0xFFFF;
	len = length;

	while (len--) {
		accum = ((accum & 0x00FF) << 8) ^
			crc_table[((accum >> 8) ^ *data++) & 0x00FF];
	}
	return(accum);
}

/* get random data for sram powerup bits */
// Preserve EEPROM memory checked in programmer to keep daa past device programm changes !!!!!!!!!
// look at random SRAM data for PRNG seeds and ID key
// uses a udata section of memeory 2xPUF_SIZE and eeprom from
// 0 to 3xPUF_SIZE+sizeof(puf_bits) to store key, diff data, masked data and seed data in EEPROM
// if clear_mode is TRUE the stored key diff data will be zeroed and will return 0
// if kmode is TRUE the seed will be generated from the masked contents of sram
// if kmode is FALSE the seed will be generated from the raw sram contents
// kmode should be stable after about 10 or more power cycles

unsigned int puf_sram(unsigned char kmode, unsigned char clear_mode)
{
	unsigned int e;
	static unsigned int seeda = 0;
	unsigned char entr_s, entr_r, entr_d, *seedp;

	puf_bits.sram_raw = 0;
	puf_bits.eeprom_save = 0;
	puf_bits.eeprom_mask = 0;
	puf_bits.eeprom_key = 0;
	puf_bits.ok = FALSE;
	for (e = 0; e < PUF_SIZE; e++) {
		seeda = seeda + sram_key[e];
		entr_s = sram_key[e]; // SRAM power up bits in memory cells, some should be random if not set to zero by C runtime
		puf_bits.sram_raw += bit_count(entr_s);
		Busy_eep();
		entr_r = Read_b_eep(e); // read eeprom seed data
		puf_bits.eeprom_save += bit_count(entr_r);
		Busy_eep();
		entr_d = Read_b_eep(e + PUF_SIZE); // read unstuck bits data
		entr_d = entr_d | (entr_s^entr_r); // XOR to look for diff bits
		puf_bits.eeprom_mask += bit_count(entr_d);
		sram_key_masked[e] = sram_key[e] & (~entr_d); // store unstuck bits
		puf_bits.eeprom_key += bit_count(sram_key_masked[e]);

		if (!clear_mode) { // write to EEPROM sram memory history so we can find changing sram power up bits
			Busy_eep();
			Write_b_eep(e, entr_s); // write eeprom new key data
			Busy_eep();
			Write_b_eep(e + PUF_SIZE, entr_d); // write eeprom key unstuck bits data
			Busy_eep();
			Write_b_eep(e + PUF_SIZE + PUF_SIZE, sram_key_masked[e]); // write eeprom key stuck bits data
		} else { // cmode  will zero key data history
			Busy_eep();
			Write_b_eep(e, entr_s); // write eeprom key data
			Busy_eep();
			Write_b_eep(e + PUF_SIZE, 0); // write zeros to  eeprom key unstuck bits data
			Busy_eep();
			Write_b_eep(e + PUF_SIZE + PUF_SIZE, 0); // write zeros to  eeprom key masked data
		}
	}
	if (puf_bits.sram_raw > PUF_SIZE) puf_bits.ok = TRUE;

	puf_bits.seed = Make_Crc16(sram_key, PUF_SIZE); // make seed from crc16 of sram
	if (kmode) puf_bits.seed = Make_Crc16(sram_key_masked, PUF_SIZE); // make seed from crc16 of sram masked with unstuck bits
	if (clear_mode) puf_bits.seed = 0; // cmode return 0

	seedp = (unsigned char*) &puf_bits; // set seedp pointer to address of data
	for (e = 0; e<sizeof(puf_bits); e++) {
		Busy_eep();
		Write_b_eep(e + PUF_SIZE + PUF_SIZE + PUF_SIZE, (unsigned char) (*seedp++)); // Write seed byte to EEPROM
	}
	return(puf_bits.seed);
}

void memio_dump(void)
{
	INTCONbits.GIEH = LOW;
	ed_cmd(1); //clear display
	if (E.mode == M_PS0) printf("PS0 RO%lu, RA%lu, IO%lu, RT%lu ", E.ROM, E.RAM, E.io, E.runtime);
	if (E.mode == M_PS1) printf("PS1 INT%lu, IOW%lu, MEW%lu, SR%lu ", E.int_count, E.io_wr, E.mem_wr, E.spi_rd);
	wdtdelay(4500000);
	E.dump = FALSE;
	INTCONbits.GIEH = HIGH;
}

void main(void) /* CPU Master/Slave loopback */
{
	check_config();
	init_ed_display();
	srand(puf_sram(FALSE, FALSE)); // rng init
	int_enable();

	E.mode = M_PS0;
	memio_dump();
	while (TRUE) { // just loop, while in emulation very little cpu time is spent here so rand() is very slow
		if (!ZRFSH) { // refresh is active
			WAIT = HIGH;
		} else {
			WAIT = LOW;
		}
		puf_bits.seed = rand();
		if (E.dump) memio_dump();
	}
}

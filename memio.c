

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


#include <spi.h>
#include <timers.h>
#include <GenericTypeDefs.h>
#include "memio_vector.h"

/* ADC master command format
 *
 * hardware pins
 * SPI config
 * Pin 24 SDO
 * Pin 25 SDI
 * Pin 18 SCK clock
 */

#pragma udata gpr6
uint8_t volatile z80_ram[256];
const rom int8_t z80_rom[1024] = {0x00, 0xf3, 0xc7};

#pragma udata 
void work_handler(void);

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
		Nop();
		Nop();
	}
}

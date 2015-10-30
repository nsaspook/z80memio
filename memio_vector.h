/* 
 * File:   memio_vector.h
 * Author: root
 *
 * Created on October 23, 2015, 2:59 PM
 * 
 */

#ifndef MEMIO_VECTOR_H
#define	MEMIO_VECTOR_H

#ifdef	__cplusplus
extern "C" {
#endif

#include <p18f46k22.h>
#include <GenericTypeDefs.h>
#include <timers.h>
#include "ringbufs.h"
	
#define DEBUG_MEM	FALSE
#define DEBUG_IO	TRUE
//#define SLOW_STEP

#define	TIMEROFFSET	32000			// timer0 16bit counter value for ~1 second to overflow
#define Z80_STEP	56000
#define	PDELAY		28000			// refresh for I/O	

	/* general DIO defines */
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

	/* Z80 pins */
#define	A10		PORTCbits.RC0 // RAM/ROM select LOW for ROM starting at address 0
#define WAIT		LATCbits.LATC1  // memory access delay
#define	ZDATA_O		LATD
#define ZDATA_I		PORTD
#define ADDR_LOW	PORTA
#define ADDR_HIGH	PORTE
#define ZRD		PORTBbits.RB4
#define ZM1		PORTBbits.RB5
#define ZRFSH 		PORTBbits.RB6
#define ZWR		PORTBbits.RB7

	/* debug pins */
#define DLED2		LATCbits.LATC2
#define DLED7		LATCbits.LATC7
	
	/* display chip select pin */
#define EDCS		LATCbits.LATC7	
#define EDRS		LATBbits.LATB3

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

	/*
	 * the logic is true HIGH for STATE
	 */
	struct z80_type { // internal state table
		uint16_t RUN : 1;
		uint16_t MREQ : 1;
		uint16_t IORQ : 1;
		uint16_t RD : 1;
		uint16_t WR : 1;
		uint16_t M1 : 1;
		uint16_t RFSH : 1;
		uint16_t ISRAM : 1;
		uint16_t S1 : 1;
		uint16_t S2 : 1;
		uint16_t paddr;
		uint16_t maddr;
		uint8_t data;
	};
	
	struct spi_link_type { // internal state table
	uint8_t SPI_LCD : 1;
	uint8_t SPI_AUX : 1;
	uint8_t LCD_TIMER : 1;
	uint8_t LCD_DATA : 1;
	uint16_t delay;
	uint8_t config;
	struct ringBufS_t *tx1b, *tx1a;
	int32_t int_count;
};

	extern volatile struct z80_type Z;
	extern volatile uint8_t z80_ram[256];
	extern const rom unsigned char z80_rom[256];
	extern volatile uint8_t data_in2;

#ifdef	__cplusplus
}
#endif

#endif	/* MEMIO_VECTOR_H */


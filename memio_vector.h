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
#define READ_IO_SPI	TRUE			// This should be TRUE in normal operation
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

	/* spare */
#define SPARE1		LATCbits.LATC6
#define SPARE2		LATEbits.LATE2

	/* IO address */
#define SPI_DATA	0x80
#define SPI_CMD		0x81
#define PORT_BIT1	0x01
#define PORT_BIT2	0x02
#define RNGL		0x10
#define RNGH		0x11

#define M_PS0		0xf0	// show Z80 stats on the LCD display
#define M_PS1		0xf1
#define M_PS2		0xf2
#define M_PS3		0xf3	

	/*
	 * 
	 */
#define RNG_UPDATE	128
#define	RNG_SPEED_F	12
#define	RNG_SPEED_S	10
#define PUF_SIZE 128

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

	struct memio_type { // internal Z80 stats
		uint32_t int_count, mem, io, mem_wr, io_wr, ROM, RAM, spi_rd, runtime;
		uint8_t mode, dump;
	};

	struct btype {
		unsigned int sheader, seed, sram_raw, eeprom_save, eeprom_mask, eeprom_key, ok, eheader;
	};

	extern volatile struct z80_type Z;
	extern volatile uint8_t z80_ram[256];
	extern const rom unsigned char z80_rom[512];
	extern volatile uint8_t data_in2;
	extern volatile struct btype puf_bits;
	extern volatile struct memio_type E;

#ifdef	__cplusplus
}
#endif

#endif	/* MEMIO_VECTOR_H */


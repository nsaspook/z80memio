/* 
 * File:   eadog.h
 * Author: root
 *
 * Created on July 27, 2015, 2:05 PM
 */

/* Parts taken from:
 * Modified for C18 and PIC18F45K80 with SPI using ring buffers with interrupts
 * 
 *            file: EA-DOGM_MIO.c
 *         version: 2.03
 *     description: Multi I/O driver for EA DOGM displays
 *                : Uses 8Bit, SPI HW or SPI SW (bitbang)
 *     written by : Michael Bradley (mbradley@mculabs.com)
 *   contributions: Imaginos (CCS forum), Emil Nad (8Bit testing)
 *                  jgschmidt (CCS forum)
 */

#ifndef EADOG_H
#define	EADOG_H

#ifdef	__cplusplus
extern "C" {
#endif

#include <spi.h>
#include "memio_vector.h"

#define LCD_CMD_MASK	0x01
#define LCD_CMD_SET		0x100
#define LCD_CLEAR_HOME	0x04

#define EADOGM_CMD_CLR		1
#define EADOGM_CMD_CURSOR_ON     0b00001111
#define EADOGM_CMD_CURSOR_OFF    0b00001100
#define EADOGM_CMD_DISPLAY_ON    0b00001100
#define EADOGM_CMD_DISPLAY_OFF   0b00001000
#define EADOGM_CMD_DDRAM_ADDR    0b10000000
#define EADOGM_CMD_CGRAM_ADDR    0b01000000
#define EADOGM_CMD_SELECT_R0     0b00011000
#define EADOGM_CMD_SELECT_R1     0b00010000
#define EADOGM_CMD_SET_TABLE2    0b00101010
#define EADOGM_COLSPAN		16

	extern volatile struct spi_link_type spi_link;
	extern void wdtdelay(uint32_t);

	void init_display(void);
	void send_lcd_data(uint8_t);
	void send_lcd_cmd(uint8_t);
	void start_lcd(void);
	void wait_lcd(void);

	void eaDogM_WriteChr(char);
	void eaDogM_WriteCommand(uint8_t);
	void eaDogM_SetPos(uint8_t, uint8_t);
	void eaDogM_ClearRow(uint8_t);
	void eaDogM_WriteString(char *);
	void eaDogM_WriteStringAtPos(uint8_t, uint8_t, char *);
	void eaDogM_WriteIntAtPos(uint8_t, uint8_t, uint8_t);
	void eaDogM_WriteByteToCGRAM(uint8_t, uint8_t);

#ifdef	__cplusplus
}
#endif

#endif	/* EADOG_H */


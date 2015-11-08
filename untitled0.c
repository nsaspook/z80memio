// 
// Z80 to PIC18f46k22 MEMIO demo program 
//
// setup the IO ports to send data
__sfr __at 0x80 IoPort_data; // send display data text
__sfr __at 0x81 IoPort_cmd; // send commands to program the display
__sfr __at 0x01 IoPort_bit1; // set or clear bit RC6 on the PIC
__sfr __at 0x02 IoPort_bit2; // set or clear bit RE2 on the PIC
__sfr __at 0x10 IoPort_rngl; // random number generator bits, low byte
__sfr __at 0x11 IoPort_rngh; // random number generator bits, high byte
__sfr __at 0x00 IoPort_dummy; // SPI dummy read
__sfr __at 0xf0 IoPort_ps0; // MEMIO process status display on LCD
__sfr __at 0xf1 IoPort_ps1;
__sfr __at 0xf2 IoPort_ps2;
__sfr __at 0xf3 IoPort_ps3;

// MEMIO SRAM memory space 
volatile __at(0x0400) unsigned char ram_space;

// start the program
static const char __at 0x01b0 z80_text0[] = " Shall we play a game? ";
static const char __at 0x01d0 z80_text[] = " All work and no play makes Jack a dull boy ";

// display functions
void config_display(void);
void putc(unsigned char);
void ddelay(unsigned int);

static unsigned char __at(0x0400) b;
static unsigned char __at(0x0401) d;
static unsigned int __at(0x0402) z;

void main(void)
{
	unsigned char c, i = 0, *ram_ptr = &ram_space;
	unsigned int a;

	// setup ram stack pointer
	__asm;
	ld sp,#0x04ff;
		__endasm;

	config_display();
	z = 0;

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
		if (z++ == 200) {
			z = 0;
			a = IoPort_ps0;
			ddelay(5000);
			a = IoPort_ps1;
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

// delay function

void ddelay(unsigned int dd)
{
	while (dd) dd--;
}
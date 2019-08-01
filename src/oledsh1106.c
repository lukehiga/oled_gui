#include "oledsh1106.h"


static void _print_msg(const char * string, size_t str_length, uint8_t page, uint8_t column);

#ifdef INCLUDE_RESCALING_FUNCTIONS
static void _print_doublesized_char(char target, uint8_t page, uint8_t column);
static void _print_triplesized_char(char target, uint8_t page, uint8_t column);
static void _print_quadsized_char(char target, uint8_t page, uint8_t column);
#endif

void flush_oledpage_bounded(uint8_t page, uint8_t start, uint8_t end);
uint8_t nonempty_pages = 0xFF; /* to avoid flushing everything */




void spi_init(void)
{
	/* Atmega328 SPI init */
	DDRB |= 1<<PB5; /* SCK as output */
	DDRB |= 1<<PB3; /* MOSI as output */
	DDRB |= 1<<PB2; /* Slave/chip Select as output */
	DDRB |= 1<<PB1; /* Data/Command as output */
	DDRB |= 1<<PB0;  /* Reset pin as output */


	PORTB |= (1<<PB2); /* HIGH IDLE */
	
	SPCR |= (1<<SPE)/*|(1<<DORD)*/|(1<<MSTR);/*|(1<<SPR1)*///|(1<<SPR0);
	SPCR |= (1<<CPHA);
	SPCR |= (1<<CPOL);
	SPSR |= (1<<SPI2X);
	/* Min serial clock cycle on the SH1106 is 500ns -> max freq = 2Mhz */
	
	//SPSR |= 0x01;
}

static inline void spi_send(uint8_t data)
{
	/* Blocking mode SPI transmition*/
	
	PORTB &= ~(1<<PB2); /* CS LOW */ 

	SPDR = data;
	while(!(SPSR & (1<<SPIF)));  /* Wait for transmition to complete*/

	PORTB |= (1<<PB2); /* CS HIGH */ 

	return;
	
}

void send_command(uint8_t command)
{
	PORTB &= ~(1<<PB1); /* Set Data/command pin low*/
	spi_send(command);
	return;
}

static inline void send_data(uint8_t data)
{
	PORTB |= (1<<PB1); /* Set D/C pin HIGH */
	spi_send(data);
	return;	
}





void sh1106_128x64_init()
{
	uint8_t i;
	PORTB &= ~(1<<PB0);  /* RESET low */
	_delay_us(20); /* Higher than 10us as specified on datasheet */
	PORTB |= (1<<PB0); /* Reset HIGH  */
	
	/* When RESET input falls to LOW the circuit enters into default settings state (POR = Power-On-Reset) (pp18 datasheet)
	 * Display OFF
	 * 132x64 mode
	 * Normal column and row address mapping (SEG0 to 0x00 and COM0 to 0x00)
	 * Clear shift register
	 * Display start line at 0x00
	 * Normal scanning direction of the common outputs -???-
	 * Contrast to default value 128 (range 1-256)
	 * Internal DC-DC selected 
	 */

	uint8_t init_commands[] =
	{
		0xAE, /* Display OFF */
		0xA6, /* Normal, not inverted (e.g. 0b1000 turns to 0b0111) */
		0xA1, /* Segment remap (ADC) to 1. (Flips the screen) */
		0xDA, /* Hardware Pads set */
		0x12, /* Standard hardware pads */
		0xC8, /* Common output scan direction (Mirrors screen vertically)*/
		0xA8, /* Multiplex ratio set */
		0xFF, /* Standard (POR) multiplex ratio value */
		
		0xD5, /* Oscillator freq and ratio set */
		0xF0, /* 0x50 Standard osc. and div. */
		
		0xD3, /* Offset set (vertical offset)*/
		0x00, /* No offset */
		
		0xD9,  /* Discharge and pre-charge period */
		0x11, /* 1 DCLK */
		
		0xDB, /* VCOM set  */
		0x35, /* Standard (POR) VCOM value */
		
		0x81, /* Contrast set */
		0xC8, /* Contrast value 200 (range 1-255) */
		
		0x30, /* Set pump voltage to ---  */
		
		0xAD, /*DC-DC Control set*/
		0x8B, /* DC-DC ON*/
		
		0xA4, /* Output ram to display */
		0x40,  /* Display start line at 0 */
		0xAF, /* Display ON */
		0xB0, /* Set page address to 0 */
		0x00, /* set low column address */
		0x10 /* set high column address */
	};
	
	
	for (i = 0 ; i < sizeof(init_commands) ; i++)
	{
		send_command(init_commands[i]);
		_delay_us(INIT_CALLS_DELAY_US);
	}

}


void fastdraw_xy_point(uint8_t x, uint8_t y)
{
	/* Drawing options still in development */
	
	/* Out of screen shouldn't send anything */
	if(	 x > OLED_SCREEN_MAX_X_RES || y > OLED_SCREEN_MAX_Y_RES )  
		return;

	send_command(0xB7 - y/8); 
	send_command(0x00 + (x & 0x0F) ); /* Set low column address */
	send_command(0x10 + (x>>4)  ); /* Set high column address */
	send_data(0x80 >> y%8); 
	
	return;
}

void draw_xy_point(int16_t x, int16_t y)
{
	/* Drawing options still in development */
	/* 
	 * Due to the lack of a screen buffer, multiple points on the same column of a given page needs special care.
	 * Can set multiple points on the same page and column ONLY IN CONSECUTIVE CALLS (useful for vertical lines).
	 */
	
	static uint8_t prev_page, prev_column;
	uint8_t actual_page;
	
	static uint8_t current_page_bits;

	/* Out of screen shouldn't send anything */
	if(	x < 0 || x > OLED_SCREEN_MAX_X_RES || 
		y <0 || y > OLED_SCREEN_MAX_Y_RES )  
		return;

	actual_page = 0xB7 - y/8; /* Set page address command */
	if(actual_page == prev_page && x == prev_column)
		current_page_bits |= (0x80 >> y%8); /* add new point to old bit page */
	else
		current_page_bits = (0x80 >> y%8);
	
	
	send_command(actual_page); 
	send_command(0x00 + (x & 0x0F) ); /* Set low column address */
	send_command(0x10 + (x>>4)  ); /* Set high column address */
	send_data(current_page_bits); 
	
	prev_page = actual_page;
	prev_column = x;
	
	
	return;
}

void draw_line(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1)
{
	/* Drawing options still in development */
	
	uint8_t i;
	int16_t delta_x = x1-x0;
	int16_t delta_y = y1-y0;
	double slope = 0;
	
	if(delta_x != 0)
		slope = (double)delta_y/delta_x;
	
	
	for (i = x0 ; i <= x1 ; i++)
	{
		draw_xy_point(i, y0 + delta_y/delta_x*i);
		//delta_x = delta_y - 156555551*slope; /* to do */
	}
	/*................*/

}



void flush_oled_ram(void)
{
	/* Fills oled RAM with zeros on non-empty pages */
	
	/* To do: 8 Pages are hardcoded */
	uint8_t i, j;
	
	for(j=0; j< OLED_MAX_PAGES; j++)	
	{
		if(nonempty_pages & (1<<j))
		{
			set_oled_pagecolumn(j,0);
			for(i=0; i< OLED_SCREEN_MAX_X_RES ; i++)
				send_data(0);
		}		
	}
	nonempty_pages = 0;
}

void flush_oledpage_ram(uint8_t page)
{
	/* Fills page with zeros. If speed is a constrain, use flush_oledpage_bounded() 
	when possible. */
	size_t i;
	set_oled_pagecolumn(page,0);
	
	for(i=0; i< OLED_SCREEN_MAX_X_RES ; i++)
	{
		PORTB |= (1<<PB1);
		PORTB &= ~(1<<PB2); /* CS LOW */
		SPDR = 0;
		while(!(SPSR & (1<<SPIF)));  /* Wait for transmition to complete*/
		PORTB |= (1<<PB2); /* CS HIGH */
	}
}

void set_oled_pagecolumn(uint8_t page, uint8_t column)
{
	/* Sets page and column before writing. Calling this function keeps record
	of the written page for a smarter flushing. */
	
	if(page >= OLED_MAX_PAGES || column >= OLED_SCREEN_MAX_X_RES)
		return;
		
	send_command(0xB0 + page); 
	send_command(/*0x00 +*/ ((column+SH1106_OFFSET) & 0x0F) ); /* Set low column address */
	send_command( 0x10 + ((column+SH1106_OFFSET) >> 4)  ); /* Set high column address */	
	
	/* To do: 8 Pages are hardcoded */
	nonempty_pages |= (1<<page); 
	
	return;
}


static void _print_msg(const char * string, size_t str_length, uint8_t page, uint8_t column) 
{

	/* To be called from regular size print_msg(), which truncates string if necessary
	(str_length is useful in the case of not printing the full string) */
	uint8_t i;
	size_t letter_index; 
	char current_char;
	
	if(string == 0 /*NULL*/)
		return;
		
	if(page > OLED_MAX_PAGES)
		return;
		
	set_oled_pagecolumn(page, column);	
	
	for (letter_index = 0 ; letter_index < str_length ; letter_index++)
	{
		
		current_char = string[letter_index] - ASCII_OFFSET;
		for (i = 0; i < FONT_PIXEL_WIDTH ; i++)
		{
			if(current_char >= 0 && current_char < MAX_CHARS)
			{	
				/* Beware: uses AVR's pgm_read_byte */
				
				//send_data(pgm_read_byte(&font_bytes[(current_char-ASCII_OFFSET)*FONT_PIXEL_WIDTH+i]));
				
				/* inlined atmega328 send_data(pgm_read_byte(...)) since its the main function
				for printing data on the oled screen. */
				PORTB |= (1<<PB1);  /* Needs a falling edge? */
				PORTB &= ~(1<<PB2);
				SPDR = pgm_read_byte(&font_bytes[(current_char)*FONT_PIXEL_WIDTH+i]);
				while(!(SPSR & (1<<SPIF)));  /* Wait for transmition to complete*/
				PORTB |= (1<<PB2);
			}
		}
		
		send_data(0x00); /* Space between chars */
		
	}
}

void print_msg_centered(const char * string, oledfontsize_t size, size_t page)
{
	/* Calculates starting column so text is centered and calls print_msg(). */
	size_t str_length;
	uint8_t fontscaling = 0;
	
	if(string == 0 /*NULL*/) 
		return;
	
	str_length = strlen(string);
	
	#ifndef INCLUDE_RESCALING_FUNCTIONS
	size = REGULAR_SIZE_FONT; /* Force everything to be regular size*/
	#endif
	
	
	switch(size)
	{
		case REGULAR_SIZE_FONT: fontscaling = 1; break;
		case DOUBLE_SIZE_FONT: fontscaling = 2; break;
		case TRIPLE_SIZE_FONT: fontscaling = 3; break;
		case CUADRUPLE_SIZE_FONT: fontscaling = 4; break;
		default: fontscaling = 1; break;
	}
	
	if (str_length*fontscaling*(FONT_PIXEL_WIDTH+1) >= OLED_SCREEN_MAX_X_RES)
	{
		print_msg(string, size, page, 0); 
		return;
	}
	
	print_msg(string, size, page, (OLED_SCREEN_MAX_X_RES - str_length*fontscaling*(FONT_PIXEL_WIDTH+1) )/2);
	return;
}

void flush_oledpage_bounded(uint8_t page, uint8_t start_column, uint8_t end_column)
{
	/* Flushes ram between bounds start_column and end_column at specified page. */
	size_t i;
	
	if(page >= OLED_MAX_PAGES || (start_column >= end_column) || end_column > OLED_SCREEN_MAX_X_RES)
		return;

	set_oled_pagecolumn(page,start_column);
	
	for(i=0; i< (end_column - start_column) ; i++)
	{
		/* Trying to flush as fast as possible so no send_data(0) used. */
		/* send_data(0); */
		PORTB |= (1<<PB1);
		PORTB &= ~(1<<PB2); 
		SPDR = 0;
		while(!(SPSR & (1<<SPIF)));  
		PORTB |= (1<<PB2); 
	}
	
}



void print_msg(const char * string, oledfontsize_t size, uint8_t page, uint8_t column )
{
	/*Prints message in specified page and column. Truncates string if it doesnt fit the screen.
	
	To avoid using high amounts of flash memory, double, triple and cuadruple size are generated 
	in run time by rescaling the simple size fonts. Defining INCLUDE_RESCALING_FUNCTIONS should
	enable these functions (aprox 1 - 2kB of Flash memory).
	*/
	
	/*To do: continue string in next line? */

	size_t available_letterspace;
	size_t str_len;
	size_t index_lim = 0;
	
	#ifdef INCLUDE_RESCALING_FUNCTIONS
		size_t i; 
		size_t j;
	#endif
	
	
	if(string == 0 /*NULL*/)
		return;
	
	str_len = strlen(string);
	
	#ifndef INCLUDE_RESCALING_FUNCTIONS
	size = REGULAR_SIZE_FONT; /* Force everything to be regular size*/
	#endif
	
	
	switch(size)
	{	
		case REGULAR_SIZE_FONT:
			/* Check length and size to truncate string */
			available_letterspace = (OLED_SCREEN_MAX_X_RES - column )/(FONT_PIXEL_WIDTH+1);
			index_lim = (available_letterspace < str_len) ? available_letterspace : str_len;
			
			//flush_oledpage_ram(page); 
			
			flush_oledpage_bounded(page, 0, column);
			flush_oledpage_bounded(page, column+(index_lim)*(FONT_PIXEL_WIDTH+1), OLED_SCREEN_MAX_X_RES);
			_print_msg(string,index_lim,page,column); /* No rescaling needed */
			break;
			
#ifdef INCLUDE_RESCALING_FUNCTIONS
		case DOUBLE_SIZE_FONT:
			available_letterspace = (OLED_SCREEN_MAX_X_RES - column )/(2*(FONT_PIXEL_WIDTH+1));
			index_lim = (available_letterspace < str_len) ? available_letterspace : str_len;
			
			/* Flush non used space */
			for(j=0; j<2; j++)
			{
				/* Flushes space between left limit and text start*/
				flush_oledpage_bounded(page+j, 0, column);
				/* Flushes space text end and right screen limit */
				flush_oledpage_bounded(page+j, column+(index_lim)*2*(FONT_PIXEL_WIDTH+1), OLED_SCREEN_MAX_X_RES);
			}
				
			for (i = 0; i< index_lim ; i++)
				_print_doublesized_char(string[i], page, column + i*2*(FONT_PIXEL_WIDTH+1) );
			break;
		
		case TRIPLE_SIZE_FONT:
			available_letterspace = (OLED_SCREEN_MAX_X_RES - column )/(3*(FONT_PIXEL_WIDTH+1));
			index_lim = (available_letterspace < str_len) ? available_letterspace : str_len;

			for(j=0; j<3; j++)
			{
				flush_oledpage_bounded(page+j, 0, column);
				flush_oledpage_bounded(page+j, column+(index_lim)*3*(FONT_PIXEL_WIDTH+1), OLED_SCREEN_MAX_X_RES);
			}
				
			for (i = 0; i<index_lim ; i++)
				_print_triplesized_char(string[i], page, column + i*3*(FONT_PIXEL_WIDTH+1) );
					
			break;
		
		case CUADRUPLE_SIZE_FONT:
			available_letterspace = (OLED_SCREEN_MAX_X_RES - column )/(4*(FONT_PIXEL_WIDTH+1));
			index_lim = (available_letterspace < str_len) ? available_letterspace : str_len;
			
			for(j=0; j<4; j++)
			{
				flush_oledpage_bounded(page+j, 0, column);
				flush_oledpage_bounded(page+j, column+(index_lim)*4*(FONT_PIXEL_WIDTH+1), OLED_SCREEN_MAX_X_RES);
			}
			
			for (i = 0; i<index_lim ; i++)
				_print_quadsized_char(string[i], page, column + i*4*(FONT_PIXEL_WIDTH+1) );
				
			break;
#endif
		default: break;
	}
	
}


void print_char(const char targetchar, uint8_t page, uint8_t column)
{
	/* Prints char in specified position without flushing */
	uint8_t i;

	if(page > OLED_MAX_PAGES)
		return;
	
	set_oled_pagecolumn(page, column);
	
	
	if(targetchar >= ASCII_OFFSET && targetchar < MAX_CHARS + ASCII_OFFSET)
	{
		for (i = 0; i < FONT_PIXEL_WIDTH ; i++)
			send_data(pgm_read_byte(&font_bytes[(targetchar-ASCII_OFFSET)*FONT_PIXEL_WIDTH+i]));
	}
	
}

#ifdef INCLUDE_RESCALING_FUNCTIONS

/* To do 
 Improve: there are repeated send_data() which is an inline function. */

static void _print_doublesized_char(char target, uint8_t page, uint8_t column)
{
	size_t i, j;
	uint8_t buffer;
	
		set_oled_pagecolumn(page,column);
		for(j = 0; j < FONT_PIXEL_WIDTH; j++)
		{
			for (i = 0 ; i < 4 ; i++) /* for low nibble */
			{
				if( pgm_read_byte( &font_bytes[(target-ASCII_OFFSET)*FONT_PIXEL_WIDTH+j]) & (1<<i))
				{
					buffer |= (0x03 << 2*i);
				}
			}
			send_data(buffer);
			send_data(buffer);
			buffer = 0;
		}
		/* When using flush_oledpage_bounded(), since not the whole page is erased
		there might be some trash between letters when changing string length so
		empty bytes are sent. */
		send_data(0); /* Space between letters */
		send_data(0);
		
		
		set_oled_pagecolumn(page+1,column);
		for(j = 0; j < FONT_PIXEL_WIDTH; j++)
		{
			for (i = 0 ; i < 4 ; i++) /* for high nibble */
			{
				if( (pgm_read_byte( &font_bytes[(target-ASCII_OFFSET)*FONT_PIXEL_WIDTH+j])>>4) & (1<<i))
				{
					buffer |= (0x03 << 2*i);
				}
			}
			send_data(buffer);
			send_data(buffer);
			buffer = 0;
		}
		send_data(0);
		send_data(0);
}

static void _print_triplesized_char(char target, uint8_t page, uint8_t column)
{
	size_t i, j;
	uint8_t buffer;

	
	set_oled_pagecolumn(page,column);
	for(j = 0; j < FONT_PIXEL_WIDTH; j++)
	{
		for (i = 0 ; i < 3 ; i++) 
		{
			if( pgm_read_byte( &font_bytes[(target-ASCII_OFFSET)*FONT_PIXEL_WIDTH+j]) & (1<<i))
			{
				 /* Due to the 3 bits from original byte mapping to 8 bit new bytes
				  * i.e. 
				  *      00000001 should become  00000111,
				  *		 00000010 becomes		 00111000
				  * but  00000100 has to be		 11000000
				  *
				  * This yields to some distortion but is still readable.
				  */
				buffer |= ( ((i<2) ? 0x07 : 0x03) << 3*i); 
			}
		}
		send_data(buffer);
		send_data(buffer);
		send_data(buffer);
		buffer = 0;
	}
	send_data(0);
	send_data(0);
	send_data(0);
	
	set_oled_pagecolumn(page+1,column);
	for(j = 0; j < FONT_PIXEL_WIDTH; j++)
	{
		for (i = 0 ; i < 3 ; i++) 
		{
			if( ( pgm_read_byte( &font_bytes[(target-ASCII_OFFSET)*FONT_PIXEL_WIDTH+j])>>3 ) & (1<<i))
			{
				buffer |= ( ((i<2) ? 0x07 : 0x03) << 3*i);
			}
		}
		send_data(buffer);
		send_data(buffer);
		send_data(buffer);
		buffer = 0;
	}
	send_data(0);
	send_data(0);
	send_data(0);
	
	set_oled_pagecolumn(page+2,column);
	for(j = 0; j < FONT_PIXEL_WIDTH; j++)
	{
		for (i = 0 ; i < 2 ; i++)
		{
			if( ( pgm_read_byte( &font_bytes[(target-ASCII_OFFSET)*FONT_PIXEL_WIDTH+j])>>6 ) & (1<<i))
			{
				buffer |= ( 0x07 << 3*i);
			}
		}
		send_data(buffer);
		send_data(buffer);
		send_data(buffer);
		buffer = 0;
	}
	send_data(0);
	send_data(0);
	send_data(0);
}

static void _print_quadsized_char(char target, uint8_t page, uint8_t column)
{
	size_t i, j;
	uint8_t buffer;
	//uint8_t mask;
	
	set_oled_pagecolumn(page,column);
	for(j = 0; j < FONT_PIXEL_WIDTH; j++)
	{
		for (i = 0 ; i < 2 ; i++) 
		{
			if( pgm_read_byte( &font_bytes[(target-ASCII_OFFSET)*FONT_PIXEL_WIDTH+j]) & (1<<i))
			{
				buffer |= 0x0F << 4*i;
			}
		}
		send_data(buffer);
		send_data(buffer);
		send_data(buffer);
		send_data(buffer);
		buffer = 0;
	}
		send_data(0);
		send_data(0);
		send_data(0);
		send_data(0);
	
	set_oled_pagecolumn(page+1,column);
	for(j = 0; j < FONT_PIXEL_WIDTH; j++)
	{
		for (i = 0 ; i < 2 ; i++) 
		{
			if( ( pgm_read_byte( &font_bytes[(target-ASCII_OFFSET)*FONT_PIXEL_WIDTH+j])>>2 ) & (1<<i))
			{
				buffer |= 0x0F << 4*i;
			}
		}
		send_data(buffer);
		send_data(buffer);
		send_data(buffer);
		send_data(buffer);
		buffer = 0;
	}
	
	send_data(0);
	send_data(0);
	send_data(0);
	send_data(0);
	
	set_oled_pagecolumn(page+2,column);
	for(j = 0; j < FONT_PIXEL_WIDTH; j++)
	{
		for (i = 0 ; i < 2 ; i++) 
		{
			if( ( pgm_read_byte( &font_bytes[(target-ASCII_OFFSET)*FONT_PIXEL_WIDTH+j])>>4 ) & (1<<i))
			{
				buffer |= 0x0F << 4*i;
			}
		}
		send_data(buffer);
		send_data(buffer);
		send_data(buffer);
		send_data(buffer);
		buffer = 0;
	}
	
	send_data(0);
	send_data(0);
	send_data(0);
	send_data(0);
	
	
	set_oled_pagecolumn(page+3,column);
	for(j = 0; j < FONT_PIXEL_WIDTH; j++)
	{
		for (i = 0 ; i < 2 ; i++) 
		{
			if( ( pgm_read_byte( &font_bytes[(target-ASCII_OFFSET)*FONT_PIXEL_WIDTH+j])>>6 ) & (1<<i))
			{
				buffer |= 0x0F << 4*i;
			}
		}
		send_data(buffer);
		send_data(buffer);
		send_data(buffer);
		send_data(buffer);
		buffer = 0;
	}
	
	send_data(0);
	send_data(0);
	send_data(0);
	send_data(0);
}

#endif

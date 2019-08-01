#ifndef _OLEDSH1106_

#define _OLEDSH1106_

#ifndef F_CPU
#define F_CPU 16000000UL
#endif

#include <string.h> 
#include <avr/io.h>
#include <util/delay.h>
#include "oledfont.h"


/* Commment this to disable rescaling functions (only regular fonts available) */
//#define INCLUDE_RESCALING_FUNCTIONS

#define OLED_SCREEN_MAX_X_RES 132 /* Datasheet specifies 132 but cannot display 2 outer columns and there is an offset */
#define OLED_SCREEN_MAX_Y_RES 64
#define OLED_MAX_PAGES	OLED_SCREEN_MAX_Y_RES/8
#define SH1106_OFFSET 2 /* For some reason cant display first 2 columns */
#define INIT_CALLS_DELAY_US 100

typedef enum fontsize{REGULAR_SIZE_FONT, DOUBLE_SIZE_FONT, TRIPLE_SIZE_FONT, CUADRUPLE_SIZE_FONT} oledfontsize_t;


/* HW SPI */
void spi_init(void);
void send_command(uint8_t command);
static inline void spi_send(uint8_t data);
static inline void send_data(uint8_t data);


void sh1106_128x64_init();
void flush_oled_ram(void);
void flush_oledpage_ram(uint8_t page);
void set_oled_pagecolumn(uint8_t page, uint8_t column);
void draw_xy_point(int16_t x, int16_t y);
void fastdraw_xy_point(uint8_t x, uint8_t y);
void draw_line(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1);

void print_msg(const char * string, oledfontsize_t size, uint8_t page, uint8_t column );
void print_msg_centered(const char * string, oledfontsize_t size, size_t page);

void print_char(const char targetchar, uint8_t page, uint8_t column);
//void print_resized_char(char targetchar, oledfontsize_t size, uint8_t page, uint8_t column);


#endif
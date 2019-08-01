/*
 * OLEDpruebas.c
 *
 * Created: 3/2/2019 5:30:55 PM
 * Author : Lucas
 * OLED SH1106 128x64 driver + Light Menu 
 */ 

#ifndef F_CPU
#define F_CPU 16000000UL
#endif

#include <avr/io.h>
#include <util/delay.h>
#include <stdlib.h>
#include "oledsh1106.h"
#include "menuoled.h"

void process_GUI_controls(void);

/* Global variables that can be reached by menues. */
volatile int testdata = 9;
	

int main(void)
{	
	spi_init();
	//leds_init();
	sh1106_128x64_init();
	
	set_menu_refresh();
	display_menu_oled();
	
	/* rotary encoder inputs: PD2 for A, PD3 for B and PD6 for the button.*/
	PORTD &= ~((1<<PD2)|(1<<PD3)|(1<<PD6)); 


    while (1) 
    {
		process_GUI_controls();
	}
		
    
	
}


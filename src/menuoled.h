#ifndef _MENU_OLED_

#define _MENU_OLED_

#include "oledsh1106.h"
#include <stdlib.h>

/*
Menus use custom data types where basically every menu has an array of options which
can perform specific actions. Every option has a function pointer, a next menu pointer
and a data pointer. 

Expected prototype for the function pointer is "void foo(void)" with no input arguments since
they're meant to run simple tasks, such as toggling a led, changing some value or so. There's
a Next menu pointer if this option should switch between menus and a data pointer (union menuoptdata)
which allows to change the value while using the GUI.

Menu style determines the format of the displayed menu, e.g.  MENU_SHOWDATA_TYPE is used to display
the value of the pointed data in each option next to the text of each option.

Since structs have 4 pointers, every struct is stored in FLASH memory using const __flash.

*/


typedef enum menustyle {MENU_BROWSE_TYPE, MENU_SHOWDATA_TYPE, MENU_CHANGEDATA_TYPE} menu_style_t;
typedef enum bool{FALSE, TRUE} bool_t;

typedef uint8_t menu_cursor_t;
/* typedef before definition of structs contents since each struct refers to the other struct */
typedef struct menu_option menu_opt_t;
typedef struct menu menu_t;

void display_menu_oled(void);
void process_selected_option(void);
void optionselect_animation(void);
void set_menu_refresh(void);
void process_GUI_controls(void);

/* test functions. Delete  */
void toggle_led_prueba(void);
void show_system_info(void);



/*************************/
/* Data type definitions */
/*************************/
union menuoptdata
{
	bool_t * bool_data;
	uint8_t * uint8data;
	int * intdata;
	void * data_ptr;
};

struct menu_option{
	char * option_text;
	void (*action_pfun)(void);
	const __flash menu_t * next_menu;
	union menuoptdata optdata;
};

struct menu{
	char * menu_title;
	const __flash menu_t * prev_menu;
	menu_style_t menu_style;
	uint8_t num_options;
	const __flash menu_opt_t * available_options[]; /* This line must be the last one */
};

struct menu_cfg{
	oledfontsize_t title_fontsize;
	oledfontsize_t options_fontsize;
	char selection_icon_char;
	uint8_t option_indent_pixels;
	uint8_t options_page_begin;
	uint8_t title_pagenum;
};


typedef struct
{
	uint8_t rotary_actual : 1;
	uint8_t rotary_prev : 1;
	uint8_t button_actual: 1;
	uint8_t button_prev: 1;
} rotary_encoder_states_bitfield;


#endif
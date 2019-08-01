#include "menuoled.h"


/* 'private' functions */
static void _enable_data_modify(void);
static void _data_modif_promptmenu_display(void);


/* Extern variables so menu_options can point at them.  */
extern int testdata;


menu_cursor_t option_cursor;
bool_t data_modif_menu_mode;
int encoder_data_value;
bool_t whole_menu_refresh_needed = TRUE;


const __flash menu_t main_menu;
const __flash menu_t change_menu;
const __flash menu_opt_t option_cambiar;
const __flash menu_opt_t option_valorcambio;
const __flash menu_opt_t back_to_main;
const __flash menu_opt_t option_showsystem;


/* Pointer to const data */
static const __flash menu_t * current_menu_ptr = &main_menu;


const __flash menu_t main_menu =
{
	.menu_title = "TEST",
	.available_options = {&option_cambiar, &option_showsystem},
	.num_options = 2,
	.prev_menu = NULL,
	.menu_style = MENU_BROWSE_TYPE
};

const __flash menu_t change_menu =
{
	.menu_title = "CHANGE MENU",
	.available_options = {&option_valorcambio, &back_to_main},
	.num_options = 2,
	.prev_menu = NULL,
	.menu_style = MENU_SHOWDATA_TYPE
};


const __flash menu_opt_t option_cambiar =
{
	.action_pfun= NULL,
	.option_text = "Go to next menu",
	.next_menu = &change_menu,
	.optdata.bool_data = FALSE
};

const __flash menu_opt_t option_valorcambio =
{
	.action_pfun= &_enable_data_modify,
	.option_text = "testdata",
	.next_menu = NULL,
	.optdata.data_ptr = (void*)&testdata
};

const __flash menu_opt_t back_to_main =
{
		.action_pfun= NULL,
		.option_text = "Back to main",
		.next_menu = &main_menu,
		.optdata.bool_data = FALSE
};


const __flash menu_opt_t option_showsystem =
{
	.action_pfun = &show_system_info,
	.option_text = "Show info",
	.next_menu = NULL,
	.optdata.bool_data = NULL
};








struct menu_cfg menuconfig = {
	.title_pagenum = 0,
	.title_fontsize = REGULAR_SIZE_FONT,
	.options_fontsize = REGULAR_SIZE_FONT,
	.selection_icon_char = '-',
	.option_indent_pixels = 10,
	.options_page_begin = REGULAR_SIZE_FONT + 2
};


//////////////////////////////////////////////////////////////////////////////

void display_menu_oled(void)
{

	char strbuffer[OLED_SCREEN_MAX_X_RES/(FONT_PIXEL_WIDTH+1)];
	char itoa_buffer[6];
	uint8_t i, fontscaling = 0;
	
	if(current_menu_ptr == NULL)
		return;

	if(whole_menu_refresh_needed)
	{
		flush_oled_ram(); 
		print_msg_centered(current_menu_ptr->menu_title, menuconfig.title_fontsize, menuconfig.title_pagenum);
	}
	
	if(data_modif_menu_mode)
	{
		_data_modif_promptmenu_display();
		whole_menu_refresh_needed = FALSE;
		return;
	}
	
	/* fontscaling = menuconfig.options_fontsize+1*/
	switch(menuconfig.options_fontsize)
	{
		case REGULAR_SIZE_FONT: fontscaling=1; break;
		case DOUBLE_SIZE_FONT: fontscaling=2; break;
		case TRIPLE_SIZE_FONT: fontscaling=3; break;
		case CUADRUPLE_SIZE_FONT: fontscaling=4; break;
		default: fontscaling=1; break;
	}
		
		
	for (i = 0; i < current_menu_ptr->num_options ; i++)
		{
			/* Should print all the text data once.*/
			if(whole_menu_refresh_needed)
			{
					strcpy(strbuffer, current_menu_ptr->available_options[i]->option_text);
				
					if(current_menu_ptr->menu_style == MENU_SHOWDATA_TYPE
						&& (current_menu_ptr->available_options[i] != &back_to_main))
					{
						strcat(strbuffer, ": ");
						itoa(*((int*)(current_menu_ptr->available_options[i]->optdata.data_ptr)), itoa_buffer, 10); /* Hardcoded int? */
						strcat(strbuffer, itoa_buffer);
					}
					else if(current_menu_ptr->menu_style == MENU_BROWSE_TYPE
						&& (*(current_menu_ptr->available_options[i]->optdata.bool_data) == TRUE))
					{
						strcat(strbuffer, " [*]");
					}	
				
				print_msg(strbuffer, menuconfig.options_fontsize, menuconfig.options_page_begin + i*fontscaling,
					 FONT_PIXEL_WIDTH*fontscaling+1);
			}
			
			print_char( (option_cursor == i)? menuconfig.selection_icon_char : ' ',
					menuconfig.options_page_begin + i*fontscaling, 0);
		}		

	whole_menu_refresh_needed = FALSE;
	return;
}

static void _data_modif_promptmenu_display(void)
{
	/* To to: Some position values are hardcoded */
	
	char itoa_buffer1[OLED_SCREEN_MAX_X_RES/(FONT_PIXEL_WIDTH+1)];
	char itoa_buffer2[6];
	
	if(whole_menu_refresh_needed)
		print_msg_centered(current_menu_ptr->available_options[option_cursor]->option_text, DOUBLE_SIZE_FONT, 2);
				
	itoa(*((int*)current_menu_ptr->available_options[option_cursor]->optdata.data_ptr) , itoa_buffer1, 10  ); /*HC*/
	itoa(encoder_data_value, itoa_buffer2, 10);
				
	strcat(itoa_buffer1, "->");
	strcat(itoa_buffer1, itoa_buffer2);
				
	print_msg_centered(itoa_buffer1,TRIPLE_SIZE_FONT, 5/*HC*/);

	return;
}



void optionselect_animation(void)
{
	/* FIX: An animation gives a better feel but blocking the 
	 program is REALLY inefficient. */
	print_char('>', menuconfig.options_page_begin + option_cursor , 0);
	_delay_ms(100); /* !!! */ 
}


void process_selected_option(void)
{
	/* To be called when the button on the rotary encoder is pressed. */
	if(current_menu_ptr == NULL)
		return;
	
	optionselect_animation();
	
	/* Sets data if modify mode is enabled */
	if(data_modif_menu_mode == 1)
	{
		*((int*)current_menu_ptr->available_options[option_cursor]->optdata.data_ptr) = encoder_data_value;
		data_modif_menu_mode = 0;
		return;
	}
	
	if(current_menu_ptr->available_options[option_cursor]->action_pfun != NULL)
		current_menu_ptr->available_options[option_cursor]->action_pfun();
	
	if(current_menu_ptr->available_options[option_cursor]->next_menu != NULL)
	{
		current_menu_ptr = current_menu_ptr->available_options[option_cursor]->next_menu;
		option_cursor = 0;
	}

	return;
	
}

static void _enable_data_modify(void)
{
	encoder_data_value = *((int*)current_menu_ptr->available_options[option_cursor]->optdata.data_ptr); /* Start value */
	data_modif_menu_mode = TRUE;
}


void show_system_info(void)
{
	print_msg_centered("test!", DOUBLE_SIZE_FONT, 0);
	_delay_ms(3000);
}



static inline uint8_t get_menu_num_options(void)
{
	return current_menu_ptr->num_options;
}

void set_menu_refresh(void)
{
	whole_menu_refresh_needed = TRUE;
}


void process_GUI_controls(void)
{
	/* No debouncing algorithm since using a 10nF cap seems to solve bounce
	using a KY-40 module. */
	/* No edge interrupt nor timers used */
	/* Avoid using this in main loop if loop time is high */
	
	/* To do: getters. Atmega328 pin read hardcoded.*/
	
	static rotary_encoder_states_bitfield r_encoder_states;
	
	/* Check for rotary spin */
	r_encoder_states.rotary_actual = (PIND & (1<<PD2))? 1:0;
	if( !(r_encoder_states.rotary_actual) && r_encoder_states.rotary_prev) /* if falling edge */
	{		
		/* Given any of the rotary's outputs, the spin direction
			is determined at the falling edge by checking the status of the other output
			at this instant: If it was already low or still high at the time of the falling edge*/
		if(PIND & (1<<PD3))  
		{
			if(data_modif_menu_mode) /* Implementar limites */
				encoder_data_value--;
			else if(option_cursor > 0)
				option_cursor--;
		}
		else
		{
			if(data_modif_menu_mode)
				encoder_data_value++;
			else if(option_cursor < get_menu_num_options() - 1)
				option_cursor++;
		}
		display_menu_oled();
	}
			
	r_encoder_states.rotary_prev = r_encoder_states.rotary_actual;
				
		
	/* Check if button is pressed */
	r_encoder_states.button_actual = (PIND & (1<<PD6))? 1:0;
	if( !(r_encoder_states.button_actual) && r_encoder_states.button_prev) /* if falling edge */
	{			
		process_selected_option();
		set_menu_refresh();
		display_menu_oled();
	}
	r_encoder_states.button_prev = r_encoder_states.button_actual;
		
	return;
}
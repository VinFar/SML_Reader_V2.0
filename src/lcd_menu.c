/*
 * lcd_menu.c
 *
 *  Created on: 16.09.2019
 *      Author: vfv13
 */

#include "lcd_menu.h"
#include "main.h"
#include "eeprom.h"
#include "timer.h"
#include "time.h"
#include <stdio.h>
#include <stdarg.h>
#include "string.h"
#include "i2clcd.h"
#include "rtc.h"

uint32_t old_ctr_cnt;

int32_t menu_timer_index = 0;

menu_t *current_menu_ptr = &Hauptmenu;

int8_t menu_add_submenu(menu_t *prev, menu_t *sub, uint8_t index_item) {

	if (sub->items == NULL || prev->items == NULL) {
		return -1;
	}

	/*
	 * set the correct submenu for item with the index 'index_item'
	 */
	prev->items[index_item].menu_ptr = sub;
	prev->items[index_item].on_push = &on_push;

	/*
	 * init the back button of the submenu with the previus menu
	 */
	sub->items[0].menu_ptr = prev;
	sub->items[0].on_push = &on_push;
	return 0;

}

void menu_init_menu(menu_t *instance, item_t *items, uint8_t nbr_of_items) {

	/*
	 * init item struct with zero
	 */
	memset(items, 0, sizeof(items) * nbr_of_items);

	/*
	 * set correct size in menu struct
	 */
	instance->size = nbr_of_items;

	/*
	 * set correct pointer in menu struct to item struct
	 */
	instance->items = items;

	char tmp[nbr_of_items];
	char men[nbr_of_items];
	for (int i = 0; i < nbr_of_items; i++) {
		strcpy(men, "Menu");
		itoa(i, tmp, 10);
		strcat(men, tmp);
		strcpy(instance->items[i].string, men);
		instance->items[i].on_push = NULL;
		instance->items[i].on_push_delayed = &on_push_reset_system;
		instance->items[i].on_rotate = &on_rotate_refresh_lcd;
	}
	instance->items[0].on_rotate = &on_rotate_refresh_lcd;
	/*
	 * init the back item to call the main menu
	 */
	instance->items[0].on_push = &on_push_go_back;

	strcpy(instance->items[0].string, (char*) "Zurueck");

}

void menu_init(menu_t *main_menu, item_t *main_menu_items, uint8_t size) {
	menu_init_menu(main_menu, main_menu_items, size);

	menu_init_menu(&menu_changing_value, menu_changing_value_item,
			SIZE_OF_MENU(menu_changing_value_item));
	/*
	 * universal menu for changing a value with the rotary encoder
	 */
	menu_init_menu(&menu_changing_value, menu_changing_value_item,
			SIZE_OF_MENU(menu_changing_value_item));

	menu_changing_value.items[0].on_rotate = &on_rotate_change_value;
	menu_changing_value.items[1].on_rotate = &on_rotate_change_value;

	menu_changing_value.items[0].on_push = &on_push;
	menu_changing_value.items[1].on_push = &on_push;

	menu_changing_value.user_data = NULL;
	menu_init_text(&menu_changing_value.items[0], "");
	menu_init_text(&menu_changing_value.items[1], "");

}

void menu_init_text(item_t *item, char text[]) {

//	if(strlen(text)>LCD_COLS){
//		/*
//		 * can't be displayed on the LCD, so snip it and insert a point at the end
//		 */
//		text[20] = '.';
//		text[21] = '\0';
//	}

	strcpy(item->string, text);
}

void menu_printf(item_t *item, const char *fmt, ...) {

//	item->user_data = (void*)

	va_list va;
	va_start(va, fmt);
	vsiprintf(item->string, fmt, va);
	va_end(va);
	uint8_t len = strlen(item->string);
	if (item->on_push != NULL) {
		/*
		 * menu has a submenu so at arrow
		 */
		for (; len < LCD_COLS - 2; len++) {
			strcat(item->string, " ");
		}
		strcat(item->string, ">");
	}
}

void menu_fct_on_push(item_t *item, void (*ptr)(menu_t *instance)) {
	item->on_push = ptr;
}

void menu_fct_on_delayed_push(item_t *item, void (*ptr)(menu_t *instance)) {
	item->on_push_delayed = ptr;
}

void menu_fct_on_rotate(item_t *item, void (*ptr)()) {
	item->on_rotate = ptr;
}

void menu_add_userdata(item_t *item, void *ptr_to_data) {
	item->user_data = ptr_to_data;
}

void menu_printf_add_itemvalue(item_t *item, void *ptr_to_data, const char *fmt,
		...) {

	for (int i = 0; fmt[i] != '\0'; i++) {
		if (fmt[i] == '%') {
			if (i < 19) {
				if (fmt[i + 1] != '%') {
					item->idx_for_value = i;
					break;
				}
			}
		}
	}

	va_list va;
	va_start(va, fmt);
	vsiprintf(item->string, fmt, va);
	va_end(va);
	uint8_t len = strlen(item->string);
	if (item->on_push != NULL) {
		/*
		 * menu has a submenu so add arrow
		 */
		for (; len < LCD_COLS - 2; len++) {
			strcat(item->string, " ");
		}
		strcat(item->string, ">");
	}
	menu_add_userdata(item, ptr_to_data);
}

void menu_info_print() {

	char tmp_string[20] = "";

	lcd_clear();

	switch (((menu_idx_isr) % 3)) {				//Main manue
	case 0:

		lcd_setcursor(1, 1);
		lcd_print("PV:");
		lcd_setcursor(1, 4);
		itoa((int32_t) sm_power_plant_current, tmp_string, 10);
		strcat(tmp_string, "W");
		lcd_print(tmp_string);

		lcd_setcursor(2, 1);
		lcd_print("Main:");
		lcd_setcursor(2, 6);
		itoa((int32_t) sm_power_main_current, tmp_string, 10);
		strcat(tmp_string, "W");
		lcd_print(tmp_string);

		lcd_setcursor(3, 1);
		lcd_print("TX ctr:");
		lcd_setcursor(3, 9);
		itoa(nrf24_tx_ctr, tmp_string, 10);
		lcd_setcursor(3, 9);
		lcd_print(tmp_string);

		lcd_setcursor(4, 1);
		lcd_print("MW Plant:");
		lcd_setcursor(4, 10);
		itoa(sm_power_plant_mean, tmp_string, 10);
		lcd_setcursor(4, 10);
		strcat(tmp_string, "W");
		lcd_print(tmp_string);

		break;
	case 1:						//Menue two
		lcd_setcursor(1, 1);
		lcd_print("PV:");
		lcd_setcursor(1, 4);
		itoa((int32_t) sm_consumption_plant, tmp_string, 10);
		strcat(tmp_string, "kWh");
		lcd_print(tmp_string);

		lcd_setcursor(2, 1);
		lcd_print("MainDel:");
		lcd_setcursor(2, 9);
		itoa((int32_t) sm_consumption_main_del, tmp_string, 10);
		strcat(tmp_string, "kWh");
		lcd_print(tmp_string);

		lcd_setcursor(3, 1);
		lcd_print("MainPur:");
		lcd_setcursor(3, 9);
		itoa(sm_consumption_main_pur, tmp_string, 10);
		lcd_setcursor(3, 9);
		strcat(tmp_string, "W");
		lcd_print(tmp_string);

		break;
	case 2:
		NOP
		char str[21] = { { TOPLINE } };
		lcd_print_frame();
		snprintf(str, 20, "%02d.%02d.%02d", sm_date.Date, sm_date.Month,
				sm_date.Year);
		lcd_printlc(2, 7, str);
		snprintf(str, 20, "%02d:%02d:%02d", sm_time.Hours, sm_time.Minutes,
				sm_time.Seconds);
		lcd_printlc(3, 7, str);
		break;
	default:
		lcd_setcursor(1, 1);
		lcd_print("should not happen!:(");
		//should not happen
		break;

		break;
	}

	return;
}

void on_push(menu_t *instance) {

	flags.refreshed_rotary = 1;
	current_menu_ptr = current_menu_ptr->items[menu_timer_index].menu_ptr;
	if (current_menu_ptr != &menu_changing_value) {
		/*
		 * We don't want to reset the menu index when we are enterend the menu for
		 * changing a value of an item.
		 * This is because the changing value menu needs to now from which
		 * item (indexed by menu indes) it is called, in order to
		 * be able to change its value.
		 * If it would be reset to 0 that menu could not now the
		 * item from which it is called.
		 */
		menu_timer_index = 0;
	}

	return;
}

void on_push_go_back(menu_t *instance) {

	flags.refreshed_rotary = 1;
	flags.currently_in_menu = ~flags.currently_in_menu;
	current_menu_ptr = &Hauptmenu;
	menu_timer_index = 0;
	return;

}

uint32_t on_rotate_refresh_lcd(menu_t *ptr, int32_t index) {
	if (flags.currently_in_menu) {
		/*
		 * we have entered the menu
		 */

		if (index > ptr->size - 1) {
			index = ptr->size - 1;
		} else if (index < 0) {
			index = 0;
		}

		uint8_t page = index / 4; //current page. of the menu
		uint8_t page_index = index % 4; // index in the current pagee

		lcd_clear();

		for (uint8_t i = 4 * page; i < 4 * page + 4; i++) {
			if (i > ptr->size - 1) {
				break;
			}
			if (ptr->items[i].user_data != NULL) {
				char istr[10];
				itoa((int32_t)*((int32_t*)ptr->items[i].user_data),istr,10);
				memset(&ptr->items[i].string[ptr->items[i].idx_for_value],' ',20-ptr->items[i].idx_for_value);
				uint8_t k=0;
				while(ptr->items[i].string[k]!='\0' && istr[k]!='\0'){
					ptr->items[i].string[ptr->items[i].idx_for_value + k] = istr[k];
					k++;
				}
			}
			lcd_printlc(i - 4 * page + 1, 2, ptr->items[i].string);
		}
		lcd_setcursor(page_index + 1, 1);
		lcd_putchar('>');
	} else {
		menu_info_print();
	}
	return index;
}

uint32_t on_rotate_change_value(menu_t *instance, uint32_t index) {

	/*
	 * A value of an item an be changed over this function.
	 * It is called from the item that wants its value changed and
	 * passes the current menu pointer, which is obviously the
	 * 'menu_changing_value', but also the current index of the calling
	 * item.
	 * The displayed items are not changed, but the value of the selected item.
	 * It is necessary to that the value that has to be changed is present
	 * in the displayed string (exact value), because it searches the position
	 * of this value and replaced it by the new value.
	 * ToDo: change this method by a better one, without using additional RAM
	 */

	if (instance == NULL) {
		return index;
	}
	instance = instance->items[0].menu_ptr;
	if (instance->items[index].user_data == NULL) {
		return index;
	}

	uint32_t data = *(uint32_t*) instance->items[index].user_data;

	float jog_value = (float) TIM17->CNT; //Calculate difference
	jog_value = (1000 / jog_value) + 1;

	jog_value = 10;

	TIM17->CNT = 0;
	TIM17_ENABLE;

	if (flags.rotary_direction) {
		data += (uint32_t) jog_value;

	} else {
		data -= (uint32_t) jog_value;
	}
	*((uint32_t*) instance->items[index].user_data) = data;
	char needle[10];
	itoa(data, needle, 10);

	for (uint8_t i = 0;
			needle[i] != '\0'
					&& instance->items[index].string[instance->items[index].idx_for_value
							+ i] != '\0'; i++) {
		instance->items[index].string[instance->items[index].idx_for_value + i] =
				needle[i];
	}
	instance->items[0].on_rotate(instance, menu_timer_index);
	return index;

}

void on_push_reset_value(menu_t *instance) {

	if (instance->items[menu_timer_index].user_data == NULL) {
		return;
	}
	eeprom_t *eeprom_data =
			((eeprom_t*) (instance->items[menu_timer_index].user_data));

	memset(&eeprom_data->data, 0, eeprom_data->size);

}

void on_push_reset_system(menu_t *instance) {

	NVIC_SystemReset();
}


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
#include "i2clcd.h"

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
	prev->items[index_item].on_push = &call_menu;

	/*
	 * init the back button of the submenu with the previus menu
	 */
	sub->items[0].menu_ptr = prev;
	sub->items[0].on_push = &call_menu;
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
		instance->items[i].on_rotate = &lcd_refresh_rotary;
	}
	instance->items[0].on_rotate = &lcd_refresh_rotary;
	/*
	 * init the back item to call the main menu
	 */
	instance->items[0].on_push = &go_back_to_main_menu;

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

	menu_changing_value.items[0].on_rotate = &on_rotary_change_value;
	menu_changing_value.items[1].on_rotate = &on_rotary_change_value;

	menu_changing_value.items[0].on_push = &call_menu_change_value;
	menu_changing_value.items[1].on_push = &call_menu_change_value;

	menu_changing_value.user_data = NULL;
	menu_init_text(&menu_changing_value.items[0], "");
	menu_init_text(&menu_changing_value.items[1], "");
	menu_fct_for_push(&menu_changing_value.items[0], &call_menu_change_value);

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

void menu_fct_for_push(item_t *item, void (*ptr)(menu_t *instance)) {
	item->on_push = ptr;
}

void menu_fct_for_delayed_push(item_t *item, void (*ptr)(menu_t *instance)) {
	item->on_push_delayed = ptr;
}

void menu_fct_for_rotary(item_t *item, void (*ptr)()) {
	item->on_rotate = ptr;
}

void menu_add_userdata(item_t *item, void *ptr_to_data) {
	item->user_data = ptr_to_data;
}

void menu_printf_add_itemvalue(item_t *item, void *ptr_to_data, const char *fmt,
		...) {
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

void call_menu(menu_t *instance) {

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

void call_menu_steckdoseneinstellunge(menu_t *instance) {

	flags.refreshed_rotary = 1;
	current_menu_ptr = current_menu_ptr->items[menu_timer_index].menu_ptr;
	current_menu_ptr->items[0].user_data =
			instance->items[menu_timer_index].user_data;
	menu_timer_index = 0;

	return;
}

void call_menu_change_value(menu_t *instance) {

	flags.refreshed_rotary = 1;
	TIM17_DISABLE;
	TIM15->CNT = 0xffff;

	current_menu_ptr = current_menu_ptr->items[0].menu_ptr;

	menu_timer_index = 0;

	return;
}

void go_back_to_main_menu(menu_t *instance) {

	flags.refreshed_rotary = 1;
	flags.currently_in_menu = ~flags.currently_in_menu;
	current_menu_ptr = &Hauptmenu;
	menu_timer_index = 0;
	return;

}

void lcd_printint(int32_t data) {

	char tmp[32] = "";
	itoa(data, tmp, sizeof(tmp));
	lcd_print(tmp);

	return;
}

void lcd_printarrow(int line) {

	if (line > 4) {
		line = 4;
	}
	if (line < 1) {
		line = 1;
	}

	lcd_setcursor(old_arrow_line, 1);
	lcd_putchar(' ');
	lcd_setcursor(line, 1);
	lcd_putchar('>');
	old_arrow_line = line;
	return;

}

void lcd_refresh_push() {
	if (flags.currently_in_menu) {
		/*
		 * we are in the menu
		 */
		if (current_menu_ptr->items[menu_timer_index].on_push == NULL) {
			/*
			 * no function pointer, so abort
			 */
			return;
		} else {
			current_menu_ptr->items[menu_timer_index].on_push(
					current_menu_ptr->items[menu_timer_index].user_data);
		}
	} else {
		flags.currently_in_menu = 1;
	}
	flags.refreshed_rotary = 1;
	old_ctr_cnt = menu_timer_index;
	return;
}

uint32_t lcd_refresh_rotary(menu_t *ptr, int32_t index) {
	int32_t ctr_cnt = menu_timer_index;
	if (flags.currently_in_menu) {
		/*
		 * we have entered the menu
		 */
//		index = menu_timer_index
		old_ctr_cnt = ctr_cnt;

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
			lcd_printlc(i - 4 * page + 1, 2, ptr->items[i].string);
		}
		lcd_setcursor(page_index + 1, 1);
		lcd_putchar('>');
	} else {
		lcd_print_info();
	}
	return index;
}

void lcd_print_info() {

	char tmp_string[20] = "";

	lcd_clear();

	switch (((menu_timer_index) % 2)) {				//Main manue
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
	default:
		lcd_setcursor(1, 1);
		lcd_print("should not happen!:(");
		//should not happen
		break;

		break;
	}

	return;
}

void lcd_print_value_unit(int pos_line, int pos_row, char *value, char *unit) {

	for (char i = pos_row; i <= 20; i++) {		//flush line
		lcd_setcursor(pos_line, i);
		lcd_putchar(' ');
	}
	lcd_setcursor(pos_line, pos_row);
	lcd_print(value);
	lcd_print(unit);

}

uint32_t on_rotary_change_value(menu_t *instance, uint32_t index) {

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

	char needle[10] = { 0 };
	uint32_t data = *(uint32_t*) instance->items[index].user_data;

	itoa(data, needle, 10);
	char *needle_found = strstr(instance->items[index].string, needle);
	if (needle_found == NULL) {
		return index;
	}
	uint8_t pos = needle_found - instance->items[index].string;

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
	itoa(data, needle, 10);

	for (uint8_t i = 0;
			needle[i] != '\0' && instance->items[index].string[pos] != '\0';
			i++, pos++) {
		instance->items[index].string[pos] = needle[i];
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

//	NVIC_SystemReset();
}

static int lcd_fwrite(int file, char *ptr, int len) {
	int DataIdx;

	for (DataIdx = 0; DataIdx < len; DataIdx++) {
		lcd_putchar(*ptr++);
	}
	return len;
}

int8_t lcd_printf(const char *fmt, ...) {
	int length = 0;
	va_list va;
	va_start(va, fmt);
	length = ts_formatlength(fmt, va);
	va_end(va);
	{
		char buf[length];
		va_start(va, fmt);
		length = ts_formatstring(buf, fmt, va);
		length = lcd_fwrite(1, buf, length);
		va_end(va);
	}
	return length;
}

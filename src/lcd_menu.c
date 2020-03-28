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

int32_t menu_index = 0;

menu_t *current_menu_ptr = &Hauptmenu;

void call_menu(menu_t *instance) {

	flags.refreshed_rotary = 1;
	current_menu_ptr = current_menu_ptr->items[menu_index].menu_ptr;
	menu_index = 0;

	return;
}

void call_menu_steckdoseneinstellunge(menu_t *instance) {

	flags.refreshed_rotary = 1;
	current_menu_ptr = current_menu_ptr->items[menu_index].menu_ptr;
	current_menu_ptr->items[0].user_data =
			instance->items[menu_index].user_data;
	menu_index = 0;

	return;
}

void call_menu_change_value(menu_t *instance) {

	flags.refreshed_rotary = 1;
	TIM15_DISABLE;
	TIM15->CNT = 0xffff;
	outlets_t *outlet =
			((outlets_t*) current_menu_ptr->items[menu_index].user_data);

	current_menu_ptr = current_menu_ptr->items[menu_index].menu_ptr;

	int32_t old_power_value = 0;
	eeprom_read_data(outlet->eeprom_page, outlet->eeprom_byte,
			(uint8_t*) &old_power_value, 4);

	/*
	 * if the value has changed we have to set correct value for the consumer
	 * power value
	 */
	if (outlet->state == 1) {
		powervalue_used_by_consumers -= old_power_value;
		powervalue_used_by_consumers += outlet->union_value.value;
	}
	eeprom_write_data(outlet->eeprom_page, outlet->eeprom_byte,
			(uint8_t*) &outlet->union_value.value, 4);

	menu_index = 0;

	return;
}


void go_back_to_main_menu(menu_t *instance) {

	flags.refreshed_rotary = 1;
	flags.currently_in_menu = ~flags.currently_in_menu;
	current_menu_ptr = &Hauptmenu;
	menu_index = 0;
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
		if (current_menu_ptr->items[menu_index].on_push == NULL) {
			/*
			 * no function pointer, so abort
			 */
			return;
		} else {
			current_menu_ptr->items[menu_index].on_push(
					current_menu_ptr->items[menu_index].user_data);
		}
	} else {
		flags.currently_in_menu = 1;
	}
	flags.refreshed_rotary = 1;
	old_ctr_cnt = menu_timer_index;
	return;
}

void lcd_refresh_rotary() {
	int32_t ctr_cnt = menu_timer_index;
	if (flags.currently_in_menu) {
		/*
		 * we have entered the menu
		 */
		if (old_ctr_cnt > ctr_cnt) {
			menu_index++;
		} else {
			menu_index--;

		}
		old_ctr_cnt = ctr_cnt;

		if (menu_index > current_menu_ptr->size - 1) {
			menu_index = current_menu_ptr->size - 1;
		} else if (menu_index < 0) {
			menu_index = 0;
		}

		uint8_t page = menu_index / 4; //current page. of the menu
		uint8_t page_index = menu_index % 4; // index in the current pagee

		lcd_clear();

		for (uint8_t i = 4 * page; i < 4 * page + 4; i++) {
			if (i > current_menu_ptr->size - 1) {
				break;
			}
			lcd_setcursor(i - 4 * page + 1, 2);
			lcd_print(current_menu_ptr->items[i].string);
		}
		lcd_setcursor(page_index + 1, 1);
		lcd_putchar('>');
	} else {
		lcd_print_info();
	}
	return;
}

void lcd_print_info() {

	char tmp_string[20] = "";

	lcd_clear();

	switch (((menu_timer_index) % 5)) {				//Main manue
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
//		strcat(tmp_string, "W");
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
		lcd_setcursor(1, 1);
		lcd_print("1:");
		lcd_print_on_off(1, 4, outlets[0].state);

		lcd_setcursor(2, 1);
		lcd_print("2:");
		lcd_print_on_off(2, 4, outlets[1].state);

		lcd_setcursor(3, 1);
		lcd_print("3:");
		lcd_print_on_off(3, 4, outlets[2].state);

		lcd_setcursor(1, 9);
		lcd_print("4:");
		lcd_print_on_off(1, 12, outlets[3].state);

		lcd_setcursor(2, 9);
		lcd_print("5:");
		lcd_print_on_off(2, 12, outlets[4].state);

		lcd_setcursor(3, 9);
		lcd_print("6:");
		lcd_print_on_off(3, 12, outlets[5].state);

		break;
	case 3:
		lcd_setcursor(1, 1);
		lcd_print("mean time:");
		lcd_setcursor(1, 11);
		itoa((int) 0, tmp_string, 10);
		strcat(tmp_string, "ms");
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

void lcd_print_checkbox(int pos_line, int pos_row, unsigned flag) {

	lcd_setcursor(pos_line, pos_row);
	if (flag) {
		lcd_putchar(CHECKBOX);
	} else {
		lcd_putchar(' ');
	}

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

void lcd_print_on_off(int pos_line, int pos_row, unsigned flag) {

	lcd_setcursor(pos_line, pos_row);
	if (flag) {
		lcd_print("ein");
	} else {
		lcd_print("aus");
	}

}

void menu_add_submenu(menu_t *prev, menu_t *sub, uint8_t index_item) {

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

}

void menu_init_struct(menu_t *instance, item_t *items, uint8_t nbr_of_items) {

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

void on_rotary_change_value(menu_t *instance) {

	outlets_t *outlet = ((outlets_t*) instance->items[menu_index].user_data);
	lcd_clear();

	float jog_value = TIM15->CNT; //Calculate difference
	jog_value = (1000 / jog_value) + 1;

	jog_value = 10;

	TIM15->CNT = 0;
	TIM15_ENABLE;

	if (TIM3->CR1 & TIM_CR1_DIR) {
		outlet->union_value.value += (uint32_t) jog_value;
		if (outlet->union_value.value > 3680) {
			outlet->union_value.value = 3680;
		}
	} else {
		outlet->union_value.value -= (uint32_t) jog_value;
		if (outlet->union_value.value < 0) {
			outlet->union_value.value = 0;
		}
	}
	char value_string[20] = { 0 };
	itoa(outlet->union_value.value, value_string, 10);
	lcd_print_value_unit(2, 2, value_string, (char*) "W");

}

void on_push_reset_value(menu_t *instance) {

	if (instance->items[menu_index].user_data == NULL) {
		return;
	}
	eeprom_t *eeprom_data =
			((eeprom_t*) (instance->items[menu_index].user_data));

	memset(&eeprom_data->data, 0, eeprom_data->size);

}

void on_push_reset_system(menu_t *instance) {

	NVIC_SystemReset();
}

static lcd_fwrite(int file, char *ptr, int len) {
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

/*
 * lcd_menu.h
 *
 *  Created on: 16.09.2019
 *      Author: vfv13
 */

#ifndef LCD_MENU_H_
#define LCD_MENU_H_

#include "functions.h"

#define SIZE_OF_MENU(items) (sizeof(items) / sizeof(items[0]))

union type_union {
	float float_data;
	uint32_t uint32_data;
	uint16_t uint16_data[2];
	uint8_t uint8_data[4];
};

typedef union type_union item_data;

item_data outlet1_data;
item_data outlet2_data;
item_data outlet3_data;

typedef struct item_struct item_t;

typedef struct menu_struct {
	item_t *items;
	uint8_t size;
	void *user_data;
} __attribute__((packed)) menu_t;

/*
 * these are the individual items
 */
struct item_struct {
	char string[20];
	menu_t *menu_ptr;
	void (*on_push)(menu_t *instance);
	void (*on_push_delayed)(menu_t *instance);
	uint8_t (*on_rotate)(menu_t *instance, uint8_t index);
	void *user_data;
};

extern int32_t menu_timer_index;
extern menu_t *current_menu_ptr;

int8_t menu_add_submenu(menu_t *prev, menu_t *sub, uint8_t index_item);
void menu_add_prevmenu(menu_t *main, menu_t *prev);
void menu_init_menu(menu_t *instance, item_t *items, uint8_t nbr_of_items);
void menu_init_text(item_t *item, char text[]);
void menu_fct_for_rotary(item_t *item, void (*ptr)());
void menu_add_userdata(item_t *item, void *ptr_to_data);
void menu_fct_for_push(item_t *item, void (*ptr)());
void menu_fct_for_delayed_push(item_t *item, void (*ptr)(menu_t *instance));
uint32_t on_rotary_change_value(menu_t *instance, uint32_t index);
void on_push_reset_value(menu_t *instance);
void on_push_reset_system(menu_t *instance);
void menu_printf(item_t *item, const char *fmt, ...);
void menu_printf_add_itemvalue(item_t *item, void *ptr_to_data, const char *fmt, ...);
void menu_init(menu_t *main_menu, item_t *main_menu_items,uint8_t size);
/*
 * Main menu
 */
menu_t Hauptmenu;
item_t Hauptmenu_items[5];

/*
 * submenu for changing a value
 */
menu_t menu_changing_value;
item_t menu_changing_value_item[2];

/*
 * Infomenu
 */
menu_t menu_system_info;
item_t infomenu_items[8];

/*
 * system settings
 */
menu_t system_settings;
item_t system_settings_items[8];

#define SIZE_OF_MENU_STRUCT ((int32_t)(sizeof(menu_struct)))

void on_push_start_stopp_usart(menu_t *instance);
void call_menu(menu_t *instance);
void call_menu_change_value(menu_t *instance);
void call_menu_steckdoseneinstellunge(menu_t *instance);
void on_push_prevmenu(menu_t *instance);
void go_back_to_main_menu(menu_t *instance);

#endif /* LCD_MENU_H_ */

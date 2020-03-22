/*
 * lcd_menu.h
 *
 *  Created on: 16.09.2019
 *      Author: vfv13
 */

#ifndef LCD_MENU_H_
#define LCD_MENU_H_

#include "functions.h"

#define SIZE_OF_MENU(items) (sizeof(items) / sizeof(item_t))

union type_union{
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
}__attribute__((packed)) menu_t;

/*
 * these are the individual items
 */
struct item_struct {
	char string[20];
	menu_t *menu_ptr;
	void (*on_push)(menu_t *instance);
	void (*on_push_delayed)(menu_t *instance);
	void (*on_rotate)(menu_t *instance);
	void *user_data;
};


extern int32_t menu_index;
extern menu_t *current_menu_ptr;


void menu_add_submenu(menu_t *prev, menu_t *sub,uint8_t index_item);
void menu_add_prevmenu(menu_t *main, menu_t *prev);
void menu_init_struct(menu_t *instance, item_t *items,uint8_t nbr_of_items);
void menu_init_text(item_t *item,char text[]);
void menu_fct_for_rotary(item_t *item, void (*ptr)());
void menu_add_userdata(item_t * item, void *ptr_to_data);
void menu_fct_for_push(item_t *item, void (*ptr)());
void menu_fct_for_delayed_push(item_t *item, void (*ptr)(menu_t *instance));
void on_rotary_change_value(menu_t *instance);
void on_push_reset_value(menu_t *instance);
void on_push_reset_system(menu_t *instance);
void menu_printf(item_t *item, const char *fmt, ...);
/*
 * Main menu
 */
menu_t Hauptmenu;
item_t Hauptmenu_items[5];

/*
 * submenu for changing a value
 */
menu_t changing_value;
item_t changing_value_item[NUMBER_OF_OUTLETS];

/*
 * Infomenu
 */
menu_t infomenu;
item_t infomenu_items[7];

/*
 *Maxima
 */
menu_t maxima_menu;
item_t maxima_items[3];

/*
 *Minima
 */
menu_t minima_menu;
item_t minima_items[3];

/*
 *genutze Leistung Menu
 */
menu_t used_energy_menu;
item_t used_energy_items[3];

/*
 *24h mean
 */
menu_t mean_24h_menu;
item_t mean_24h_items[3];

/*
 *7d mean
 */
menu_t mean_7d_menu;
item_t mean_7d_items[3];

/*
 *30d mean
 */
menu_t mean_30d_menu;
item_t mean_30d_items[3];

/*
 *1y mean
 */
menu_t mean_1y_menu;
item_t mean_1y_items[3];

/*
 * system settings
 */
menu_t system_settings;
item_t system_settings_items[5];

#define SIZE_OF_MENU_STRUCT ((int32_t)(sizeof(menu_struct)))

void on_push_start_stopp_usart(menu_t *instance);
void call_menu(menu_t *instance);
void call_menu_change_value(menu_t *instance);
void call_menu_steckdoseneinstellunge(menu_t *instance);
void on_push_prevmenu(menu_t *instance);
void go_back_to_main_menu(menu_t *instance);

#endif /* LCD_MENU_H_ */

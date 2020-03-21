/*
 * functions.h
 *
 *  Created on: 23.12.2019
 *      Author: vfv13
 */

#ifndef FUNCTIONS_H_
#define FUNCTIONS_H_

#include "main.h"
#include "lcd_menu.h"

extern int32_t old_Powervalue;
extern int32_t tmp_PWR;

extern int32_t powervalue_current, powervalue_mean,powervalue_used_by_consumers;
extern int32_t total_consumption, total_money;
extern int32_t highest_prio;
extern int32_t powervalue_no_mean;

extern int32_t power_value_history[300];
extern uint16_t time_history[300];
extern uint16_t time_mean;
extern uint16_t milli_seconds_passsed;
extern uint16_t seconds_for_meanvalue;
extern uint32_t seconds_passed;
extern uint8_t P_CONFIG;
extern int32_t menu_timer_index;

typedef struct outlets {
	union {
		int32_t value;
		uint8_t value_by_byte[4];
	} union_value;
	unsigned state :1;
	unsigned lock :1;
	uint16_t PORT;
	GPIO_TypeDef *BANK;
	uint8_t prio;
	char *ptr_to_string;
	uint8_t eeprom_page;
	uint8_t eeprom_byte;
	uint8_t io_exp_output;
}__attribute__((packed)) outlets_t;

extern outlets_t outlets[NUMBER_OF_OUTLETS];
extern uint32_t *outlets_prio_ptr[NUMBER_OF_OUTLETS];
extern uint32_t *outlets_value_ptr[NUMBER_OF_OUTLETS];

uint16_t Log2n(uint16_t n);
int16_t isPowerOfTwo(uint16_t n);
int16_t findPosition(uint16_t n);
void check_cmd_frame();
void sm_main_extract_data();
void sm_plant_extract_data();
void flash_main_store_data_in_cache(uint32_t timestamp);
void flash_plant_store_data_in_cache(uint32_t timestamp);

void check_cmd_struct(void *param);
void Poweroutlets(int32_t* Powervalue);
void Poweroutlets_NOPRIO_new(int32_t* Powervalue);
void Poweroutlets_PRIO(int32_t *Powervalue);
void _delay_ms(uint32_t value);
void Reset();
void switch_outlet(outlets_t *outlet, uint8_t new_state);
int compare_prio(outlets_t *ptr1, outlets_t* ptr2);
int compare_values(const void *ptr1, const void *ptr2);
void sort_outlets_by_prio();
void sort_outlets_by_value();
void set_max_min_power(int32_t power);
void set_max_min_time(uint16_t time);
//void outlet_on_off(menu_t *instance);

#endif /* FUNCTIONS_H_ */

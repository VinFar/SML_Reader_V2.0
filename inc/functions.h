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
#include "nrf24.h"

#define SM_MAIN_IDX_ARRAY 0
#define SM_PLANT_IDX_ARRAY 1

#define SM_MAIN_SIZE_ARRAY 300

#define W25N_MAX_PAGE         65535
#define W25N_MAX_COLUMN_ECC           2112
#define W25N_MAX_CLOUMN				2048
#define W25N_START_OF_USER_PAGE 0x0C

#define W25N_MAX_USER_PAGE         (W25N_MAX_PAGE-W25N_START_OF_USER_PAGE)
#define W25N_MAX_COLUMN_ECC           2112
#define W25N_MAX_CLOUMN				2048

#define W25N_MAX_PAGE_MAIN ((W25N_MAX_USER_PAGE/2))
#define W25N_MAX_PAGE_PLANT ((W25N_MAX_USER_PAGE))
#define W25N_START_PAGE_MAIN 0
#define W25N_START_PAGE_PLANT (W25N_MAX_PAGE_MAIN+1)

#define W25N_MAX_ADDRESS_MAIN ((W25N_MAX_PAGE_MAIN)*W25N_MAX_CLOUMN)
#define W25N_MAX_ADDRESS_PLANT ((W25N_MAX_PAGE_PLANT*W25N_MAX_CLOUMN))
#define W25N_START_ADDRESS_MAIN 0
#define W25N_START_ADDRESS_PLANT (W25N_START_PAGE_PLANT*W25N_MAX_CLOUMN)

extern int32_t old_Powervalue;
extern int32_t tmp_PWR;

extern int32_t sm_power_main_current, sm_power_main_mean;
extern int32_t sm_power_plant_current, sm_power_plant_mean;
extern int32_t sm_consumption_main_del,sm_consumption_main_pur, sm_consumption_plant;
extern int32_t highest_prio;
extern int32_t powervalue_used_by_consumers;
extern int32_t power_value_main_max,power_value_main_min;
extern int32_t power_value_pant_max;


extern uint32_t meter_main_del,meter_main_pur,meter_plant_del;

extern uint16_t time_for_meanvalue;
extern uint8_t P_CONFIG;

extern uint8_t free_cap_main;
extern uint8_t free_cap_plant;


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

int8_t ping_cmd_handler(nrf24_frame_t *frame, void *userData);
int8_t ping_sm_data_handler(nrf24_frame_t *frame, void *userData);
int8_t ping_rtc_data_handler(nrf24_frame_t *frame, void *userData);
int8_t nrf_flash_data_handler(nrf24_frame_t *frame, void *userData);

uint32_t sm_main_value_Nseconds_past(uint16_t n);
uint32_t sm_plant_value_Nseconds_past(uint16_t n);
void sm_calc_mean();

#endif /* FUNCTIONS_H_ */

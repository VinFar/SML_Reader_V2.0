#include "main.h"
#include "FreeRTOS.h"
#include "stm32f0xx_it.h"
#include "usart.h"
#include "rtc.h"
#include "crc.h"
#include "gpio.h"
#include "stdlib.h"
#include "functions.h"
#include "eeprom.h"
#include "lcd_menu.h"

int32_t old_Powervalue;
int32_t tmp_PWR;

int32_t sm_power_main_current, sm_power_main_mean;
int32_t sm_power_plant_current, sm_power_plant_mean;
int32_t sm_consumption_main_del,sm_consumption_main_pur, sm_consumption_plant;
int32_t highest_prio;
int32_t power_value_mean_main = 0, power_value_mean_plant = 0;
int32_t power_value_main_max,power_value_main_min;
int32_t power_value_pant_max;

uint32_t meter_main_del, meter_main_pur, meter_plant_del;

int32_t sm_power_hist[2][SM_MAIN_SIZE_ARRAY];
uint16_t time_for_meanvalue=300;




uint8_t P_CONFIG;

outlets_t outlets[NUMBER_OF_OUTLETS];
uint32_t *outlets_prio_ptr[NUMBER_OF_OUTLETS];
uint32_t *outlets_value_ptr[NUMBER_OF_OUTLETS];

uint16_t Log2n(uint16_t n) {
	return (n > 1) ? 1 + Log2n(n / 2) : 0;
}

int16_t isPowerOfTwo(uint16_t n) {
	return n && (!(n & (n - 1)));
}

int16_t findPosition(uint16_t n) {
	if (!isPowerOfTwo(n))
		return -1;
	return Log2n(n) + 1;
}
void _delay_ms(uint32_t value) {

	delay_us(value * 1000);

}

void set_max_min_power(int32_t power) {
	char tmp_str[20];
	char men[20];

	if (power > eeprom_powermax.data) {
		/*
		 * new max power was recorded
		 */
		eeprom_powermax.data = power;

		/*
		 * store it in the eeprom
		 */
		eeprom_write_data_struct(&eeprom_powermax);


		flags.refreshed_rotary = 1;

	} else if (power < eeprom_powermin.data) {
		/*
		 * new min power was recorded
		 */
		eeprom_powermin.data = power;

		/*
		 * store it in the eeprom
		 */
		eeprom_write_data_struct(&eeprom_powermin);

		/*
		 * set new text in powermin submenu
		 */

		flags.refreshed_rotary = 1;

	}

}


int8_t ping_cmd_handler(nrf24_frame_t *frame, void *userData) {
	flags.smu_connected=1;
	return 0;
}
int8_t ping_sm_data_handler(nrf24_frame_t *frame, void *userData) {
	sm_power_main_current = frame->data[0].int32_data;
	sm_power_plant_current = frame->data[1].int32_data;
	sm_consumption_main_del = frame->data[2].int32_data;
	sm_consumption_main_pur = frame->data[3].int32_data;
	sm_consumption_plant = frame->data[4].int32_data;
	nrf24_tx_ctr = frame->data[5].int32_data;
	current_menu_ptr->items[menu_timer_index].on_rotate(current_menu_ptr,menu_timer_index);
	return 0;

}
int8_t ping_rtc_data_handler(nrf24_frame_t *frame, void *userData) {
}

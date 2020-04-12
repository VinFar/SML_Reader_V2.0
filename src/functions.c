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
int32_t sm_consumption_main_del, sm_consumption_main_pur, sm_consumption_plant;
int32_t highest_prio;
int32_t power_value_mean_main = 0, power_value_mean_plant = 0;
int32_t power_value_main_max, power_value_main_min;
int32_t power_value_pant_max;

uint32_t meter_main_del, meter_main_pur, meter_plant_del;

int32_t sm_power_hist[2][SM_MAIN_SIZE_ARRAY];
uint16_t time_for_meanvalue = 300;

static uint32_t flash_current_address_main_sml;
static uint32_t flash_current_address_plant_sml;

uint8_t free_cap_main;
uint8_t free_cap_plant;

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

int8_t ping_cmd_handler(nrf24_frame_t *frame, void *userData) {
	flags.smu_connected = 1;
	RTC_DISABLE_WP
	;
	RTC_INIT_WAIT
;
	uint32_t TR = frame->data[0].uint32_data;
	uint32_t DR = frame->data[1].uint32_data;
	RTC->TR = (uint32_t) (TR & RTC_TR_RESERVED_MASK);
	RTC->DR = (uint32_t) (DR & RTC_DR_RESERVED_MASK);
	RTC->ISR &= (uint32_t) ~RTC_ISR_INIT;
	RTC_ENABLE_WP;
	flash_current_address_plant_sml = frame->data[2].uint32_data;
	flash_current_address_main_sml = frame->data[3].uint32_data;
	return 0;
}

int8_t ping_sm_data_handler(nrf24_frame_t *frame, void *userData) {
	sm_power_main_current = frame->data[0].int32_data;
	sm_power_plant_current = frame->data[1].int32_data;
	sm_consumption_main_del = frame->data[2].int32_data;
	sm_consumption_main_pur = frame->data[3].int32_data;
	sm_consumption_plant = frame->data[4].int32_data;

	if (sm_power_main_current > power_value_main_max) {
		power_value_main_max = sm_power_main_current;
//		eeprom_write_data(&eeprom_powermax_main);
	}
	if (sm_power_main_current < power_value_main_min) {
		power_value_main_min = sm_power_main_current;
//		eeprom_write_data(&eeprom_powermin_main);
	}
	if (sm_power_plant_current < power_value_pant_max) {
		power_value_pant_max = sm_power_plant_current;
//		eeprom_write_data(&eeprom_powermax_plant);
	}
	if (flags.currently_in_menu == 0) {
		flags.refreshed_rotary = 1;
	}

	return 0;

}

int8_t ping_rtc_data_handler(nrf24_frame_t *frame, void *userData) {
	return 0;
}

int8_t nrf_flash_data_handler(nrf24_frame_t *frame, void *userData) {
	flash_current_address_main_sml = frame->data[0].uint32_data;
	flash_current_address_plant_sml = frame->data[1].uint32_data;

	free_cap_plant =
			(uint8_t) ((((float) flash_current_address_plant_sml
					- (float) W25N_START_ADDRESS_PLANT)
					/ ((float) W25N_MAX_ADDRESS_PLANT
							- (float) W25N_START_ADDRESS_PLANT)) * 100);

	free_cap_main = (uint8_t) ((((float) flash_current_address_main_sml)
			/ ((float) W25N_MAX_ADDRESS_MAIN - (float) W25N_START_ADDRESS_MAIN))
			* 100);

	return 0;
}

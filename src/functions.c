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
#include "delay.h"
#include "string.h"

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

static int32_t sm_power_hist[2][SM_MAIN_SIZE_ARRAY];
uint16_t time_for_meanvalue = 300;

static uint32_t flash_current_address_main_sml;
static uint32_t flash_current_address_plant_sml;

uint8_t free_cap_main;
uint8_t free_cap_plant;

static uint32_t sm_power_hist_idx_write = 0;
static uint32_t sm_power_hist_idx_oldest_value = 0;

static struct nrf24_queue_struct nrf24_queue;

static uint32_t Log2n(uint32_t n) {
	return (n > 1) ? 1 + Log2n(n / 2) : 0;
}

static int32_t findPosition(uint32_t n) {
	int32_t ret = Log2n(n) + 1;
	return ret;
}

void _delay_ms(uint32_t value) {

	delay_us(value * 1000);

}

int8_t nrf_cmd_ping_handler(nrf24_frame_t *frame, void *userData) {
	flags.smu_connected = 1;
	RTC_DISABLE_WP
	;
	RTC_INIT_WAIT
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

int8_t nrf_cmd_sm_data_handler(nrf24_frame_t *frame, void *userData) {
	sm_power_main_current = frame->data[0].int32_data;
	sm_power_plant_current = frame->data[1].int32_data;
	sm_consumption_main_del = frame->data[2].int32_data;
	sm_consumption_main_pur = frame->data[3].int32_data;
	sm_consumption_plant = frame->data[4].int32_data;
	timer_ctr_for_power_valid_timeout = 0;

	if (flags.currently_in_menu == 0) {
		flags.refreshed_rotary = 1;
	}

	return 0;

}

int8_t nrf_cmd_rtc_data_handler(nrf24_frame_t *frame, void *userData) {
	return 0;
}

int8_t nrf_cmd_flash_data_handler(nrf24_frame_t *frame, void *userData) {
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

int8_t nrf_cmd_relay_handler(nrf24_frame_t *frame, void *userData){

	if(frame->data[0].int32_data){
		relay_left_off();
	}
	return 0;
}

uint32_t sm_main_value_Nseconds_past(uint16_t n) {
	int16_t idx_last = (sm_power_hist_idx_write - 1);
	if (idx_last < 0) {
		idx_last = SM_MAIN_SIZE_ARRAY - 1;
	}

	int16_t idx = (idx_last) - n;

	if (idx < 0) {

		idx = SM_MAIN_SIZE_ARRAY - n;
	}
	return sm_power_hist[SM_MAIN_IDX_ARRAY][idx];
}

uint32_t sm_plant_value_Nseconds_past(uint16_t n) {
	int16_t idx_last = (sm_power_hist_idx_write - 1);
	if (idx_last < 0) {
		idx_last = SM_MAIN_SIZE_ARRAY - 1;
	}

	int16_t idx = (idx_last) - n;

	if (idx < 0) {

		idx = SM_MAIN_SIZE_ARRAY - n;
	}
	return sm_power_hist[SM_PLANT_IDX_ARRAY][idx];
}


void sm_calc_mean() {

	/*
	 * write powervalue into history
	 */
	sm_power_hist[SM_MAIN_IDX_ARRAY][sm_power_hist_idx_write] =
			sm_power_main_current;
	sm_power_hist[SM_PLANT_IDX_ARRAY][sm_power_hist_idx_write] =
			sm_power_plant_current;

	/*
	 * we have a history of 300 values, so catch overflow
	 */

	if (sm_power_hist_idx_write++ == ARRAY_LENGTH(sm_power_hist[0])) {
		sm_power_hist_idx_write = 0;
	}

	if (sm_power_hist_idx_oldest_value == sm_power_hist_idx_write) {
		sm_power_hist_idx_oldest_value++;
		if (sm_power_hist_idx_oldest_value == ARRAY_LENGTH(sm_power_hist[0])) {
			sm_power_hist_idx_oldest_value = 0;
		}
	}

	int32_t sm_power_sum_main = 0;
	int32_t sm_power_sum_plant = 0;
	/*
	 * we have to add all values that are inside the period specified by SECONDS_FOR_MEAN_VALUE
	 * From the newest value down to the period, so we have to decrement the idx.
	 * The user can configure the time period over which the mean value
	 * will be calculated (time_for_meanvalue). So abort on reaching this value
	 */
	int16_t idx_read = sm_power_hist_idx_write - 1;
	if (idx_read < 0) {
		idx_read = 0;
	}
	uint32_t ctr;
	for (ctr = 0; ctr < time_for_meanvalue; ctr++) {

		/*
		 * add all values
		 */
		sm_power_sum_main += sm_power_hist[SM_MAIN_IDX_ARRAY][idx_read];
		sm_power_sum_plant += sm_power_hist[SM_PLANT_IDX_ARRAY][idx_read];

		/*
		 * catch overflow of index and decrement idx
		 */

		if (idx_read == sm_power_hist_idx_oldest_value) {
			/*
			 * at the beginning there will be not enough time stamps in the array
			 * to reach the sum of time_for_meanvalue, so catch index on which we began.
			 * if this is the case take the maximum passed time and use this for the mean value
			 */
			break;
		}
		idx_read--;
		if (idx_read < 0) {
			idx_read = 0;
		}
	}
	sm_power_main_mean = (int32_t) (sm_power_sum_main / (int32_t) (ctr));
	sm_power_plant_mean = sm_power_sum_plant / (int32_t) (ctr);

	/*
	 * end of mean value calculation
	 */

}

int8_t relay_left_on() {
	if (flags.power_valid_timeout) {
		RELAY_LEFT_OFF;
		return -1;
	}
	RELAY_LEFT_ON;
	return 0;
}

int8_t relay_left_off() {
	RELAY_LEFT_OFF;
	return 0;
}

int8_t relay_right_on() {
	if (flags.power_valid_timeout) {
		RELAY_RIGHT_OFF;
		return -1;
	}
	RELAY_RIGHT_ON;
	return 0;
}

int8_t relay_right_off() {
	RELAY_RIGHT_OFF;
	return 0;
}

int8_t relay_left_get_state() {
	return ((RELAY_LEFT_GPIO_PORT->ODR & RELAY_LEFT_PIN)
			>> (findPosition(RELAY_LEFT_PIN) - 1));
}

int8_t relay_right_get_state() {
	return ((RELAY_RIGHT_GPIO_PORT->ODR & RELAY_RIGHT_PIN)
			>> (findPosition(RELAY_RIGHT_PIN) - 1));
}

void nrf_queue_init() {
	nrf24_queue.read_idx = 0;
	nrf24_queue.write_idx = 0;
}

enum enqueue_result nrf_queue_enqueue(nrf24_frame_queue_t *p_new_item) {
	uint16_t elements_in = nrf24_queue.write_idx - nrf24_queue.read_idx;

	size_t const capacity = ARRAY_LENGTH(nrf24_queue.items);
	if (elements_in == capacity) {
		return ENQUEUE_RESULT_FULL;
	}

	uint16_t i = (nrf24_queue.write_idx)++ & (capacity - 1);

	memcpy(&nrf24_queue.items[i], p_new_item, sizeof(nrf24_frame_t));

	return ENQUEUE_RESULT_SUCCESS;
}

enum dequeue_result nrf_queue_dequeue(nrf24_frame_queue_t *p_item_out) {
	uint16_t elements_in = nrf24_queue.write_idx - nrf24_queue.read_idx;
	size_t const capacity = ARRAY_LENGTH(nrf24_queue.items);

	if (elements_in == 0) {
		return DEQUEUE_RESULT_EMPTY;
	}

	uint16_t i = (nrf24_queue.read_idx)++ & (capacity - 1);
	memcpy(p_item_out, &nrf24_queue.items[i], sizeof(nrf24_frame_t));

	return DEQUEUE_RESULT_SUCCESS;
}

uint8_t nrf_queue_is_empty() {
	return ((nrf24_queue.write_idx - nrf24_queue.read_idx) == 0);
}

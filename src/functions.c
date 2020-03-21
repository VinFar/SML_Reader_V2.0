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

int32_t powervalue_current, powervalue_mean, powervalue_used_by_consumers = 0;
int32_t total_consumption, total_money;
int32_t highest_prio;
int32_t powervalue_no_mean = 0;

int32_t power_value_history[300];
uint16_t time_history[300]; //max 60 second history
uint16_t time_mean = 0;
uint16_t milli_seconds_passsed = 0;

uint16_t seconds_for_meanvalue = 60;
int32_t menu_timer_index = 1000000;

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

void check_cmd_frame() {
	if (flags.usart6_new_cmd == 1) {
		flags.usart6_new_cmd = 0;

		/*
		 * A new command frame was received from the Host PC and has to be evaluated.
		 */
		/*
		 * depending on the received amounts of bytes, it may be that a last CRC calculation wiht filled zeros
		 * has to be calculated
		 */
		uint8_t err = 0;
		uint8_t *del_ptr;
		del_ptr = (uint8_t*) &usart6_cmd_frame.size
				+ (usart6_cmd_frame.size - 5);
		uint8_t size = usart6_cmd_frame.size;
		uint8_t cmd = usart6_cmd_frame.cmd;
		if (size < CMD_FRAME_MIN_SIZE) {
			/*
			 * size is not valid
			 */
			err = 1;
		} else {
			if (size > CMD_FRAME_MAX_SIZE) {
				/*
				 * error size is too big
				 */

				err = 1;
			} else {

				if (*del_ptr != FRAME_DELIMITER) {
					/*
					 * delimiter is not valid
					 */

					err = 1;
				} else {
					if (cmd >= MAX_ENUM_CMDS) {
						/*
						 * command is not valid
						 */
						err = 1;

					} else {
//						uint32_t crc_check =
//								*(uint32_t*) ((uint8_t*) &usart6_cmd_frame
//										+ (usart6_cmd_frame.size - 4));
//						uint32_t crc_result = crc32_calc(
//								(uint8_t*) &usart6_cmd_frame,
//								usart6_cmd_frame.size - 4);
						NOP
						NOP
						if (0) {
							/*
							 * CRC32 Check is not valid
							 */

							err = 1;
						} else {
						}
					}
				}

			}
		}
		usart6_ack_frame.size = ACK_FRAME_MIN_SIZE;
		usart6_ack_frame.cmd = usart6_cmd_frame.cmd;
		uint16_t data_size = 0;
		if (!err) {
			usart6_ack_frame.ack = CMD_ACK;
			switch (usart6_cmd_frame.cmd) {
			case CMD_PING:
				flags.gateway = 0;
				NOP
				break;
			case CMD_READ_UUID:
				memcpy(usart6_ack_frame.data, &uuid, sizeof(uuid));
				data_size = sizeof(uuid);
				break;

			case CMD_SET_RTC:
				NOP
				uint32_t TR = 0, DR = 0;
				RTC_DISABLE_WP
				;
				RTC_INIT_WAIT
;				TR = usart6_cmd_frame.data[0].uint32_data;
				DR = usart6_cmd_frame.data[1].uint32_data;
				RTC->TR = (uint32_t)(TR & RTC_TR_RESERVED_MASK);
				RTC->DR = (uint32_t)(DR & RTC_DR_RESERVED_MASK);
				RTC->ISR &= (uint32_t)~RTC_ISR_INIT;
				RTC_ENABLE_WP;
				break;
				case CMD_GET_UNIX_TIME:
				RTC_GetTime(RTC_FORMAT_BIN, &sm_time);
				RTC_GetDate(RTC_FORMAT_BIN, &sm_date);
				usart6_ack_frame.data[0].uint32_data = rtc_get_unix_time(&sm_time,
						&sm_date);
				data_size = 4;
				break;
				default:
				usart6_ack_frame.ack = CMD_NACK;
				NOP
				NOP
				break;
			}

			/*
			 * fill CRC with dummy
			 */
		} else {
			usart6_ack_frame.ack = CMD_NACK;
		}

		usart6_ack_frame.size = ACK_FRAME_MIN_SIZE + data_size;
		(((uint8_t*) (&usart6_ack_frame))[usart6_ack_frame.size - 5]) =
		FRAME_DELIMITER;
		usart6_cmd_frame_ptr = (uint8_t*) &usart6_cmd_frame;
		usart6_rx_ctr = 0;
		flags.usart6_new_cmd = 0;
		USART6->CR1 |= USART_CR1_RE;
		USART6->CR1 |= USART_CR1_RXNEIE;
		usart6_ack_frame.cmd = usart6_cmd_frame.cmd;
		memset(&usart6_cmd_frame, 0, sizeof(usart6_cmd_frame));
		usart6_send_ack_frame(&usart6_ack_frame);

	}
}

/*
 *
 * This Function selects the correct Method for switching the Power Outlets.
 *
 *
 */
void Poweroutlets(int32_t *Powervalue) {

	switch (P_CONFIG) {
	case 1:
	case 2:
		Poweroutlets_NOPRIO_new(Powervalue);
		return;
	default:
		Poweroutlets_NOPRIO_new(Powervalue);
		return;
	}

}

/*
 * this function switches the outlet depending on their power value stored in the structure.
 *
 */
void Poweroutlets_NOPRIO_new(int32_t *Powervalue) {

	/*
	 * The most effective way this system could work if, when no power is buyed from or selled to the grid.
	 * So this means the current power is always 0(without considering MAX_CONSUMED_PWR)!
	 * Since this is not possible, cause we can't control the power the consumers are
	 * consuming this will always have a deviation to 0.
	 *
	 * This function expects, that the array of outlets is sorted by their values.
	 * From high to low. This has to be, cause we want to switch on the big consumers first,
	 * and then for optimizing in little ranges the little consumers.
	 */

	int32_t current_power = *Powervalue;
	/*
	 * MAX_CONSUMED_PWR defines how many power can be bought from grid. This means if this value is 100W
	 * than we can buy a maximum power of 100W from the grid. This value has to be added (substracted)
	 * from the current power value
	 */
	current_power -= MAX_CONSUMED_PWR;

	/*
	 * at first we have to check, whether it is neccessary to switch off one
	 * of the outlets.
	 * This is done the following loop:
	 * If current_power (power selled (negative) or buyed(positive)) is buyed from the grid,
	 * thus a positive value, we have to switch off at least one outlet.
	 * If this is done, this has to be checked again.
	 */

	uint8_t i = 0;

	while ((current_power > 0) && (i < NUMBER_OF_OUTLETS)) {

		if (((outlets_t*) outlets_value_ptr[i])->state == 1) {
			switch_outlet(((outlets_t*) outlets_value_ptr[i]), 0);
			current_power -=
					((outlets_t*) outlets_value_ptr[i])->union_value.value;
		}
		i++;
	}
	/*
	 * at this point no more power is buyed from the grid, so eventually we have to switch on
	 * some outlet, because maybe the selled power is greater than one of outlets values
	 */

	for (int i = 0; i < NUMBER_OF_OUTLETS; i++) {
		if (current_power
				< -(((outlets_t*) outlets_value_ptr[i])->union_value.value)) {
			/*
			 * this outlet has a consumer attached to it that can be switched without
			 * buying too much power from the grid.
			 * So if this outlet has value of 1200W and we are selling 1300W (-1300W as integer)
			 * + 100W MAX CONSUMED PWR (1400W), we can switch this outlet on.
			 * Than we have to add the power value of the outlet to the current power.
			 * This is dann inside function 'switch_outlet'
			 */
			if (((outlets_t*) outlets_value_ptr[i])->state == 0) {
				/*
				 * this outlet is switched off, so switch it on.
				 * Else do nothing
				 */
				switch_outlet(((outlets_t*) outlets_value_ptr[i]), 1);
				current_power +=
						((outlets_t*) outlets_value_ptr[i])->union_value.value;
			}
		}
	}

}

/*
 * this function sets the outlet with the highest prio first. then the one with second highest prio and so on.
 * If the available power is lower than the value from the outlet with the highest prio it switches the second highest prio on.
 * and so on...
 * this is less efficient than without the prios
 *
 * ToDo: Reset tmp_PWR and old_Powervalue when the user changes P_Config
 * ToDo: Write a function to determine sort the outlets by its prio values. from 0(highest) to lowest
 *
 */

void Poweroutlets_PRIO(int32_t *Powervalue) {

	tmp_PWR = *Powervalue;
	tmp_PWR -= MAX_CONSUMED_PWR; //Substract(add) max_consumed_pwr to new value
	tmp_PWR -= old_Powervalue;		//Substract stored value from new value
	old_Powervalue = 0;	//tmp_PWR is the value which is avaiable at this moment
	uint8_t i;

	i = 1;
	while ((tmp_PWR > MAX_CONSUMED_PWR) && (i <= NUMBER_OF_OUTLETS)) {//switch outlets off, if tmp_PWR is

		switch_outlet(&outlets[i - 1], 0);
		tmp_PWR += outlets[i - 1].union_value.value;
		i++;
	}

	return;
}

/*
 *
 * Very very Bad delay function
 *
 */

void _delay_ms(uint32_t value) {

	delay_us(value * 1000);

}

void Reset() {

	return;
}

/*
 *
 * function for switching the outlets and/or locking them in one state
 * index: the arrayindex of the struct from the outlet
 * new_state: new state of the requested outlet
 * the outlet is not switched if the lock bit of the outlet is set
 *
 */

void switch_outlet(outlets_t *outlet, uint8_t new_state) {

	if (outlet->lock)
		return;

	if (new_state == outlet->state) {
		/*
		 * this outlet is already in this state, so it would be pointless to switch them
		 * Also this woudl cause an error, due to the fact that the value
		 * woudl be substracted or added once again to the value from the grid
		 */
		return;
	}

	if (new_state == 1) { //switch on
		tmp_PWR += outlet->union_value.value;
		powervalue_used_by_consumers += outlet->union_value.value;

		outlet->state = 1;
		outlet->BANK->BSRR = outlet->PORT; //Set set-bit in bsrr register
	} else {
		outlet->BANK->BRR = outlet->PORT;
		outlet->state = 0;
		powervalue_used_by_consumers -= outlet->union_value.value;

	}
	return;
}

/*
 *
 * compare function for sorting outlets structe by their prios
 * needed for qsort
 *
 */

int compare_prio(outlets_t *ptr1, outlets_t *ptr2) {

	if (ptr1->prio < ptr2->prio)
		return -1;
	else if (ptr1->prio > ptr2->prio)
		return 1;
	else
		return 0;

}

/*
 *
 * compare function for sorting outlets structe by their values
 * needed for qsort
 *
 */

int compare_values(const void *ptr1, const void *ptr2) {

	uint32_t o1_address = *((uint32_t*) ptr1);
	uint32_t o2_address = *((uint32_t*) ptr2);

	outlets_t *o1 = o1_address;
	outlets_t *o2 = o2_address;

	if (o1->union_value.value < o2->union_value.value) {
		return 1;
	} else if (o1->union_value.value > o2->union_value.value) {
		return -1;
	} else {
		return 0;
	}

	return 0;
}

/*
 *
 * This function sorts the list of outlets by their prios. from 0 (highest) to n (lowest)
 *
 */

void sort_outlets_by_prio() {

	qsort((void*) outlets_prio_ptr, NUMBER_OF_OUTLETS, sizeof(struct outlets),
			(__compar_fn_t ) compare_prio);
	return;
}

/*
 *
 * this functions sorts the reference list by value by their values. from lowest to highest value
 *
 */

void sort_outlets_by_value() {

	qsort((void*) outlets_value_ptr, NUMBER_OF_OUTLETS, 4,
			(__compar_fn_t ) compare_values);

	return;
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

		/*
		 * set new text in powermax submenu
		 */
		memset(men, 0, sizeof(men));
		strcpy(men, "Power:");
		itoa(eeprom_powermax.data, tmp_str, 10);
		strcat(men, tmp_str);
		strcat(men, "W");
		menu_init_text(&maxima_items[1], men);
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
		memset(men, 0, sizeof(men));
		strcpy(men, "Power:");
		itoa(eeprom_powermin.data, tmp_str, 10);
		strcat(men, tmp_str);
		strcat(men, "W");
		menu_init_text(&minima_items[1], men);
		flags.refreshed_rotary = 1;

	}

}

void set_max_min_time(uint16_t time) {
	char tmp_str[20];
	char men[20];

	if (time > eeprom_timemax.data) {
		/*
		 * new max time was recorded
		 */
		eeprom_timemax.data = time;

		/*
		 * store it in the eeprom
		 */
		eeprom_write_data_struct(&eeprom_timemax);
		&eeprom_timemax.byte;

		/*
		 * set new text in the timemax submenu
		 */
		memset(men, 0, sizeof(men));
		strcpy(men, "Time:");
		itoa(eeprom_timemax.data, tmp_str, 10);
		strcat(men, tmp_str);
		strcat(men, "ms");
		menu_init_text(&maxima_items[2], men);
		flags.refreshed_rotary = 1;

	} else if (time < eeprom_timemin.data && time > 0) {
		/*
		 * new min time was recorded
		 */
		eeprom_timemin.data = time;

		/*
		 * store it in the eeprom
		 */
		eeprom_write_data_struct(&eeprom_timemin);

		/*
		 * set new text in timemin submenu
		 */
		memset(men, 0, sizeof(men));
		strcpy(men, "Time:");
		itoa(eeprom_timemin.data, tmp_str, 10);
		strcat(men, tmp_str);
		strcat(men, "ms");
		menu_init_text(&minima_items[2], men);
		flags.refreshed_rotary = 1;

	}

}




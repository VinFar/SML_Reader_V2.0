#include "main.h"
#include "FreeRTOS.h"
#include "stm32f0xx_it.h"
#include "usart.h"
#include "flash.h"
#include "rtc.h"
#include "crc.h"
#include "gpio.h"

int32_t old_main_power = 0;
int32_t old_plant_power = 0;

/*
 *	sm_current_data is the current valid data of the smart meter
 */
smartmeter_data_t sm_main_current_data;
smartmeter_data_t sm_plant_current_data;

/*
 * sm_flash_cache_data is a array of smartmter_data_t surrounded by two delimiters
 * this struct is used to cache the smart meter data in order to just have one
 * write acces to the flash per page.
 * The entire struct is written to the flash when it is full.
 */
smartmeter_flash_data_t sm_flash_main_cache_data[W25N_MAX_CLOUMN
		/ sizeof(smartmeter_flash_data_t)];
smartmeter_flash_data_t sm_flash_plant_cache_data[W25N_MAX_CLOUMN
		/ sizeof(smartmeter_flash_data_t)];

uint8_t sm_idx_for_main_cache_data = 0;
uint8_t sm_idx_for_plant_cache_data = 0;
uint32_t flash_current_address_main_sml = 0;
uint32_t flash_current_address_plant_sml = W25N_START_ADDRESS_PLANT;

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
			case CMD_READ_FLASH_ADDRESS_MAIN:
				usart6_ack_frame.data[0].uint32_data =
						flash_current_address_main_sml;
				usart6_ack_frame.data[1].uint32_data = W25N_MAX_ADDRESS_MAIN;
				usart6_ack_frame.data[2].uint32_data = W25N_START_ADDRESS_MAIN;
				data_size = 12;
				break;
			case CMD_READ_FLASH_ADDRESS_PLANT:
				usart6_ack_frame.data[0].uint32_data =
						flash_current_address_plant_sml;
				usart6_ack_frame.data[1].uint32_data = W25N_MAX_ADDRESS_PLANT;
				usart6_ack_frame.data[2].uint32_data = W25N_START_ADDRESS_PLANT;
				data_size = 12;
				break;
			case CMD_READ_FLASH:
				flash_read_data(usart6_cmd_frame.data[0].uint32_data,
						usart6_ack_frame.data,
						usart6_cmd_frame.major_cmd.uint16_data);
				data_size += usart6_cmd_frame.major_cmd.uint16_data;
				break;
			case CMD_READ_MAIN_POWER:
				usart6_ack_frame.data[0].uint32_data =
						sm_main_current_data.power;
				data_size = 4;
				break;
			case CMD_READ_PLANT_POWER:
				usart6_ack_frame.data[0].uint32_data =
						sm_plant_current_data.power;
				data_size = 4;
				break;
			case CMD_READ_UUID:
				memcpy(usart6_ack_frame.data, &uuid, sizeof(uuid));
				data_size = sizeof(uuid);
				break;
			case CMD_DO_BULK_ERASE:
				if (usart6_cmd_frame.major_cmd.uint16_data == 0xaffe) {
					if (usart6_cmd_frame.minor_cmd.uint16_data == 0xdead) {
						if (usart6_cmd_frame.data[0].uint32_data
								== 0xbadeaffe) {
							flash_bulkErase();
							flash_current_address_main_sml = 0;
							flash_current_address_plant_sml = W25N_START_ADDRESS_PLANT;
						}
					}
				}
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

void sm_plant_extract_data() {

	uint8_t *needle_ptr;
	uint8_t needle[20] = { 0 };

	if (sml_plant_raw_data_idx > sizeof(sml_plant_raw_data)
			|| sml_plant_raw_data_idx < 200) {

		sml_plant_raw_data_idx = 0;
	} else {
		uint16_t crc_calc = ccrc16((char*) sml_plant_raw_data,
				sml_plant_raw_data_idx - 2);
		uint16_t crc_check =
				((uint16_t) sml_plant_raw_data[sml_plant_raw_data_idx - 2] << 8)
						+ (uint16_t) sml_plant_raw_data[sml_plant_raw_data_idx
								- 1];

		if (crc_check != crc_calc) {

			sml_plant_raw_data_idx = 0;

		} else {

			sml_plant_raw_data_idx = 0;

			needle[0] = 0x07;
			needle[1] = 0x01;
			needle[2] = 0x00;
			needle[3] = 0x10;
			needle[4] = 0x07;
			needle[5] = 0x00;
			needle[6] = 0xff;	//Set String for Powervalue key

			if ((needle_ptr = (uint8_t*) memmem(sml_plant_raw_data,
					sizeof(sml_plant_raw_data), needle, 7)) == NULL) {
				//No Active Power String detected, ERror

				return;
			} else {
				float consumption_tmp = 0;
				int32_t tmp_value = (needle_ptr[14] << 24)
						+ (needle_ptr[15] << 16) + (needle_ptr[16] << 8)
						+ (needle_ptr[18]);	//Extract and calculate Powervalue
				sm_plant_current_data.power = tmp_value / 10;
				/*
				 * powervalue_buying_from_grid is only for testing and represent the
				 * attached consumers, if they actually connected!
				 * This value means, that this power is used by the connected consumers so we have to add it
				 * to the current power value
				 */
				//						powervalue_current += powervalue_used_by_consumers;
				/*
				 * calculate a millisecond timer without the need of a 1khz interrupt by using
				 * the cnt regiser of Timer 6
				 */

				/*
				 * end of mean value calculation
				 */

				needle[0] = 0x07;
				needle[1] = 0x01;
				needle[2] = 0x00;
				needle[3] = 0x02;
				needle[4] = 0x08;
				needle[5] = 0x01;
				needle[6] = 0xff;	//Set String for consumption fare 1

				if ((needle_ptr = (unsigned char*) memmem(sml_plant_raw_data,
						sizeof(sml_plant_raw_data), needle, 7)) == NULL) {
					return;
				} else {

					sm_plant_current_data.meter_delivery = ((needle_ptr[15]
							<< 24) + (needle_ptr[16] << 16)
							+ (needle_ptr[17] << 8) + (needle_ptr[18])) / 10000;

				}

				/*
				 * at this point all data from the smart meter was written
				 * to the struct.
				 */
				LED_ERROR2_TOGGLE;

			}
		}
	}

}

void sm_main_extract_data() {

	uint8_t *needle_ptr;
	uint8_t needle[20] = { 0 };

	if (sml_main_raw_data_idx > sizeof(sml_main_raw_data)
			|| sml_main_raw_data_idx < 200) {

		sml_main_raw_data_idx = 0;

	} else {
		uint16_t crc_calc = ccrc16((char*) sml_main_raw_data,
				sml_main_raw_data_idx - 2);
		uint16_t crc_check = ((uint16_t) sml_main_raw_data[sml_main_raw_data_idx
				- 2] << 8)
				+ (uint16_t) sml_main_raw_data[sml_main_raw_data_idx - 1];

		if (crc_check == crc_calc) {

			sml_main_raw_data_idx = 0;

			needle[0] = 0x07;
			needle[1] = 0x01;
			needle[2] = 0x00;
			needle[3] = 0x10;
			needle[4] = 0x07;
			needle[5] = 0x00;
			needle[6] = 0xff;	//Set String for Powervalue key

			if ((needle_ptr = (uint8_t*) memmem(sml_main_raw_data,
					sizeof(sml_main_raw_data), needle, 7)) == NULL) {
				//No Active Power String detected, ERror

				return;
			} else {
				int32_t tmp_value = (needle_ptr[14] << 24)
						+ (needle_ptr[15] << 16) + (needle_ptr[16] << 8)
						+ (needle_ptr[18]);	//Extract and calculate Powervalue
				sm_main_current_data.power = tmp_value / 10;
				/*
				 * powervalue_buying_from_grid is only for testing and represent the
				 * attached consumers, if they actually connected!
				 * This value means, that this power is used by the connected consumers so we have to add it
				 * to the current power value
				 */
				//						powervalue_current += powervalue_used_by_consumers;
				/*
				 * calculate a millisecond timer without the need of a 1khz interrupt by using
				 * the cnt regiser of Timer 6
				 */

				/*
				 * end of mean value calculation
				 */
				needle[0] = 0x07;
				needle[1] = 0x01;
				needle[2] = 0x00;
				needle[3] = 0x01;
				needle[4] = 0x08;
				needle[5] = 0x01;
				needle[6] = 0xff;	//Set String for consumption fare 1

				if ((needle_ptr = (unsigned char*) memmem(sml_main_raw_data,
						sizeof(sml_main_raw_data), needle, 7)) == NULL) {
					return;
				} else {

					sm_main_current_data.meter_purchase =
							((needle_ptr[15] << 24) + (needle_ptr[16] << 16)
									+ (needle_ptr[17] << 8) + (needle_ptr[18]))
									/ 10000;

				}

				needle[0] = 0x07;
				needle[1] = 0x01;
				needle[2] = 0x00;
				needle[3] = 0x02;
				needle[4] = 0x08;
				needle[5] = 0x01;
				needle[6] = 0xff;	//Set String for consumption fare 1

				if ((needle_ptr = (unsigned char*) memmem(sml_main_raw_data,
						sizeof(sml_main_raw_data), needle, 7)) == NULL) {
					return;
				} else {

					sm_main_current_data.meter_delivery =
							((needle_ptr[15] << 24) + (needle_ptr[16] << 16)
									+ (needle_ptr[17] << 8) + (needle_ptr[18]))
									/ 10000;

				}

				/*
				 * at this point all data from the smart meter was written
				 * to the struct
				 */


			}
		}
	}

}

void flash_main_store_data_in_cache(uint32_t timestamp) {
	/*
	 * this function gets called every 2 second
	 * from the main loop to store the data to the
	 * cache/flash
	 */

	sm_flash_main_cache_data[sm_idx_for_main_cache_data].data =
			sm_main_current_data;
	sm_flash_main_cache_data[sm_idx_for_main_cache_data].begin =
	BEGIN_DELIMITER;
	sm_flash_main_cache_data[sm_idx_for_main_cache_data].delimiter =
	END_DELIMITER;
	sm_flash_main_cache_data[sm_idx_for_main_cache_data].data.uptime =
			timestamp;

	sm_idx_for_main_cache_data++;

	/*
	 * if the cache is full, write it to the flash
	 */
	if (sm_idx_for_main_cache_data
			> ((W25N_MAX_CLOUMN / sizeof(smartmeter_flash_data_t)) - 1)) {
		/*
		 * cache is full, so write it to the flash
		 * init the cache with 0 and set the correct packet ctr
		 */

		flash_write_data(flash_current_address_main_sml,
				sm_flash_main_cache_data, sizeof(sm_flash_main_cache_data));
		flash_current_address_main_sml +=
		W25N_MAX_CLOUMN;
		sm_idx_for_main_cache_data = 0;

		if (flash_current_address_main_sml > W25N_MAX_ADDRESS_MAIN) {
			/*
			 * if we reached the end of the flash memory,
			 * start at the beginning again.
			 */
			flash_current_address_main_sml = 0;
		}

	}

}

void flash_plant_store_data_in_cache(uint32_t timestamp) {
	/*
	 * this function gets called every 2 second from
	 * the main loop to the data to the cache/flash
	 */

	sm_flash_plant_cache_data[sm_idx_for_plant_cache_data].data =
			sm_plant_current_data;
	sm_flash_plant_cache_data[sm_idx_for_plant_cache_data].begin =
	BEGIN_DELIMITER;
	sm_flash_plant_cache_data[sm_idx_for_plant_cache_data].delimiter =
	END_DELIMITER;
	sm_flash_plant_cache_data[sm_idx_for_plant_cache_data].data.uptime =
			timestamp;

	sm_idx_for_plant_cache_data++;

	/*
	 * if the cache is full, write it to the flash
	 */
	if (sm_idx_for_plant_cache_data
			> ((W25N_MAX_CLOUMN / sizeof(smartmeter_flash_data_t)) - 1)) {
		/*
		 * cache is full, so write it to the flash
		 * init the cache with 0 and set the correct packet ctr
		 */

		flash_write_data(flash_current_address_plant_sml,
				sm_flash_plant_cache_data, sizeof(sm_flash_plant_cache_data));
		flash_current_address_plant_sml +=
		W25N_MAX_CLOUMN;
		sm_idx_for_plant_cache_data = 0;
		if (flash_current_address_plant_sml > W25N_MAX_ADDRESS_PLANT) {
			/*
			 * if we reached the end of the flash memory,
			 * start at the beginning again.
			 */
			flash_current_address_plant_sml = 0;
		}

	}

}

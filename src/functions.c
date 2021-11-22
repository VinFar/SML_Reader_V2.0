#include "main.h"
#include "FreeRTOS.h"
#include "stm32f0xx_it.h"
#include "usart.h"
#include "flash.h"
#include "rtc.h"
#include "crc.h"
#include "gpio.h"
#include "functions.h"
#include "string.h"

static struct nrf24_queue_struct nrf24_queue;

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

int32_t sm_main_hist[SM_HISTORY_SIZE], sm_plant_hist[SM_HISTORY_SIZE];
static int32_t sm_main_hist_idx = 0, sm_plant_hist_idx = 0;

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
							flash_current_address_plant_sml =
							W25N_START_ADDRESS_PLANT;
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
				;
				TR = usart6_cmd_frame.data[0].uint32_data;
				DR = usart6_cmd_frame.data[1].uint32_data;
				RTC->TR = (uint32_t) (TR & RTC_TR_RESERVED_MASK);
				RTC->DR = (uint32_t) (DR & RTC_DR_RESERVED_MASK);
				RTC->ISR &= (uint32_t) ~RTC_ISR_INIT;
				RTC_ENABLE_WP;
				break;
			case CMD_GET_UNIX_TIME:
				RTC_GetTime(RTC_FORMAT_BIN, &sm_time);
				RTC_GetDate(RTC_FORMAT_BIN, &sm_date);
				usart6_ack_frame.data[0].uint32_data = rtc_get_unix_time(
						&sm_time, &sm_date);
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

				sm_plant_hist[sm_plant_hist_idx] = sm_plant_current_data.power;
				if (++sm_plant_hist_idx > SM_HISTORY_SIZE) {
					sm_plant_hist_idx = 0;
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
		sml_main_raw_data_idx = 0;

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

				sm_main_hist[sm_main_hist_idx] = sm_main_current_data.power;
				if (++sm_main_hist_idx > SM_HISTORY_SIZE) {
					sm_main_hist_idx = 0;
				}

				/*
				 * at this point all data from the smart meter was written
				 * to the struct
				 */


			}
		}
	}

	memset(sml_main_raw_data, 0, sizeof(sml_main_raw_data));

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

		union data_union sm[6];
		sm[0].uint32_data = flash_current_address_main_sml;
		sm[1].uint32_data = flash_current_address_plant_sml;
		sm[2].uint32_data = W25N_MAX_ADDRESS_MAIN;
		sm[3].uint32_data = W25N_MAX_ADDRESS_PLANT;
		sm[4].uint32_data = W25N_START_ADDRESS_MAIN;
		sm[5].uint32_data = W25N_START_ADDRESS_PLANT;

		nrf_add_qeue(NRF24_CMD_FLASH_DATA, sm, NRF_ADDR_DISP, NRF_DISP_PIPE);

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

		union data_union sm[6];
		sm[0].uint32_data = flash_current_address_main_sml;
		sm[1].uint32_data = flash_current_address_plant_sml;
		sm[2].uint32_data = W25N_MAX_ADDRESS_MAIN;
		sm[3].uint32_data = W25N_MAX_ADDRESS_PLANT;
		sm[4].uint32_data = W25N_START_ADDRESS_MAIN;
		sm[5].uint32_data = W25N_START_ADDRESS_PLANT;

		nrf_add_qeue(NRF24_CMD_FLASH_DATA, sm, NRF_ADDR_DISP, NRF_DISP_PIPE);

		if (flash_current_address_plant_sml > W25N_MAX_ADDRESS_PLANT) {
			/*
			 * if we reached the end of the flash memory,
			 * start at the beginning again.
			 */
			flash_current_address_plant_sml = 0;
		}

	}

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

static uint8_t sml_tx_data[395];

void init_tx_data() {
	sml_tx_data[0] = 0x1B;
	sml_tx_data[1] = 0x1B;
	sml_tx_data[2] = 0x1B;
	sml_tx_data[3] = 0x1B;
	sml_tx_data[4] = 0x01;
	sml_tx_data[5] = 0x01;
	sml_tx_data[6] = 0x01;
	sml_tx_data[7] = 0x01;
	sml_tx_data[8] = 0x76;
	sml_tx_data[9] = 0x07;
	sml_tx_data[10] = 0x00;
	sml_tx_data[11] = 0x0E;
	sml_tx_data[12] = 0x03;
	sml_tx_data[13] = 0xAD;
	sml_tx_data[14] = 0x1E;
	sml_tx_data[15] = 0x67;
	sml_tx_data[16] = 0x62;
	sml_tx_data[17] = 0x00;
	sml_tx_data[18] = 0x62;
	sml_tx_data[19] = 0x00;
	sml_tx_data[20] = 0x72;
	sml_tx_data[21] = 0x63;
	sml_tx_data[22] = 0x01;
	sml_tx_data[23] = 0x01;
	sml_tx_data[24] = 0x76;
	sml_tx_data[25] = 0x01;
	sml_tx_data[26] = 0x01;
	sml_tx_data[27] = 0x07;
	sml_tx_data[28] = 0x00;
	sml_tx_data[29] = 0x0E;
	sml_tx_data[30] = 0x07;
	sml_tx_data[31] = 0xA7;
	sml_tx_data[32] = 0xB4;
	sml_tx_data[33] = 0xCD;
	sml_tx_data[34] = 0x0B;
	sml_tx_data[35] = 0x09;
	sml_tx_data[36] = 0x01;
	sml_tx_data[37] = 0x45;
	sml_tx_data[38] = 0x4D;
	sml_tx_data[39] = 0x48;
	sml_tx_data[40] = 0x00;
	sml_tx_data[41] = 0x00;
	sml_tx_data[42] = 0x41;
	sml_tx_data[43] = 0xA7;
	sml_tx_data[44] = 0x9F;
	sml_tx_data[45] = 0x01;
	sml_tx_data[46] = 0x01;
	sml_tx_data[47] = 0x63;
	sml_tx_data[48] = 0x77;
	sml_tx_data[49] = 0x00;
	sml_tx_data[50] = 0x76;
	sml_tx_data[51] = 0x07;
	sml_tx_data[52] = 0x00;
	sml_tx_data[53] = 0x0E;
	sml_tx_data[54] = 0x03;
	sml_tx_data[55] = 0xAD;
	sml_tx_data[56] = 0x1E;
	sml_tx_data[57] = 0x68;
	sml_tx_data[58] = 0x62;
	sml_tx_data[59] = 0x00;
	sml_tx_data[60] = 0x62;
	sml_tx_data[61] = 0x00;
	sml_tx_data[62] = 0x72;
	sml_tx_data[63] = 0x63;
	sml_tx_data[64] = 0x07;
	sml_tx_data[65] = 0x01;
	sml_tx_data[66] = 0x77;
	sml_tx_data[67] = 0x01;
	sml_tx_data[68] = 0x0B;
	sml_tx_data[69] = 0x09;
	sml_tx_data[70] = 0x01;
	sml_tx_data[71] = 0x45;
	sml_tx_data[72] = 0x4D;
	sml_tx_data[73] = 0x48;
	sml_tx_data[74] = 0x00;
	sml_tx_data[75] = 0x00;
	sml_tx_data[76] = 0x41;
	sml_tx_data[77] = 0xA7;
	sml_tx_data[78] = 0x9F;
	sml_tx_data[79] = 0x07;
	sml_tx_data[80] = 0x01;
	sml_tx_data[81] = 0x00;
	sml_tx_data[82] = 0x62;
	sml_tx_data[83] = 0x0A;
	sml_tx_data[84] = 0xFF;
	sml_tx_data[85] = 0xFF;
	sml_tx_data[86] = 0x72;
	sml_tx_data[87] = 0x62;
	sml_tx_data[88] = 0x01;
	sml_tx_data[89] = 0x65;
	sml_tx_data[90] = 0x07;
	sml_tx_data[91] = 0xA7;
	sml_tx_data[92] = 0x1A;
	sml_tx_data[93] = 0x79;
	sml_tx_data[94] = 0x7A;
	sml_tx_data[95] = 0x77;
	sml_tx_data[96] = 0x07;
	sml_tx_data[97] = 0x81;
	sml_tx_data[98] = 0x81;
	sml_tx_data[99] = 0xC7;
	sml_tx_data[100] = 0x82;
	sml_tx_data[101] = 0x03;
	sml_tx_data[102] = 0xFF;
	sml_tx_data[103] = 0x01;
	sml_tx_data[104] = 0x01;
	sml_tx_data[105] = 0x01;
	sml_tx_data[106] = 0x01;
	sml_tx_data[107] = 0x04;
	sml_tx_data[108] = 0x45;
	sml_tx_data[109] = 0x4D;
	sml_tx_data[110] = 0x48;
	sml_tx_data[111] = 0x01;
	sml_tx_data[112] = 0x77;
	sml_tx_data[113] = 0x07;
	sml_tx_data[114] = 0x01;
	sml_tx_data[115] = 0x00;
	sml_tx_data[116] = 0x00;
	sml_tx_data[117] = 0x00;
	sml_tx_data[118] = 0x09;
	sml_tx_data[119] = 0xFF;
	sml_tx_data[120] = 0x01;
	sml_tx_data[121] = 0x01;
	sml_tx_data[122] = 0x01;
	sml_tx_data[123] = 0x01;
	sml_tx_data[124] = 0x0B;
	sml_tx_data[125] = 0x09;
	sml_tx_data[126] = 0x01;
	sml_tx_data[127] = 0x45;
	sml_tx_data[128] = 0x4D;
	sml_tx_data[129] = 0x48;
	sml_tx_data[130] = 0x00;
	sml_tx_data[131] = 0x00;
	sml_tx_data[132] = 0x41;
	sml_tx_data[133] = 0xA7;
	sml_tx_data[134] = 0x9F;
	sml_tx_data[135] = 0x01;
	sml_tx_data[136] = 0x77;
	sml_tx_data[137] = 0x07;
	sml_tx_data[138] = 0x01;
	sml_tx_data[139] = 0x00;
	sml_tx_data[140] = 0x01;
	sml_tx_data[141] = 0x08;
	sml_tx_data[142] = 0x00;
	sml_tx_data[143] = 0xFF;
	sml_tx_data[144] = 0x64;
	sml_tx_data[145] = 0x01;
	sml_tx_data[146] = 0x01;
	sml_tx_data[147] = 0xC2;
	sml_tx_data[148] = 0x01;
	sml_tx_data[149] = 0x62;
	sml_tx_data[150] = 0x1E;
	sml_tx_data[151] = 0x52;
	sml_tx_data[152] = 0xFF;
	sml_tx_data[153] = 0x56;
	sml_tx_data[154] = 0x00;
	sml_tx_data[155] = 0x04;
	sml_tx_data[156] = 0x97;
	sml_tx_data[157] = 0xD5;
	sml_tx_data[158] = 0x1E;
	sml_tx_data[159] = 0x01;
	sml_tx_data[160] = 0x77;
	sml_tx_data[161] = 0x07;
	sml_tx_data[162] = 0x01;
	sml_tx_data[163] = 0x00;
	sml_tx_data[164] = 0x02;
	sml_tx_data[165] = 0x08;
	sml_tx_data[166] = 0x00;
	sml_tx_data[167] = 0xFF;
	sml_tx_data[168] = 0x64;
	sml_tx_data[169] = 0x01;
	sml_tx_data[170] = 0x01;
	sml_tx_data[171] = 0xC2;
	sml_tx_data[172] = 0x01;
	sml_tx_data[173] = 0x62;
	sml_tx_data[174] = 0x1E;
	sml_tx_data[175] = 0x52;
	sml_tx_data[176] = 0xFF;
	sml_tx_data[177] = 0x56;
	sml_tx_data[178] = 0x00;
	sml_tx_data[179] = 0x12;
	sml_tx_data[180] = 0x7E;
	sml_tx_data[181] = 0x43;
	sml_tx_data[182] = 0xB6;
	sml_tx_data[183] = 0x01;
	sml_tx_data[184] = 0x77;
	sml_tx_data[185] = 0x07;
	sml_tx_data[186] = 0x01;
	sml_tx_data[187] = 0x00;
	sml_tx_data[188] = 0x01;
	sml_tx_data[189] = 0x08;
	sml_tx_data[190] = 0x01;
	sml_tx_data[191] = 0xFF;
	sml_tx_data[192] = 0x01;
	sml_tx_data[193] = 0x01;
	sml_tx_data[194] = 0x62;
	sml_tx_data[195] = 0x1E;
	sml_tx_data[196] = 0x52;
	sml_tx_data[197] = 0xFF;
	sml_tx_data[198] = 0x56;
	sml_tx_data[199] = 0x00;
	sml_tx_data[200] = 0x04;
	sml_tx_data[201] = 0x97;
	sml_tx_data[202] = 0xD5;
	sml_tx_data[203] = 0x1E;
	sml_tx_data[204] = 0x01;
	sml_tx_data[205] = 0x77;
	sml_tx_data[206] = 0x07;
	sml_tx_data[207] = 0x01;
	sml_tx_data[208] = 0x00;
	sml_tx_data[209] = 0x02;
	sml_tx_data[210] = 0x08;
	sml_tx_data[211] = 0x01;
	sml_tx_data[212] = 0xFF;
	sml_tx_data[213] = 0x01;
	sml_tx_data[214] = 0x01;
	sml_tx_data[215] = 0x62;
	sml_tx_data[216] = 0x1E;
	sml_tx_data[217] = 0x52;
	sml_tx_data[218] = 0xFF;
	sml_tx_data[219] = 0x56;
	sml_tx_data[220] = 0x00;
	sml_tx_data[221] = 0x12;
	sml_tx_data[222] = 0x7E;
	sml_tx_data[223] = 0x43;
	sml_tx_data[224] = 0xB6;
	sml_tx_data[225] = 0x01;
	sml_tx_data[226] = 0x77;
	sml_tx_data[227] = 0x07;
	sml_tx_data[228] = 0x01;
	sml_tx_data[229] = 0x00;
	sml_tx_data[230] = 0x01;
	sml_tx_data[231] = 0x08;
	sml_tx_data[232] = 0x02;
	sml_tx_data[233] = 0xFF;
	sml_tx_data[234] = 0x01;
	sml_tx_data[235] = 0x01;
	sml_tx_data[236] = 0x62;
	sml_tx_data[237] = 0x1E;
	sml_tx_data[238] = 0x52;
	sml_tx_data[239] = 0xFF;
	sml_tx_data[240] = 0x56;
	sml_tx_data[241] = 0x00;
	sml_tx_data[242] = 0x00;
	sml_tx_data[243] = 0x00;
	sml_tx_data[244] = 0x00;
	sml_tx_data[245] = 0x00;
	sml_tx_data[246] = 0x01;
	sml_tx_data[247] = 0x77;
	sml_tx_data[248] = 0x07;
	sml_tx_data[249] = 0x01;
	sml_tx_data[250] = 0x00;
	sml_tx_data[251] = 0x02;
	sml_tx_data[252] = 0x08;
	sml_tx_data[253] = 0x02;
	sml_tx_data[254] = 0xFF;
	sml_tx_data[255] = 0x01;
	sml_tx_data[256] = 0x01;
	sml_tx_data[257] = 0x62;
	sml_tx_data[258] = 0x1E;
	sml_tx_data[259] = 0x52;
	sml_tx_data[260] = 0xFF;
	sml_tx_data[261] = 0x56;
	sml_tx_data[262] = 0x00;
	sml_tx_data[263] = 0x00;
	sml_tx_data[264] = 0x00;
	sml_tx_data[265] = 0x00;
	sml_tx_data[266] = 0x00;
	sml_tx_data[267] = 0x01;
	sml_tx_data[268] = 0x77;
	sml_tx_data[269] = 0x07;
	sml_tx_data[270] = 0x01;
	sml_tx_data[271] = 0x00;
	sml_tx_data[272] = 0x10;
	sml_tx_data[273] = 0x07;
	sml_tx_data[274] = 0x00;
	sml_tx_data[275] = 0xFF;
	sml_tx_data[276] = 0x01;
	sml_tx_data[277] = 0x01;
	sml_tx_data[278] = 0x62;
	sml_tx_data[279] = 0x1B;
	sml_tx_data[280] = 0x52;
	sml_tx_data[281] = 0xFF;
	sml_tx_data[282] = 0x55;
	sml_tx_data[283] = 0x00;
	sml_tx_data[284] = 0x00;
	sml_tx_data[285] = 0x1E;
	sml_tx_data[286] = 0x5F;
	sml_tx_data[287] = 0x01;
	sml_tx_data[288] = 0x77;
	sml_tx_data[289] = 0x07;
	sml_tx_data[290] = 0x81;
	sml_tx_data[291] = 0x81;
	sml_tx_data[292] = 0xC7;
	sml_tx_data[293] = 0x82;
	sml_tx_data[294] = 0x05;
	sml_tx_data[295] = 0xFF;
	sml_tx_data[296] = 0x01;
	sml_tx_data[297] = 0x72;
	sml_tx_data[298] = 0x62;
	sml_tx_data[299] = 0x01;
	sml_tx_data[300] = 0x65;
	sml_tx_data[301] = 0x07;
	sml_tx_data[302] = 0xA7;
	sml_tx_data[303] = 0x1A;
	sml_tx_data[304] = 0x79;
	sml_tx_data[305] = 0x01;
	sml_tx_data[306] = 0x01;
	sml_tx_data[307] = 0x83;
	sml_tx_data[308] = 0x02;
	sml_tx_data[309] = 0xEA;
	sml_tx_data[310] = 0x21;
	sml_tx_data[311] = 0x30;
	sml_tx_data[312] = 0xD5;
	sml_tx_data[313] = 0x39;
	sml_tx_data[314] = 0x99;
	sml_tx_data[315] = 0x0D;
	sml_tx_data[316] = 0xA8;
	sml_tx_data[317] = 0x6A;
	sml_tx_data[318] = 0x5B;
	sml_tx_data[319] = 0xD1;
	sml_tx_data[320] = 0x82;
	sml_tx_data[321] = 0xA2;
	sml_tx_data[322] = 0x3D;
	sml_tx_data[323] = 0x70;
	sml_tx_data[324] = 0xEC;
	sml_tx_data[325] = 0x6A;
	sml_tx_data[326] = 0xE7;
	sml_tx_data[327] = 0x4B;
	sml_tx_data[328] = 0x58;
	sml_tx_data[329] = 0x18;
	sml_tx_data[330] = 0x53;
	sml_tx_data[331] = 0x0D;
	sml_tx_data[332] = 0x6D;
	sml_tx_data[333] = 0xE2;
	sml_tx_data[334] = 0x02;
	sml_tx_data[335] = 0xDC;
	sml_tx_data[336] = 0x8C;
	sml_tx_data[337] = 0x16;
	sml_tx_data[338] = 0x4E;
	sml_tx_data[339] = 0x18;
	sml_tx_data[340] = 0x3C;
	sml_tx_data[341] = 0x4D;
	sml_tx_data[342] = 0xB7;
	sml_tx_data[343] = 0x79;
	sml_tx_data[344] = 0xF8;
	sml_tx_data[345] = 0xEA;
	sml_tx_data[346] = 0x6B;
	sml_tx_data[347] = 0xF5;
	sml_tx_data[348] = 0x8E;
	sml_tx_data[349] = 0x49;
	sml_tx_data[350] = 0xFF;
	sml_tx_data[351] = 0x05;
	sml_tx_data[352] = 0x21;
	sml_tx_data[353] = 0xC1;
	sml_tx_data[354] = 0xC8;
	sml_tx_data[355] = 0x67;
	sml_tx_data[356] = 0x00;
	sml_tx_data[357] = 0x01;
	sml_tx_data[358] = 0x01;
	sml_tx_data[359] = 0x01;
	sml_tx_data[360] = 0x63;
	sml_tx_data[361] = 0xEC;
	sml_tx_data[362] = 0xA9;
	sml_tx_data[363] = 0x00;
	sml_tx_data[364] = 0x76;
	sml_tx_data[365] = 0x07;
	sml_tx_data[366] = 0x00;
	sml_tx_data[367] = 0x0E;
	sml_tx_data[368] = 0x03;
	sml_tx_data[369] = 0xAD;
	sml_tx_data[370] = 0x1E;
	sml_tx_data[371] = 0x6B;
	sml_tx_data[372] = 0x62;
	sml_tx_data[373] = 0x00;
	sml_tx_data[374] = 0x62;
	sml_tx_data[375] = 0x00;
	sml_tx_data[376] = 0x72;
	sml_tx_data[377] = 0x63;
	sml_tx_data[378] = 0x02;
	sml_tx_data[379] = 0x01;
	sml_tx_data[380] = 0x71;
	sml_tx_data[381] = 0x01;
	sml_tx_data[382] = 0x63;
	sml_tx_data[383] = 0xCB;
	sml_tx_data[384] = 0x4C;
	sml_tx_data[385] = 0x00;
	sml_tx_data[386] = 0x00;
	sml_tx_data[387] = 0x1B;
	sml_tx_data[388] = 0x1B;
	sml_tx_data[389] = 0x1B;
	sml_tx_data[390] = 0x1B;
	sml_tx_data[391] = 0x1A;
	sml_tx_data[392] = 0x01;
	sml_tx_data[393] = 0xF6;
	sml_tx_data[394] = 0xA5;

}

void sm_tx() {
	usart6_send_data(sml_tx_data, sizeof(sml_tx_data));
}

static int32_t calc_mean_i32b(int32_t *hist, uint32_t run_idx, uint32_t size) {

	if (run_idx == 0) {
		run_idx = SM_HISTORY_SIZE - 1;
	} else {
		run_idx--;
	}

	uint32_t seconds_sum = 0;
	int32_t power_sum = 0;
	while (seconds_sum++ < size) {
		power_sum += hist[run_idx];
		if (--run_idx < 0) {
			run_idx = SM_HISTORY_SIZE - 1;
		}
	}
	return power_sum / size;
}

int32_t sm_plant_get_mean_value(int32_t time) {

	if (time > SM_HISTORY_SIZE) {
		int32_t sum = 0;
		for (int i = 0; i < SM_HISTORY_SIZE; i++) {
			sum += sm_plant_hist[i];
		}
		return sum / SM_HISTORY_SIZE;
	}

	return calc_mean_i32b(sm_plant_hist,sm_main_hist_idx,time);

}

int32_t sm_main_get_mean_value(int32_t time) {

	if (time > SM_HISTORY_SIZE) {
		int32_t sum = 0;
		for (int i = 0; i < SM_HISTORY_SIZE; i++) {
			sum += sm_main_hist[i];
		}
		return sum / SM_HISTORY_SIZE;
	}

	return calc_mean_i32b(sm_main_hist,sm_main_hist_idx,time);

}

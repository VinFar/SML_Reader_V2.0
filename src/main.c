#include <stdint-gcc.h>
#include "stm32f091xc.h"
#include "stm32f0xx_hal.h"
#include "stm32f0xx_it.h"

#include "eeprom.h"
#include "string.h"
#include "functions.h"

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "timers.h"
#include "semphr.h"
#include "main.h"
#include "gpio.h"
#include "spi.h"
#include "flash.h"
#include "defines.h"
#include "comp.h"
#include "dac.h"

/* Priorities at which the tasks are created.  The event semaphore task is
 given the maximum priority of ( configMAX_PRIORITIES - 1 ) to ensure it runs as
 soon as the semaphore is given. */
#define mainQUEUE_RECEIVE_TASK_PRIORITY		( tskIDLE_PRIORITY + 2 )
#define	mainQUEUE_SEND_TASK_PRIORITY		( tskIDLE_PRIORITY + 1 )
#define mainEVENT_SEMAPHORE_TASK_PRIORITY	( configMAX_PRIORITIES - 1 )

/* The rate at which data is sent to the queue, specified in milliseconds, and
 converted to ticks using the portTICK_RATE_MS constant. */
#define mainQUEUE_SEND_PERIOD_MS			( 200 / portTICK_RATE_MS )

/* The period of the example software timer, specified in milliseconds, and
 converted to ticks using the portTICK_RATE_MS constant. */
#define mainSOFTWARE_TIMER_PERIOD_MS		( 1000 / portTICK_RATE_MS )

/* The number of items the queue can hold.  This is 1 as the receive task
 will remove items as they are added, meaning the send task should always find
 the queue empty. */
#define mainQUEUE_LENGTH					( 1 )

void SystemClock_Config(void);
static void prvSetupHardware(void);
void vApplicationMallocFailedHook(void);
void vApplicationStackOverflowHook( xTaskHandle *pxTask,
		signed char *pcTaskName);
void vApplicationTickHook();

TaskHandle_t xcheck_cmd_struct;

//uint8_t ucHeap[ configTOTAL_HEAP_SIZE ]={0};

uint32_t meter_purchase, meter_delivery, meter_uptime;
int32_t meter_power;
uint32_t error_counter = 0;

uint8_t *needle_ptr;
uint8_t needle[20];

/*
 *	sm_current_data is the current valid data of the smart meter
 */
smartmeter_data_t sm_current_data;

/*
 * sm_flash_cache_data is a array of smartmter_data_t surrounded by two delimiters
 * this struct is used to cache the smart meter data in order to just have one
 * write acces to the flash per page.
 * The entire struct is written to the flash when it is full.
 */
smartmeter_flash_data_t sm_flash_cache_data[W25N_MAX_CLOUMN
		/ sizeof(smartmeter_flash_data_t)];

uint8_t sm_idx_for_cache_data = 0;
uint32_t flash_current_address = 0;

int main(void) {

	prvSetupHardware();
	flags.new_main_sml_packet = 0;

	flash_init();
//	flash_bulkErase();

//	smartmeter_flash_data_t data[W25N_MAX_CLOUMN
//			/ sizeof(smartmeter_flash_data_t)];
//
//	data[0].packet_ctr = 1;
//	for (int k = 0; k < 23; k++) {
//		data[0].begin = BEGIN_DELIMITER;
//		data[0].delimiter = END_DELIMITER;
//		data[0].data.meter_delivery = 300;
//		data[0].data.meter_purchase = 400;
//		data[0].data.power = 50;
//		data[0].data.uptime = 0;
//
//		for (int i = 1; i < W25N_MAX_CLOUMN / sizeof(smartmeter_flash_data_t);
//				i++) {
//			data[i].begin = BEGIN_DELIMITER;
//			data[i].delimiter = END_DELIMITER;
//			data[i].packet_ctr = data[i - 1].packet_ctr + 1;
//			data[i].data.meter_delivery = (i + (k * 2048)) * 300;
//			data[i].data.meter_purchase = (i + (k * 2048)) * 400;
//			data[i].data.power = (i + (k * 2048)) * 50;
//			data[i].data.uptime = (i + (k * 2048));
//
//		}
//		flash_write_data(k*2048,data,2048);
//		data[0].packet_ctr = data[(W25N_MAX_CLOUMN / sizeof(smartmeter_flash_data_t))-1].packet_ctr;
//
//	}

	/*
	 * in order to find the last page that was written before system reset,
	 * we have to search the entire flah for a non 0xFF page:
	 * 1. Read page n and search for delimiters (137,78) with and distance of 16 bytes
	 * 2. this is the last page that was written
	 */
	uint32_t page_address = 0;
	for (uint32_t i = 0; i < W25N_MAX_USER_PAGE;
			i++, page_address += W25N_MAX_CLOUMN) {
		uint8_t page[W25N_MAX_CLOUMN] = { 0 };
		uint8_t first_byte[1000] = {0};
		flash_read_data(page_address, first_byte, 1000);
		if (first_byte[0] != BEGIN_DELIMITER) {
			/*
			 * this may be the first page that is empty.
			 * We have to check if the previous page was fully written
			 */
			page_address -= W25N_MAX_CLOUMN;
			smartmeter_flash_data_t flash_data = { 0 };
			for (uint32_t j = 0;
					j < (W25N_MAX_CLOUMN / sizeof(smartmeter_flash_data_t));j++) {
				flash_read_data(page_address + j*sizeof(smartmeter_flash_data_t), &flash_data, sizeof(flash_data));

				if (flash_data.begin != BEGIN_DELIMITER
						|| flash_data.delimiter != END_DELIMITER) {
					/*
					 * this means that the page was not fully written and is corrupted,
					 * so delete it and begin with this page
					 */
					flash_blockErase(page_address / W25N_MAX_CLOUMN);
					flash_current_address = page_address;
					break;
				}
			}
			/*
			 * the page was checked and is valid, so the following page is the correct page for writing
			 */
			flash_current_address = page_address + W25N_MAX_CLOUMN;
			break;
		}
	}

	while (1) {

		if (flags.new_main_sml_packet) {
			flags.new_main_sml_packet = 0;
			if (sml_main_raw_data_idx > sizeof(sml_main_raw_data)
					|| sml_main_raw_data_idx < 200) {
				LED_ERROR_ON;
				error_counter++;
				if (error_counter > 10) {
					error_counter = 0;
				}
			} else {
				uint16_t crc_calc = ccrc16((char*) sml_main_raw_data,
						sml_main_raw_data_idx - 2);
				uint16_t crc_check = ((uint16_t) sml_main_raw_data[sml_main_raw_data_idx
						- 2] << 8)
						+ (uint16_t) sml_main_raw_data[sml_main_raw_data_idx - 1];

				if (crc_check != crc_calc) {
					LED_ERROR_ON;
					error_counter++;
				} else {
					LED_ERROR_OFF;
					LED_OK_OFF;

					needle[0] = 0x07;
					needle[1] = 0x01;
					needle[2] = 0x00;
					needle[3] = 0x10;
					needle[4] = 0x07;
					needle[5] = 0x00;
					needle[6] = 0xff;	//Set String for Powervalue key

					if ((needle_ptr = (uint8_t*) memmem(sml_main_raw_data,
							sizeof(sml_main_raw_data), needle, sizeof(needle)))
							== NULL) {
						//No Active Power String detected, ERror

						continue;
					} else {
						float consumption_tmp = 0;
						int32_t tmp_value = (needle_ptr[14] << 24)
								+ (needle_ptr[15] << 16) + (needle_ptr[16] << 8)
								+ (needle_ptr[18]);	//Extract and calculate Powervalue
						sm_current_data.power = tmp_value / 10;
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
								sizeof(sml_main_raw_data), needle, sizeof(needle)))
								== NULL) {
							continue;
						} else {

							error_counter = 0;

							sm_current_data.meter_purchase = ((needle_ptr[15]
									<< 24) + (needle_ptr[16] << 16)
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
								sizeof(sml_main_raw_data), needle, sizeof(needle)))
								== NULL) {
							continue;
						} else {

							error_counter = 0;

							sm_current_data.meter_delivery = ((needle_ptr[15]
									<< 24) + (needle_ptr[16] << 16)
									+ (needle_ptr[17] << 8) + (needle_ptr[18]))
									/ 10000;

						}

						needle[0] = 0x07;
						needle[1] = 0x01;
						needle[2] = 0x00;
						needle[3] = 0x62;
						needle[4] = 0x0A;
						needle[5] = 0xff;
						needle[6] = 0xff;

						if ((needle_ptr = (unsigned char*) memmem(sml_main_raw_data,
								sizeof(sml_main_raw_data), needle, sizeof(needle)))
								== NULL) {
							continue;
						} else {
							error_counter = 0;
							sm_current_data.uptime = ((needle_ptr[11] << 24)
									+ (needle_ptr[12] << 16)
									+ (needle_ptr[13] << 8) + needle_ptr[14]);

							/*
							 * at this point all data from the smart meter was written
							 * to the struct an we can copy it into the array
							 * and increment the idx
							 */
							sm_flash_cache_data[sm_idx_for_cache_data].data =
									sm_current_data;

							if (sm_idx_for_cache_data == 0) {
								sm_flash_cache_data[sm_idx_for_cache_data].packet_ctr =
										0;
							} else {
								sm_flash_cache_data[sm_idx_for_cache_data].packet_ctr =
										sm_flash_cache_data[sm_idx_for_cache_data
												- 1].packet_ctr + 1;
							}
							sm_idx_for_cache_data++;

							/*
							 * if the cache is full, write it to the flash
							 */
							if (sm_idx_for_cache_data
									> (W25N_MAX_CLOUMN
											/ sizeof(smartmeter_flash_data_t))) {
								sm_idx_for_cache_data--;
								/*
								 * cache is full, so write it to the flash
								 * init the cache with 0 and set the correct packet ctr
								 */
								flash_write_data(flash_current_address,
										sm_flash_cache_data,
										sizeof(sm_flash_cache_data));
								flash_current_address += W25N_MAX_CLOUMN;
								sm_flash_cache_data[0].packet_ctr =
										sm_flash_cache_data[sm_idx_for_cache_data].packet_ctr
												+ 1;
								sm_idx_for_cache_data = 0;

							}
						}
					}
				}
			}
		}
	}
}

void vApplicationMallocFailedHook(void) {
	while (1)
		;
}

void vApplicationStackOverflowHook( xTaskHandle *pxTask,
		signed char *pcTaskName) {
	while (1)
		;
}

void vApplicationTickHook() {
	while (1)
		;
}

static void prvSetupHardware(void) {

	SystemClock_Config();

	gpio_init();

	/*
	 * communication for DAC and EEPROM
	 */
	i2c1_init();

	/*
	 * communication for NRF24 Wireless Chip and FLASH IC
	 */
	spi1_init();

	usart1_init();
	usart6_init();
	usart3_init();

	dac_init();
	comp_init();

}

void SystemClock_Config(void) {

	RCC_OscInitTypeDef RCC_OscInitStruct = { 0 };
	RCC_ClkInitTypeDef RCC_ClkInitStruct = { 0 };
	RCC_PeriphCLKInitTypeDef PeriphClkInit = { 0 };

	/** Initializes the CPU, AHB and APB busses clocks
	 */
	RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI
			| RCC_OSCILLATORTYPE_HSI14;
	RCC_OscInitStruct.HSIState = RCC_HSI_ON;
	RCC_OscInitStruct.HSI14State = RCC_HSI14_ON;
	RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
	RCC_OscInitStruct.HSI14CalibrationValue = 16;
	RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
	RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
	RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL6;
	RCC_OscInitStruct.PLL.PREDIV = RCC_PREDIV_DIV1;
	if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
		Error_Handler();
	}
	/** Initializes the CPU, AHB and APB busses clocks
	 */
	RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
			| RCC_CLOCKTYPE_PCLK1;
	RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
	RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
	RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;

	if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK) {
		Error_Handler();
	}
	PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_USART1
			| RCC_PERIPHCLK_I2C1;
	PeriphClkInit.Usart1ClockSelection = RCC_USART1CLKSOURCE_PCLK1;
	PeriphClkInit.I2c1ClockSelection = RCC_I2C1CLKSOURCE_HSI;
	if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK) {
		Error_Handler();
	}
}

void Error_Handler(void) {
	while (1)
		;
}

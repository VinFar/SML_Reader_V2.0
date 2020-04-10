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
#include "rtc.h"
#include "nrf24.h"
#include "nrf24_hal.h"
#include "queue.h"

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

uint8_t sml_tx_data[400] = { 0 };

uuid_t uuid = { { ((uint32_t*) UUID_BASE_ADDRESS),
		(((uint32_t*) UUID_BASE_ADDRESS + 1)), ((uint32_t*) (UUID_BASE_ADDRESS
				+ 2)) } };

int main(void) {

	prvSetupHardware();
	flags.new_main_sml_packet = 0;

	flash_init();
	flash_address_get_main();
	flash_address_get_plant();

	nrf24_init_tx();
	nrf_queue_init();

	uint32_t rtc_old_time=rtc_get_current_unix_time();

	while (1) {

		if(flags.oneHz_flags){
			flags.oneHz_flags=0;
			rtc_calc_new_time();
		}

		if (flags.new_plant_sml_packet) {
			flags.new_plant_sml_packet = 0;
			sm_plant_extract_data();
		}

		if (flags.new_main_sml_packet) {
			flags.new_main_sml_packet = 0;
			sm_main_extract_data();
		}

		if(nrf_queue_is_empty()==0){
			nrf_transmit_next_item();
		}

		if ((rtc_get_current_unix_time() - rtc_old_time) >= FLASH_SAVE_INTERVALL) {
			rtc_old_time = rtc_get_current_unix_time();
			/*
			 * save data every 2 seconds
			 */

			flash_main_store_data_in_cache(rtc_old_time);
			flash_plant_store_data_in_cache(rtc_old_time);

			union data_union sm[6];
			sm[0].uint32_data = flash_current_address_main_sml;
			sm[1].uint32_data = flash_current_address_plant_sml;
			sm[2].uint32_data = W25N_MAX_ADDRESS_MAIN;
			sm[3].uint32_data = W25N_MAX_ADDRESS_PLANT;
			sm[4].uint32_data = W25N_START_ADDRESS_MAIN;
			sm[5].uint32_data = W25N_START_ADDRESS_PLANT;

			nrf_add_qeue(NRF24_CMD_FLASH_DATA, sm);

		}

		if (flags.usart6_new_cmd) {
			check_cmd_frame();
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

	timer16_init();

	/*
	 * communication for DAC and EEPROM
	 */
	i2c1_init();

	/*
	 * communication for NRF24 Wireless Chip and FLASH IC
	 */
	spi1_init();

	/*
	 * SML Input 1
	 */
	usart1_init();

	/*
	 * HOST PC Communication
	 */
	usart6_init();

	/*
	 * SML Input 2
	 */
	usart3_init();
//	usart5_init();

	dac_init();
	comp_init();

	crc_init();

	rtc_init();

	TIM15_Init();

}

void SystemClock_Config(void) {

	RCC_OscInitTypeDef RCC_OscInitStruct = { 0 };
	RCC_ClkInitTypeDef RCC_ClkInitStruct = { 0 };
	RCC_PeriphCLKInitTypeDef PeriphClkInit = { 0 };

	/** Initializes the CPU, AHB and APB busses clocks
	 */
	RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI
			| RCC_OSCILLATORTYPE_HSI14 | RCC_OSCILLATORTYPE_LSE;
	RCC_OscInitStruct.HSIState = RCC_HSI_ON;
	RCC_OscInitStruct.LSEState = RCC_LSE_ON;
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
			| RCC_PERIPHCLK_I2C1 | RCC_PERIPHCLK_RTC;
	PeriphClkInit.Usart1ClockSelection = RCC_USART1CLKSOURCE_PCLK1;
	PeriphClkInit.I2c1ClockSelection = RCC_I2C1CLKSOURCE_HSI;
	PeriphClkInit.RTCClockSelection = RCC_RTCCLKSOURCE_LSE;
	if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK) {
		Error_Handler();
	}
}

void Error_Handler(void) {
	while (1)
		;
}

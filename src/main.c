#include <stdint-gcc.h>
#include "stm32f091xc.h"
#include "stm32f0xx_hal.h"
#include "stm32f0xx_it.h"

#include "eeprom.h"
#include "string.h"
#include "functions.h"
#include <time.h>

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "timers.h"
#include "semphr.h"
#include "main.h"
#include "gpio.h"
#include "spi.h"
#include "defines.h"
#include "dac.h"
#include "rtc.h"
#include "nrf24.h"
#include "lcd_menu.h"
#include "i2clcd.h"
#include "i2c.h"
#include "timer.h"
#include "nrf24.h"
#include "rtc.h"
#include "shared_defines.h"

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
void Initial_Init();
void outlet_on_off(menu_t *instance);

TaskHandle_t xcheck_cmd_struct;

//uint8_t ucHeap[ configTOTAL_HEAP_SIZE ]={0};

uuid_t uuid = { { ((uint32_t*) UUID_BASE_ADDRESS),
		(((uint32_t*) UUID_BASE_ADDRESS + 1)), ((uint32_t*) (UUID_BASE_ADDRESS
				+ 2)) } };

uint32_t rtc_current_time_unix;
uint32_t rtc_old_time_unix;

static uint8_t nrf24_rx_size = NRF24_RX_SIZE;

nrf24_frame_t nrf24_frame;

int8_t (*nrf24_frame_fct_ptr[MAX_ENUM_CMDS - 1])(nrf24_frame_t*,
		void*) = {ping_cmd_handler,ping_sm_data_handler,ping_rtc_data_handler,nrf_flash_data_handler
};

/*
 * Version v0.0.0.1
 */

int main(void) {

	prvSetupHardware();

	RTC_GetTime(RTC_Format_BIN, &sm_time);
	RTC_GetDate(RTC_Format_BIN, &sm_date);

	rtc_current_time_unix = rtc_old_time_unix = rtc_get_unix_time(&sm_time,
			&sm_date);

	nrf24_init_rx();

	while (1) {

		LED_OK_ON;
		__WFI();
		LED_OK_OFF;

		if (flags.refreshed_push) {

			flags.refreshed_push = 0;
		}

		if (flags.refreshed_rotary) {

			flags.refreshed_rotary = 0;
		}

		if (rtc_current_time_unix > rtc_old_time_unix) {

		}

		if (nRF24_GetStatus_RXFIFO() != nRF24_STATUS_RXFIFO_EMPTY) {
			// Get a payload from the transceiver

			nRF24_ReadPayload((uint8_t*) &nrf24_frame, &nrf24_rx_size);
			flags.nrf24_new_frame = 1;
			// Clear all pending IRQ flags
			nRF24_ClearIRQFlags();

		}

		if (flags.nrf24_new_frame) {
			LED_ERROR_TOGGLE;
			flags.nrf24_new_frame = 0;
			if (nrf24_frame.size <= NRF24_RX_SIZE) {
				/*
				 * size is ok
				 */
				if (nrf24_frame.cmd < NRF24_CMD_MAX_ENUM) {
					/*
					 * cmd is ok
					 */

					/*
					 * call the approbiate function
					 */
					nrf24_tx_ctr = nrf24_frame.tx_ctr;
					nrf24_frame_fct_ptr[nrf24_frame.cmd](&nrf24_frame, NULL);

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

	/*
	 * timer for delay function
	 */
	timer16_init();

	gpio_init();
	LED_OK_ON;
	LED_ERROR_ON;

	/*
	 * communication for DAC, EEPROM and Display
	 */
	i2c1_init();

	/*
	 * communication for NRF24 Wireless Chip and FLASH IC
	 */
	spi1_init();
//	usart6_init();

//	crc_init();

	rtc_init();

//	TIM3_Init(); //timer for rotary encoder
//	TIM6_Init(); //1khz counter for millisecond count
	TIM14_Init(); //used for resetting values and the system

	/*
	 * 40Hz interrupt for several timing features
	 */
	TIM15_Init();

	LED_ERROR_OFF;
	LED_OK_OFF;

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

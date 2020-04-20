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
#include "adc.h"
#include "stdlib.h"

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

static uint32_t rtc_old_time_unix;
nrf24_frame_t nrf24_frame;
static uint8_t nrf24_rx_size = NRF24_RX_SIZE;

int8_t (*nrf24_fct_vec_main[MAX_ENUM_CMDS - 1])(nrf24_frame_t*,
		void*) = {nrf_cmd_ping_handler,nrf_cmd_sm_data_handler,nrf_cmd_rtc_data_handler,nrf_cmd_flash_data_handler
};

int8_t (*nrf24_fct_vec_disp[MAX_ENUM_CMDS - 1])(nrf24_frame_t*,
		void*) = {nrf_cmd_ping_handler,nrf_cmd_relay_handler };

/*
 * Version v0.0.0.1
 */

int main(void) {

	prvSetupHardware();

	nrf24_init_gen();
	nrf24_init_rx();

	int32_t dr = 0;

	while (1) {
		dr++;
		LED_OK_ON;
		__WFI();
		LED_OK_OFF;

		if (rtc_get_current_unix_time() > rtc_old_time_unix) {
			rtc_old_time_unix = rtc_get_current_unix_time();
			int32_t power_tmp_main = sm_power_main_current - POWERVALUE_BUFFER;
			if (relay_right_get_state()) {
				/*
				 * if the relay is already on, than the
				 * power of main sm would not show a negative value
				 * and thus we would switch off the relay again.
				 * Then, in the next cycle we would switch the relays on again
				 * and this would lead to a 1hz toggling of the relay
				 * To prevent this, we need t osubstract the power value of the
				 * connected device to "trick" the system that everything is OK
				 */
				power_tmp_main -= POWERVALUE_RELAY_LEFT;
			}
			if (sm_power_main_current < -POWERVALUE_RELAY_LEFT) {
				relay_right_on();
			} else {
				relay_right_off();
			}
		}



		if (flags.nrf24_new_frame) {
			LED_ERROR_TOGGLE;
			flags.nrf24_new_frame = 0;
			nrf24_frame_t nrf24_frame;
			uint8_t rx_pipe = (uint8_t) nRF24_ReadPayload(
					(uint8_t*) &nrf24_frame, &nrf24_rx_size);
			UNUSED(rx_pipe);
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
					flags.smu_connected = 1;
					//nrf24_tx_ctr = nrf24_frame.tx_ctr;
					switch (rx_pipe) {
					case NRF_DISP_PIPE:
						nrf24_fct_vec_disp[nrf24_frame.cmd](&nrf24_frame, NULL);
						break;
					default:
						nrf24_fct_vec_main[nrf24_frame.cmd](&nrf24_frame, NULL);
						break;
					}
					if (nrf_queue_is_empty() != 1) {
						/*
						 * after every TX frame from the main unit we can transmit
						 * to it inside a 50ms window.
						 * So if the queue is not empty, we have to a sent the
						 * frame that is in the queue
						 */
						nrf24_init_tx();
						memset(&nrf24_frame, 0, sizeof(nrf24_frame));
						nrf_transmit_next_item();
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

	/*
	 * timer for delay function
	 */
	timer16_init();

	gpio_init();
	LED_OK_ON;
	LED_ERROR_ON;

	adc_init();

	__HAL_RCC_DAC1_CLK_ENABLE();
	DAC1->CR |= DAC_CR_EN1;
	DAC1->DHR12R1 = 4000;

	/*
	 * communication for DAC, EEPROM and Display
	 */
	i2c1_init();

	/*
	 * communication for NRF24 Wireless Chip and FLASH IC
	 */
	spi1_init();
	usart6_init();

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

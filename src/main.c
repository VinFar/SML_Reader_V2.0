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

// Buffer to store a payload of maximum width
uint8_t nRF24_payload[32];

// Pipe number
nRF24_RXResult pipe;

// Length of received payload
uint8_t payload_length;

nRF24_TXResult tx_res;

data_union_t nrf24_rx_data[5];
static const uint8_t nrf24_rx_size = NRF24_RX_SIZE;

static uint32_t idx_mean_value = 0;

static uint32_t idx_oldest_value = 0;

uint32_t startup_timestamp = 0;
uint32_t last_eeprom_timestamp = 0;

uint32_t uptime_of_smartmeter;
static uint32_t runtime_of_system_sec;

// Pipe number
nRF24_RXResult pipe;

// Length of received payload
uint8_t payload_length;

nrf24_frame_t nrf24_frame;

int8_t (*nrf24_frame_fct_ptr[MAX_ENUM_CMDS - 1])(nrf24_frame_t*,
		void*) = {ping_cmd_handler,ping_sm_data_handler,ping_rtc_data_handler };

/*
 * Version v0.1.0.2
 */

int main(void) {

	prvSetupHardware();

	RTC_GetTime(RTC_Format_BIN, &sm_time);
	RTC_GetDate(RTC_Format_BIN, &sm_date);

	rtc_current_time_unix = rtc_old_time_unix = rtc_get_unix_time(&sm_time,
			&sm_date);

	nrf24_init_rx();

	while (1) {

		rtc_current_time_unix = rtc_get_unix_time(&sm_time, &sm_date);

		if (flags.refreshed_push) {
			timer_ctr_for_lcd_light = 0;
			lcd_light(1);
			if (current_menu_ptr->items[menu_index].on_push == NULL) {

			} else {

				current_menu_ptr->items[menu_index].on_push(current_menu_ptr);
				flags.refreshed_rotary = 1;

			}
			flags.refreshed_push = 0;
		}

		if (flags.refreshed_rotary) {
			timer_ctr_for_lcd_light = 0;
			current_menu_ptr->items[menu_index].on_rotate(current_menu_ptr);
			flags.refreshed_rotary = 0;
			lcd_light(1);
		}

		if (rtc_current_time_unix > rtc_old_time_unix && flags.smu_connected) {
			rtc_old_time_unix = rtc_current_time_unix;
			/*
			 * write powervalue into history
			 */
			sm_power_hist[SM_MAIN_IDX_ARRAY][idx_mean_value] =
					sm_power_main_current;
			sm_power_hist[SM_PLANT_IDX_ARRAY][idx_mean_value] =
					sm_power_plant_current;

			/*
			 * we have a history of 300 values, so catch overflow
			 */
			uint16_t idx_newest_value = idx_mean_value;
			if (idx_mean_value++ == ARRAY_LEN(sm_power_hist[0])) {
				idx_mean_value = 0;
			}

			int32_t sm_power_sum_main = 0;
			int32_t sm_power_sum_plant = 0;
			/*
			 * we have to add all values that are inside the period specified by SECONDS_FOR_MEAN_VALUE
			 * From the newest value down to the period, so we have to decrement the idx.
			 * The user can configure the time period over which the mean value
			 * will be calculated (time_for_meanvalue). So abort on reaching this value
			 */
			for (uint32_t ctr = 0; ctr < time_for_meanvalue; ctr++) {
				/*
				 * add all values
				 */
				sm_power_sum_main +=
						sm_power_hist[SM_MAIN_IDX_ARRAY][idx_newest_value];
				sm_power_sum_plant +=
						sm_power_hist[SM_PLANT_IDX_ARRAY][idx_newest_value];

				/*
				 * catch overflow of index and decrement idx
				 */
				if (idx_newest_value == 0) {
					idx_newest_value = ARRAY_LEN(sm_power_hist[0]);
				} else {
					idx_newest_value--;
				}
				if (idx_newest_value == idx_mean_value) {
					/*
					 * at the beginning there will be not enough time stamps in the array
					 * to reach the sum of time_for_meanvalue, so catch index on which we began.
					 * if this is the case take the maximum passed time and use this for the mean value
					 */
					break;
				}
			}
			sm_power_main_mean = (int32_t) (sm_power_sum_main
					/ (time_for_meanvalue));
			sm_power_plant_mean = sm_power_sum_plant / time_for_meanvalue;

			/*
			 * end of mean value calculation
			 */
		}

		if (nRF24_GetStatus_RXFIFO() != nRF24_STATUS_RXFIFO_EMPTY) {
			// Get a payload from the transceiver

			pipe = nRF24_ReadPayload((uint8_t*) &nrf24_frame, &nrf24_rx_size);
			flags.nrf24_new_frame = 1;
			// Clear all pending IRQ flags
			nRF24_ClearIRQFlags();
		}
		if (flags.nrf24_new_frame) {
			flags.nrf24_new_frame = 0;
			if (nrf24_frame.size < NRF24_RX_SIZE) {
				/*
				 * size is ok
				 */
				if (nrf24_frame.cmd < NRF24_MAX_CMDS_ENUM) {
					/*
					 * cmd is ok
					 */

					/*
					 * call the approbiate function
					 */
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

	timer16_init();

	gpio_init();

	/*
	 * communication for DAC, EEPROM and Display
	 */
	i2c1_init();

	/*
	 * communication for NRF24 Wireless Chip and FLASH IC
	 */
	spi1_init();

	usart6_init();

	crc_init();

	rtc_init();

	TIM3_Init(); //timer for rotary encoder
	TIM6_Init(); //1khz counter for millisecond count
	TIM14_Init(); //used for resetting values and the system
	TIM15_Init();

	eeprom_erase_page(0);

	Initial_Init();

}

void Initial_Init() {

	lcd_init();
	for (int i = 0; i < 4 * 20; i++) {
		lcd_putchar(BLOCK);	//Test Display
		_delay_ms(5);
	}
	_delay_ms(800);
	lcd_command(0x01);			//Clear Display
	LED_ERROR_OFF;
	LED_FAULT_OFF;
	LED_OK_OFF;

	TIM3->CNT = 0x00ff;
	flags.lcd_light_on_off = 1;

	P_CONFIG = 1;

	eeprom_init_data();

	for (int i = 0; i < NUMBER_OF_OUTLETS; i++) {
		outlets_prio_ptr[i] = (uint32_t*) &outlets[i];
		outlets_value_ptr[i] = (uint32_t*) &outlets[i];
	}

	/*
	 * init the menu structs
	 */
	menu_init_struct(&Hauptmenu, Hauptmenu_items,
			SIZE_OF_MENU(Hauptmenu_items));
	menu_init_struct(&infomenu, infomenu_items, SIZE_OF_MENU(infomenu_items));

	/*
	 * universal menu for changing a value with the rotary encoder
	 */
	menu_init_struct(&changing_value, changing_value_item,
			SIZE_OF_MENU(changing_value_item));

	/*
	 * Minima and maxima menus fo time and power
	 */
	menu_init_struct(&maxima_menu, maxima_items, SIZE_OF_MENU(maxima_items));
	menu_init_struct(&minima_menu, minima_items, SIZE_OF_MENU(minima_items));
	menu_init_struct(&used_energy_menu, used_energy_items,
			SIZE_OF_MENU(used_energy_items));
	menu_init_struct(&mean_24h_menu, mean_24h_items,
			SIZE_OF_MENU(mean_24h_items));
	menu_init_struct(&mean_7d_menu, mean_7d_items, SIZE_OF_MENU(mean_7d_items));
	menu_init_struct(&mean_30d_menu, mean_30d_items,
			SIZE_OF_MENU(mean_30d_items));
	menu_init_struct(&mean_1y_menu, mean_1y_items, SIZE_OF_MENU(mean_1y_items));

	/*
	 * add the corresponding submenus
	 */
	menu_add_submenu(&Hauptmenu, &infomenu, 2);

	menu_add_submenu(&infomenu, &maxima_menu, 1);
	menu_add_submenu(&infomenu, &minima_menu, 2);
	menu_add_submenu(&infomenu, &used_energy_menu, 3);
	menu_add_submenu(&infomenu, &mean_24h_menu, 4);
	menu_add_submenu(&infomenu, &mean_7d_menu, 5);
	menu_add_submenu(&infomenu, &mean_30d_menu, 6);
	menu_add_submenu(&infomenu, &mean_1y_menu, 7);

	/*
	 * This menu is used to change the values of the different outlets
	 */
	changing_value.items[0].on_rotate = &on_rotary_change_value;
	changing_value.items[1].on_rotate = &on_rotary_change_value;

	changing_value.items[0].on_push = &call_menu_change_value;
	changing_value.items[1].on_push = &call_menu_change_value;

	uint8_t *tmp = 0;
	changing_value.user_data = (void*) tmp;

	/*
	 * Init text of of main menu
	 */
	menu_init_text(&Hauptmenu.items[1], "Started");
	menu_init_text(&Hauptmenu.items[2], (char*) "Systeminfo");
	menu_init_text(&Hauptmenu.items[3], (char*) "Steckdoseneinst.");
	menu_init_text(&Hauptmenu.items[4], (char*) "Tarifeinst.");

	/*
	 * Initiation of infomenu
	 */
	menu_init_text(&infomenu.items[1], "Maxima");
	menu_init_text(&infomenu.items[2], "Minima");
	menu_init_text(&infomenu.items[3], "genutzte Leist.");
	menu_init_text(&infomenu.items[4], "24h Mittel");
	menu_init_text(&infomenu.items[5], "7d Mittel");
	menu_init_text(&infomenu.items[6], "30d Mittel");
	menu_init_text(&infomenu.items[7], "1y Mittel");

	/*
	 * Initiation of the info menus
	 */
	int32_t max_value = 0;

	char tmp_str[20];
	char men[20];

	/*
	 * maximum power value of all time
	 */
	memset(men, 0, sizeof(men));
	strcpy(men, "Power:");
	strcat(men, tmp_str);
	strcat(men, "W");
	menu_init_text(&maxima_items[1], men);
	menu_fct_for_delayed_push(&maxima_menu.items[1], &on_push_reset_value);

	/*
	 * minimum value of all time
	 */
	memset(men, 0, sizeof(men));
	strcpy(men, "Power:");
	strcat(men, tmp_str);
	strcat(men, "W");
	menu_init_text(&minima_items[1], men);
	menu_fct_for_delayed_push(&minima_menu.items[1], &on_push_reset_value);

	/*
	 * 24h mean value of power
	 * Todo: new menu_printf function, check if this works
	 */
	menu_fct_for_delayed_push(&minima_menu.items[1], &on_push_reset_value);

	/*
	 * mean power over 7 days
	 */

	/*
	 * maximum time between two data packets ever recorded
	 */
	menu_fct_for_delayed_push(&maxima_menu.items[2], &on_push_reset_value);

	memset(men, 0, sizeof(men));
	strcpy(men, "Time:");
	strcat(men, tmp_str);
	strcat(men, "ms");
	menu_init_text(&minima_items[2], men);
	menu_fct_for_delayed_push(&minima_menu.items[2], &on_push_reset_value);

	menu_init_text(&changing_value.items[0], "");
	menu_init_text(&changing_value.items[1], "");

	menu_fct_for_push(&changing_value.items[0], &call_menu_change_value);

	current_menu_ptr = &Hauptmenu;
	flags.currently_in_menu = 1;
	flags.refreshed_push = 1;
	return;
}

void outlet_on_off(menu_t *instance) {

	outlets_t *outlet;
	outlet = (outlets_t*) instance->items[menu_index].user_data;
	;
	if (outlet == NULL) {
		/*
		 * null pointer
		 */
		return;
	}
	if (outlet->state == 1) {
		switch_outlet(outlet, 0);
		strcpy(&outlet->ptr_to_string[12], (char*) " aus");
	} else {
		switch_outlet(outlet, 1);
		strcpy(&outlet->ptr_to_string[12], (char*) " ein");
	}

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

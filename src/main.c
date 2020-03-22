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
uint8_t nrf24_rx_size = 0;

static uint32_t idx_mean_value = 0;

uint32_t startup_timestamp = 0;
uint32_t last_eeprom_timestamp = 0;

uint32_t uptime_of_smartmeter;
static uint32_t runtime_of_system_sec;

uint32_t i, j, k;

// Buffer to store a payload of maximum width
uint8_t nRF24_payload[32];

// Pipe number
nRF24_RXResult pipe;

// Length of received payload
uint8_t payload_length;

/*
 * Version v0.1.0.1
 */

int main(void) {

	prvSetupHardware();

	RTC_GetTime(RTC_Format_BIN, &sm_time);
	RTC_GetDate(RTC_Format_BIN, &sm_date);

	rtc_current_time_unix = rtc_old_time_unix = rtc_get_unix_time(&sm_time,
			&sm_date);

	nrf24_init_rx();

	while (1) {
		if (flags.init_lcd) {
			if (lcd_poll() < 0) {
				/*
				 * still can't reach display
				 */

			} else {
				/*
				 * display is connected again
				 */
				current_menu_ptr = &Hauptmenu;
				menu_index = 0;
				flags.init_lcd = 0;
				lcd_init();
			}
		}
		if (flags.refreshed_push) {
			if (current_menu_ptr->items[menu_index].on_push == NULL) {

			} else {

				current_menu_ptr->items[menu_index].on_push(current_menu_ptr);
				flags.refreshed_rotary = 1;

			}
			flags.refreshed_push = 0;
		}
		if (flags.refreshed_rotary) {
			current_menu_ptr->items[menu_index].on_rotate(current_menu_ptr);
			flags.refreshed_rotary = 0;
		}

		//
		// Constantly poll the status of the RX FIFO and get a payload if FIFO is not empty
		//
		// This is far from best solution, but it's ok for testing purposes
		// More smart way is to use the IRQ pin :)
		//
		if (nRF24_GetStatus_RXFIFO() != nRF24_STATUS_RXFIFO_EMPTY) {
			// Get a payload from the transceiver
			pipe = nRF24_ReadPayload(nrf24_rx_data, &nrf24_rx_size);
			powervalue_mean = nrf24_rx_data[0].int32_data;
			flags.refreshed_rotary = 1;

			// Clear all pending IRQ flags
			nRF24_ClearIRQFlags();

			// Print a payload contents to UART
			UART_SendStr("RCV PIPE#");
			UART_SendInt(pipe);
			UART_SendInt0(nrf24_rx_data[0].int32_data);
			UART_SendInt0(nrf24_rx_data[1].int32_data);
			UART_SendStr(" PAYLOAD:>");
			UART_SendBufHex((char*) nRF24_payload, payload_length);
			UART_SendStr("<\r\n");
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

	powervalue_current = 0;
	powervalue_used_by_consumers = 0;
	old_Powervalue = 0;

	TIM3->CNT = 0x00ff;
	flags.sml_rx_on_off_flag = 0;
	flags.lcd_light_on_off = 1;

	P_CONFIG = 1;

	eeprom_init_data();

	for (int i = 0; i < NUMBER_OF_OUTLETS; i++) {
		outlets_prio_ptr[i] = (uint32_t*) &outlets[i];
		outlets_value_ptr[i] = (uint32_t*) &outlets[i];
	}

	sort_outlets_by_value();

	/*
	 * init the menu structs
	 */
	menu_init_struct(&Hauptmenu, Hauptmenu_items,
			SIZE_OF_MENU(Hauptmenu_items));
	menu_init_struct(&Steckdoseneinstellungen, Steckdoseneinstellungen_items,
			SIZE_OF_MENU(Steckdoseneinstellungen_items));
	menu_init_struct(&Tarifeinstellungen, Tarifeinstellungen_items,
			SIZE_OF_MENU(Tarifeinstellungen_items));
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
	menu_add_submenu(&Hauptmenu, &Steckdoseneinstellungen, 3);
	menu_add_submenu(&Hauptmenu, &Tarifeinstellungen, 4);

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
	 * init text of tarifeinstellungen
	 */
	menu_init_text(&Tarifeinstellungen.items[0], "Zurueck ");
	menu_init_text(&Tarifeinstellungen.items[1], "Preis kWh EK");
	menu_init_text(&Tarifeinstellungen.items[2], "Preis kWh VK");
	menu_init_text(&Tarifeinstellungen.items[3], "Preis kWh EV <0.3");
	menu_init_text(&Tarifeinstellungen.items[4], "Preis kWh EV >0.3");
	menu_init_text(&Tarifeinstellungen.items[5], "Monatl. Grundpreis");

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

	/*
	 * init outlets menu and structs
	 */
	char tmp_str[20];
	char men[20];
	for (int i = 1; i < NUMBER_OF_OUTLETS + 1; i++) {
		strcpy(men, "Steckdose ");
		itoa(i, tmp_str, 10);
		const char dp[6] = ": aus";
		strcat(men, tmp_str);
		strcat(men, dp);
		menu_init_text(&Steckdoseneinstellungen.items[i], men);
		menu_fct_for_push(&Steckdoseneinstellungen.items[i], &outlet_on_off);
		menu_add_submenu(&Steckdoseneinstellungen, &changing_value, i + 6);
		menu_fct_for_push(&Steckdoseneinstellungen.items[i + 6],
				&call_menu_steckdoseneinstellunge);
		menu_add_userdata(&Steckdoseneinstellungen.items[i + 6],
				&outlets[i - 1]);
		menu_add_userdata(&Steckdoseneinstellungen.items[i], &outlets[i - 1]);
		outlets[i - 1].ptr_to_string = Steckdoseneinstellungen.items[i].string;
	}

	menu_init_text(&Steckdoseneinstellungen.items[7], "Leistung Steckd. 1");
	menu_init_text(&Steckdoseneinstellungen.items[8], "Leistung Steckd. 2");
	menu_init_text(&Steckdoseneinstellungen.items[9], "Leistung Steckd. 3");
	menu_init_text(&Steckdoseneinstellungen.items[10], "Leistung Steckd. 4");
	menu_init_text(&Steckdoseneinstellungen.items[11], "Leistung Steckd. 5");
	menu_init_text(&Steckdoseneinstellungen.items[12], "Leistung Steckd. 6");

	/*
	 * maximum power value of all time
	 */
	memset(men, 0, sizeof(men));
	strcpy(men, "Power:");
	itoa(eeprom_powermax.data, tmp_str, 10);
	strcat(men, tmp_str);
	strcat(men, "W");
	menu_init_text(&maxima_items[1], men);
	menu_add_userdata(&maxima_menu.items[1], &eeprom_powermax);
	menu_fct_for_delayed_push(&maxima_menu.items[1], &on_push_reset_value);

	/*
	 * minimum value of all time
	 */
	memset(men, 0, sizeof(men));
	strcpy(men, "Power:");
	itoa(eeprom_powermin.data, tmp_str, 10);
	strcat(men, tmp_str);
	strcat(men, "W");
	menu_init_text(&minima_items[1], men);
	menu_add_userdata(&minima_menu.items[1], &eeprom_powermin);
	menu_fct_for_delayed_push(&minima_menu.items[1], &on_push_reset_value);

	/*
	 * 24h mean value of power
	 * Todo: new menu_printf function, check if this works
	 */
	menu_printf(&mean_24h_items[1], "Power: %i W", eeprom_meanpower24h.data);
	menu_add_userdata(&mean_24h_menu.items[1], &eeprom_meanpower24h);
	menu_fct_for_delayed_push(&minima_menu.items[1], &on_push_reset_value);

	/*
	 * mean power over 7 days
	 */
	menu_printf(&mean_7d_items[1], "Power: %i W", eeprom_meanpower7d.data);

	/*
	 * maximum time between two data packets ever recorded
	 */
	menu_printf(&maxima_items[2], "Time: %d ms",
			(uint32_t) eeprom_timemax.data);
	menu_add_userdata(&maxima_menu.items[2], &eeprom_timemax);
	menu_fct_for_delayed_push(&maxima_menu.items[2], &on_push_reset_value);

	memset(men, 0, sizeof(men));
	strcpy(men, "Time:");
	itoa(eeprom_timemin.data, tmp_str, 10);
	strcat(men, tmp_str);
	strcat(men, "ms");
	menu_init_text(&minima_items[2], men);
	menu_add_userdata(&minima_menu.items[2], &eeprom_timemin);
	menu_fct_for_delayed_push(&minima_menu.items[2], &on_push_reset_value);

	menu_init_text(&changing_value.items[0], "");
	menu_init_text(&changing_value.items[1], "");

	menu_fct_for_push(&changing_value.items[0], &call_menu_change_value);
	menu_fct_for_push(&Hauptmenu.items[1], &on_push_start_stopp_usart);

	flags.sml_rx_on_off_flag = 1;
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

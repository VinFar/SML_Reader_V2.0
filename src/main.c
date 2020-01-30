#include <stdint-gcc.h>
#include "stm32f091xc.h"
#include "stm32f0xx_hal.h"

#include "eeprom.h"
#include "string.h"
#include "functions.h"

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "timers.h"
#include "semphr.h"
#include "main.h"

/* Priorities at which the tasks are created.  The event semaphore task is
 given the maximum priority of ( configMAX_PRIORITIES - 1 ) to ensure it runs as
 soon as the semaphore is given. */
#define mainEVENT_SEMAPHORE_TASK_PRIORITY	( configMAX_PRIORITIES - 1 )


void SystemClock_Config(void);
static void prvSetupHardware(void);
void vApplicationMallocFailedHook(void);
void vApplicationStackOverflowHook( xTaskHandle *pxTask,
		signed char *pcTaskName);
void vApplicationTickHook();

TaskHandle_t xcheck_cmd_struct;

//uint8_t ucHeap[ configTOTAL_HEAP_SIZE ]={0};

int main(void) {

	prvSetupHardware();

	xTaskCreate(check_cmd_struct, "cmd check", 10, (void *) 1,
			tskIDLE_PRIORITY, &xcheck_cmd_struct);

	vTaskStartScheduler();

	NOP
	NOP
	NOP
	NOP

	while (1) {

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
	 * communication for DAC and EEPROM
	 */
//	i2c1_init();

	/*
	 * communication for NRF24 Wireless Chip
	 */
//	spi1_init();

	/*
	 * PWM 2 Output
	 */
//	tim14_init();

	/*
	 * PWM 1 Output
	 */
//	tim1_init();

	/*
	 * ADC Input for Mains voltage current sensor
	 */
//	adc_init();

	/*
	 * Init DAC IC for Analog Output voltage
	 */
//	init_dac47();

//	set_dac47_out1(0);
//	set_dac47_out2(0);

}

void SystemClock_Config(void) {

	/*
	 * enable clock security system
	 */
	RCC->CR |= RCC_CR_CSSON;

	/*
	 * enable the external High speed crystal
	 */
	RCC->CR |= RCC_CR_HSEON;

	/*
	 * wait till the hse is ready
	 */
	while (!((RCC->CR & RCC_CR_HSERDY) == RCC_CR_HSERDY))
		;

	/*
	 * set the pll multiplicator to 6: 6 x 8Mhz HSE = 48Mhz PLLCLK
	 */
	RCC->CFGR |= 0b0100 << RCC_CFGR_PLLMUL_Pos;

	/*
	 * Select HSE / Predivider as source: 8Mhz/1 = 8MHz PLL Clock source
	 */
	RCC->CFGR |= RCC_CFGR_PLLSRC_HSE_PREDIV << RCC_CFGR_PLLSRC_Pos;

	/*
	 * Select PLLCLK (48Mhz) as SYSCLK Source
	 */
	RCC->CFGR |= RCC_CFGR_SWS_PLL << RCC_CFGR_SWS_Pos;

	/*
	 * Enable the PLL
	 */
	RCC->CR |= RCC_CR_PLLON;

	while (!((RCC->CR & RCC_CR_PLLRDY) == RCC_CR_PLLRDY))
		;

	SystemCoreClock = 48000000;
}

void Error_Handler(void) {
	while (1)
		;
}

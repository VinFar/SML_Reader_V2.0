#include "main.h"

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

int main(void) {

	prvSetupHardware();



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
	i2c1_init();

	eeprom_erase_page(0);

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
	while(!((RCC->CR & RCC_CR_HSERDY) == RCC_CR_HSERDY));

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

	while(!((RCC->CR & RCC_CR_PLLRDY) == RCC_CR_PLLRDY));

	SystemCoreClock = 48000000;
}

void Error_Handler(void)
{
	while(1);
}

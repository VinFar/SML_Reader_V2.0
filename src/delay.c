#include "main.h"
#include "timer.h"
#include "stm32f0xx_hal_tim.h"

#define TIMER_INSTANCE TIM16


void delay_us(uint32_t Delay) {
	uint32_t tickstart = TIMER_INSTANCE->CNT;
	uint32_t wait = Delay;

	uint64_t check = Delay + tickstart;

	if (check > HAL_MAX_DELAY) {
		/*
		 * catch overflow
		 */

		/*
		 * wait until timer has overflowed
		 */
		while (TIMER_INSTANCE->CNT > tickstart)
			;

		/*
		 * calculate the ticks to wait and then go on
		 */
		wait = wait - (HAL_MAX_DELAY - tickstart);
		tickstart = 0;

	}
	if (wait < HAL_MAX_DELAY) {
		wait += (uint32_t) (1);
	}

	while ((TIMER_INSTANCE->CNT - tickstart) < wait) {
	}

}


void HAL_Delay(uint32_t Delay){
	delay_us(Delay*1000);
	return;
}

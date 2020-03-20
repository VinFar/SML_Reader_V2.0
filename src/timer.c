#include "main.h"
#include "timer.h"
#include "stm32f0xx_hal_tim.h"


/*
 * Timer for delay function
 */
void timer2_init() {

	__HAL_RCC_TIM2_CLK_ENABLE();
	TIM2->PSC = SystemCoreClock / 1000000; //timer frequency is 1Mhz
	TIM2->CNT = 0;
	TIM2->EGR |= TIM_EGR_UG;
	TIM2->CR1 |= TIM_CR1_CEN;
	TIM2->SR &= ~TIM_SR_UIF;
	TIM2->CR1 |= TIM_CR1_CEN;

}

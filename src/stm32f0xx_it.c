#include "stm32f0xx_it.h"
#include <stm32f0xx.h>
#include "main.h"
#include "usart.h"
#include "lcd_menu.h"


void NMI_Handler(void) {
}

void HardFault_Handler(void) {
	while (1) {
	}
}

void TIM1_BRK_UP_TRG_COM_IRQHandler(void) {	//Overflow Interrupt of Timer1: Send Recieved Data from IR_Input over USART2

	if ((TIM1->SR & TIM_SR_UIF) == TIM_SR_UIF) { //Update-Interrupt Flag

		TIM1->SR &= ~TIM_SR_UIF;	//Reset Overflow Flag
	} else {
		TIM1->CR1 &= ~TIM_CR1_CEN;	//Disable Timer 1
		//should not happen
		return;
	}
	TIM1->CR1 &= ~TIM_CR1_CEN;	//Disable Timer 1

	return;
}

/*
 *
 * Timer 2 generates a Interrupt every 1000ms to send the actual Powervalue over I2C for Data Logging
 *
 */

void TIM2_IRQHandler() {

	if (TIM2->SR & TIM_SR_UIF) {
		/*
		 * interrupt from overflow
		 */
		TIM2->SR &= ~TIM_SR_UIF;	//Reset Interrupt Flag

	}

	return;
}

/*
 *
 * ISR from pressing and rotating the Rotary Encoder
 * Up and Downcounting with the Encoder is implemented by Timer 3
 *
 */
void EXTI4_15_IRQHandler() {

	if (( EXTI->PR & EXTI_PR_PR6)) {	//Interrupt from rotating rotary encoder

		flags.refreshed_rotary = 1;
		if (ROTARY_B_GPIO_Port->IDR & ROTARY_B_Pin) {
			menu_timer_index++;

		} else {
			menu_timer_index--;
		}

		EXTI->PR |= EXTI_PR_PR6;	//Reset Interrupt Flag
		return;

	}

	if ((EXTI->PR & EXTI_PR_PR15)) {	//Interrupt from pressing rotary encoder

		if (!(ROTARY_PUSH_GPIO_Port->IDR & ROTARY_PUSH_PIN)) {
			/*
			 * rotary encoder was pushed
			 */
			flags.refreshed_push = 1;
			/*
			 * restart timer with 0
			 */
			TIM14->CNT = 0;
			TIM14->CR1 |= TIM_CR1_CEN;
			TIM14->SR &= ~TIM_SR_UIF;	//Reset Interrupt Flag

		} else if ((ROTARY_PUSH_GPIO_Port->IDR & ROTARY_PUSH_PIN)) {
			/*
			 * encoder push button was released inside the reset period
			 */
			TIM14->CR1 &= ~TIM_CR1_CEN;
		}
		EXTI->PR |= EXTI_PR_PR15;

		return;

	}
}

/*
 *
 * Interrupt for timer 6 and DAC.
 * No current use for this.
 *
 */

void TIM6_DAC_IRQHandler() {

	if ((TIM6->SR & TIM_SR_UIF) == TIM_SR_UIF) {

		TIM6->SR &= ~TIM_SR_UIF;

	}
	return;
}

/*
 * Timer 14 is used to detect a user requested reset of the system or of dedicated values
 */
void TIM14_IRQHandler() {

	if (TIM14->SR & TIM_SR_UIF) {
		TIM14->SR &= ~TIM_SR_UIF;	//Reset Interrupt Flag
		TIM14->CNT = 0;
		TIM14->CR1 &= ~TIM_CR1_CEN;
		/*
		 * user requested a system reset
		 */
		if (current_menu_ptr->items[menu_index].on_push_delayed == NULL) {

		} else {

			current_menu_ptr->items[menu_index].on_push_delayed(
					current_menu_ptr);
		}

	} else {

	}
	return;
}

uint32_t timer_ctr_for_lcd_light = 0;

void TIM15_IRQHandler() {
	if ((TIM15->SR & TIM_SR_UIF) == TIM_SR_UIF) {	//Interrupt every 25 ms
		TIM15->SR &= ~TIM_SR_UIF;	//Reset update interrupt flag
		if(timer_ctr_for_lcd_light++ > 1200){
			/*
			 * light off after 5 min
			 */
			lcd_light(0);
		}
	}
}

#include "stm32f0xx_it.h"
#include <stm32f0xx.h>
#include "main.h"
#include "usart.h"
#include "lcd_menu.h"
#include "i2clcd.h"
#include "rtc.h"

void NMI_Handler(void) {
}

void HardFault_Handler(void) {
	lcd_clear();
	lcd_printlc(1, 1, "HardFaultHandler!");
	lcd_printlc(2, 1, "Shoud not happen:(");
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
uint32_t menu_idx_isr = 10000;
void EXTI4_15_IRQHandler() {
	timer_ctr_for_lcd_light = 0;
	lightOn = 1;
	if (( EXTI->PR & EXTI_PR_PR6)) {	//Interrupt from rotating rotary encoder

		flags.refreshed_rotary = 1;

		/*
		 * When we are currently in the menu for changing a value
		 * we don't want to change the menu_index, because this
		 * would lead to indexinf out of the items array.
		 * So just don't touch this index, when we are the
		 * changing value menu
		 */
		if (ROTARY_B_GPIO_Port->IDR & ROTARY_B_Pin) {
			menu_idx_isr++;
			if (current_menu_ptr != &menu_changing_value) {

				if (++menu_timer_index > current_menu_ptr->size - 1) {
					menu_timer_index = current_menu_ptr->size - 1;
				}

			}
			flags.rotary_direction = 1;

		} else {
			menu_idx_isr--;
			if (current_menu_ptr != &menu_changing_value) {

				if (--menu_timer_index < 0) {
					menu_timer_index = 0;
				}
			}
			flags.rotary_direction = 0;
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
		 * user requested a  reset
		 */
		if (current_menu_ptr->items[menu_timer_index].on_push_delayed == NULL) {

		} else {

			current_menu_ptr->items[menu_timer_index].on_push_delayed(
					current_menu_ptr);
		}

	} else {

	}
	return;
}

uint32_t timer_ctr_for_lcd_light = 0;
uint32_t time_for_lcd_light = 120;
uint32_t timer_ctr_for_nrf24_timeout = 0;
static uint32_t timer_ctr_for_rtc_calc = 38;
void TIM15_IRQHandler() {
	if ((TIM15->SR & TIM_SR_UIF) == TIM_SR_UIF) {	//Interrupt every 25 ms
		TIM15->SR &= ~TIM_SR_UIF;	//Reset update interrupt flag
		if (timer_ctr_for_lcd_light++ > time_for_lcd_light * 40) {
			/*
			 * light off after 5 min
			 */
			lightOn = 0;
			lcd_light(lightOn);
		}
		if (timer_ctr_for_nrf24_timeout++ > 5 * 40) {
			/*
			 * TImeout of NRF24 communication: no data packet for more than 5 seconds
			 */
		}
		if (timer_ctr_for_rtc_calc++ > 35) {
			timer_ctr_for_rtc_calc = 0;
			RTC_GetTime(RTC_Format_BIN, &sm_time);
			RTC_GetDate(RTC_Format_BIN, &sm_date);
			rtc_current_time_unix = rtc_get_unix_time(&sm_time, &sm_date);
		}
	}
}

void RTC_IRQHandler() {

	NOP
}

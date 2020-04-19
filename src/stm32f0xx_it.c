#include "stm32f0xx_it.h"
#include <stm32f0xx.h>
#include "main.h"
#include "usart.h"
#include "lcd_menu.h"
#include "i2clcd.h"
#include "rtc.h"
#include "nrf24.h"

void NMI_Handler(void) {
}

void HardFault_Handler(void) {
	NVIC_SystemReset();
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

			flags.rotary_direction = 1;

		} else {

			flags.rotary_direction = 0;
		}

		EXTI->PR = EXTI_PR_PR6;	//Reset Interrupt Flag
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
		EXTI->PR = EXTI_PR_PR15;

		return;

	}

	if ((EXTI->PR & EXTI_PR_PIF12) == EXTI_PR_PIF12) {
		EXTI->PR = EXTI_PR_PR12;
		if (nRF24_GetStatus_RXFIFO() != nRF24_STATUS_RXFIFO_EMPTY) {
			// Get a payload from the transceiver
			flags.nrf24_new_frame = 1;
			// Clear all pending IRQ flags
			nRF24_ClearIRQFlags();

		}

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
	} else {

	}
	return;
}

uint32_t timer_ctr_for_lcd_light = 0;
uint32_t time_for_lcd_light = 120;
uint32_t timer_ctr_for_nrf24_timeout = 0;
static uint32_t timer_ctr_for_rtc_calc = 38;
volatile uint32_t timer_ctr_for_power_valid_timeout = 0;
void TIM15_IRQHandler() {
	if ((TIM15->SR & TIM_SR_UIF) == TIM_SR_UIF) {	//Interrupt every 25 ms
		TIM15->SR &= ~TIM_SR_UIF;	//Reset update interrupt flag

		if (timer_ctr_for_nrf24_timeout++ > 5 * 40) {
			/*
			 * TImeout of NRF24 communication: no data packet for more than 5 seconds
			 */
			flags.smu_connected = 0;
		}
		if (timer_ctr_for_rtc_calc++ > 35) {
			timer_ctr_for_rtc_calc = 0;


		}
		if (++timer_ctr_for_power_valid_timeout > (5 * 60 * 40)) {
			/*
			 * If no new power frame was recieved inside 5 minutes, than
			 * the Main Unit is declared as not connected and the relays are
			 * shut off
			 */
			flags.power_valid_timeout = 1;
		}
	}
}

void RTC_IRQHandler() {

	NOP
}

#include "stm32f0xx_it.h"
#include <stm32f0xx.h>
#include "main.h"
#include "usart.h"
#include "lcd_menu.h"

volatile uint8_t usart6_rx_ctr;
uint32_t usart6_crc_data;
uint8_t *usart6_cmd_frame_ptr;
volatile uint8_t usart_tx_ctr;
uint8_t *usart_ack_frame_ptr;

cmd_frame_t usart6_cmd_frame;
ack_frame_t usart6_ack_frame;

void USART3_8_IRQHandler() {

	if ((USART6->ISR & USART_ISR_RXNE)) {

		if (usart6_rx_ctr++ < CMD_FRAME_MAX_SIZE) {
			USART6->CR1 |= USART_CR1_TE;
			*usart6_cmd_frame_ptr = USART6->RDR;
			usart6_cmd_frame_ptr++;
		} else {
			USART6->RQR = USART_RQR_RXFRQ;
		}

		return;
	} else if ((USART6->ISR & USART_ISR_ORE) == USART_ISR_ORE) {
		//Overrun detection
		USART6->ICR = USART_ICR_ORECF;	//Clear overrun interrupt bit
		USART6->RQR = USART_RQR_RXFRQ;
		usart6_cmd_frame_ptr = (uint8_t*) &usart6_cmd_frame;
		return;
	} else if (USART6->ISR & USART_ISR_IDLE) {
		/*
		 * detected idle line
		 */
		usart6_cmd_frame_ptr = (uint8_t*) &usart6_cmd_frame;
		USART6->ICR = USART_ICR_IDLECF;
		flags.usart6_new_cmd = 1;
		flags.usart6_rx_busy = 0;
		usart6_rx_ctr = 0;
	}

	return;
}

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

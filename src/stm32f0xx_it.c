#include "stm32f0xx_it.h"
#include <stm32f0xx.h>
#include "main.h"
#include "usart.h"

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


#include "main.h"
#include "FreeRTOS.h"
#include "usart.h"
#include "defines.h"
#include "crc.h"

/*
 * usart6 is used to communicate with a host pc, in order
 * to receive the data from the flash
 */
void usart6_init() {

	__HAL_RCC_USART6_CLK_ENABLE()
	;

	USART6->BRR = SystemCoreClock / 1000000;

	USART6->CR1 |= USART_CR1_RE | USART_CR1_TE;
	USART6->CR1 |= USART_CR1_RXNEIE; //Reciever Interrupt Enable
	USART6->CR1 |= USART_CR1_IDLEIE; //idle line detection interrupt
	USART6->CR1 |= USART_CR1_UE;

	NVIC_SetPriority(USART3_8_IRQn, 1);
	NVIC_EnableIRQ(USART3_8_IRQn);
	return;
}

void usart6_send_data(uint8_t *ptr, uint32_t nbr) {

	USART6->CR1 |= USART_CR1_TE;
	while (nbr--) {
		while ((USART6->ISR & USART_ISR_TXE) != USART_ISR_TXE)
			;
		USART6->TDR = *ptr++;
	}
	return;
}

void usart6_send_ack_frame(ack_frame_t *ack) {

	uint32_t *crc = (uint32_t*) &((uint8_t*) ack)[ack->size - 4];

//	*crc = crc32_calc((uint8_t*) ack, ack->size - 4);

	usart6_send_data((uint8_t*) ack, ack->size);
}

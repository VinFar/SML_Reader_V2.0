#include "main.h"
#include "FreeRTOS.h"
#include "usart.h"
#include "defines.h"
#include "crc.h"

void usart1_init() {

//	RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);

	/* (1) Oversampling by 16, 9600 baud */
	/* (2) 8 data bit, 1 start bit, 1 stop bit, no parity */

	__HAL_RCC_USART1_CLK_ENABLE()
	;

	USART1->BRR = SystemCoreClock / SMART_METER_BAUDRATE; /* (1) */

	USART1->CR1 |= USART_CR1_RE; /* (2) */
	USART1->CR1 |= USART_CR1_RXNEIE; //Reciever Interrupt Enable
	USART1->CR1 |= USART_CR1_IDLEIE; //idle line detection interrupt
	USART1->CR1 |= USART_CR1_UE;
	//	USART1->CR1 &= ~USART_CR1_OVER8; //disable 16 oversampling, due to low accuracy of sml usart layer

	//Usart1 is not enabled here. it is enabled over menu by the user

	NVIC_SetPriority(USART1_IRQn, 1);
	NVIC_EnableIRQ(USART1_IRQn);
	return;
}

void usart3_init() {

//	RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);

	/* (1) Oversampling by 16, 9600 baud */
	/* (2) 8 data bit, 1 start bit, 1 stop bit, no parity */
	__HAL_RCC_USART3_CLK_ENABLE()
	;
	USART3->BRR = SystemCoreClock / SMART_METER_BAUDRATE; /* (1) */

	USART3->CR1 |= USART_CR1_RE; /* (2) */
	USART3->CR1 |= USART_CR1_RXNEIE; //Reciever Interrupt Enable
	USART3->CR1 |= USART_CR1_IDLEIE; //idle line detection interrupt
	USART3->RQR |= USART_RQR_RXFRQ;
	USART3->CR1 |= USART_CR1_UE;

	//Usart1 is not enabled here. it is enabled over menu by the user

	NVIC_SetPriority(USART3_8_IRQn, 1);
	NVIC_EnableIRQ(USART3_8_IRQn);
	return;
}

/*
 * usart6 is used to communicate with a host pc, in order
 * to receive the data from the flash
 */
void usart6_init() {

	__HAL_RCC_USART6_CLK_ENABLE()
	;

	USART6->BRR = SystemCoreClock / 9600;

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

	usart6_send_data((uint8_t*)ack,ack->size);
}

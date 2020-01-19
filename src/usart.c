#include "main.h"
#include "FreeRTOS.h"
#include "usart.h"

void usart1_init() {

//	RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);

	/* (1) Oversampling by 16, 9600 baud */
	/* (2) 8 data bit, 1 start bit, 1 stop bit, no parity */

//	USART1->BRR = SystemCoreClock / SMART_METER_BAUDRATE; /* (1) */

	USART1->CR1 |= USART_CR1_RE; /* (2) */
	USART1->CR1 |= USART_CR1_RXNEIE; //Reciever Interrupt Enable
	USART1->CR1 |= USART_CR1_IDLEIE; //idle line detection interrupt
	USART1->RQR |= USART_RQR_RXFRQ;
//	USART1->CR1 &= ~USART_CR1_OVER8; //disable 16 oversampling, due to low accuracy of sml usart layer

	//Usart1 is not enabled here. it is enabled over menu by the user

	NVIC_SetPriority(USART1_IRQn, 1);
	NVIC_EnableIRQ(USART1_IRQn);
	return;
}

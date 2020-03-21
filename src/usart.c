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

void usart5_init() {

//	RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);

	/* (1) Oversampling by 16, 9600 baud */
	/* (2) 8 data bit, 1 start bit, 1 stop bit, no parity */
	__HAL_RCC_USART5_CLK_ENABLE()
	;
	USART5->BRR = SystemCoreClock / SMART_METER_BAUDRATE; /* (1) */

	USART5->CR1 |= USART_CR1_TE; /* (2) */
	USART5->RQR |= USART_RQR_RXFRQ;
	USART5->CR1 |= USART_CR1_UE;

	//Usart1 is not enabled here. it is enabled over menu by the user

	return;
}

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

void UART_SendChar(char ch) {
	while ((USART6->ISR & USART_ISR_TXE) != USART_ISR_TXE)
		;
	USART6->TDR = ch;
}

void UART_SendInt(int32_t num) {
	char str[10]; // 10 chars max for INT32_MAX
	int i = 0;
	if (num < 0) {
		UART_SendChar('-');
		num *= -1;
	}
	do
		str[i++] = num % 10 + '0';
	while ((num /= 10) > 0);
	for (i--; i >= 0; i--)
		UART_SendChar(str[i]);
}

void UART_SendInt0(int32_t num) {
	char str[10]; // 10 chars max for INT32_MAX
	int i = 0;
	if (num < 0) {
		UART_SendChar('-');
		num *= -1;
	}
	if ((num < 10) && (num >= 0))
		UART_SendChar('0');
	do
		str[i++] = num % 10 + '0';
	while ((num /= 10) > 0);
	for (i--; i >= 0; i--)
		UART_SendChar(str[i]);
}

void UART_SendHex8(uint16_t num) {
	UART_SendChar(HEX_CHARS[(num >> 4) % 0x10]);
	UART_SendChar(HEX_CHARS[(num & 0x0f) % 0x10]);
}

void UART_SendHex16(uint16_t num) {
	uint8_t i;
	for (i = 12; i > 0; i -= 4)
		UART_SendChar(HEX_CHARS[(num >> i) % 0x10]);
	UART_SendChar(HEX_CHARS[(num & 0x0f) % 0x10]);
}

void UART_SendHex32(uint32_t num) {
	uint8_t i;
	for (i = 28; i > 0; i -= 4)
		UART_SendChar(HEX_CHARS[(num >> i) % 0x10]);
	UART_SendChar(HEX_CHARS[(num & 0x0f) % 0x10]);
}

void UART_SendStr(char *str) {
	while (*str)
		UART_SendChar(*str++);
}

void UART_SendBuf(char *buf, uint16_t bufsize) {
	USART6->CR1 |= USART_CR1_TE;
	while (bufsize--) {
		while ((USART6->ISR & USART_ISR_TXE) != USART_ISR_TXE)
			;
		USART6->TDR = *buf++;
	}
}

void UART_SendBufPrintable(char *buf, uint16_t bufsize, char subst) {
	uint16_t i;
	char ch;
	for (i = 0; i < bufsize; i++) {
		ch = *buf++;
		UART_SendChar(ch > 32 ? ch : subst);
	}
}

void UART_SendBufHex(char *buf, uint16_t bufsize) {
	uint16_t i;
	char ch;
	for (i = 0; i < bufsize; i++) {
		ch = *buf++;
		UART_SendChar(HEX_CHARS[(ch >> 4) % 0x10]);
		UART_SendChar(HEX_CHARS[(ch & 0x0f) % 0x10]);
	}
}

void UART_SendBufHexFancy(char *buf, uint16_t bufsize, uint8_t column_width,
		char subst) {
	uint16_t i = 0, len, pos;
	char buffer[column_width];

	while (i < bufsize) {
		// Line number
		UART_SendHex16(i);
		UART_SendChar(':');
		UART_SendChar(' '); // Faster and less code than UART_SendStr(": ");

		// Copy one line
		if (i + column_width >= bufsize)
			len = bufsize - i;
		else
			len = column_width;
		memcpy(buffer, &buf[i], len);

		// Hex data
		pos = 0;
		while (pos < len)
			UART_SendHex8(buffer[pos++]);
		UART_SendChar(' ');

		// Raw data
		pos = 0;
		do
			UART_SendChar(buffer[pos] > 32 ? buffer[pos] : subst);
		while (++pos < len);
		UART_SendChar('\n');

		i += len;
	}
}

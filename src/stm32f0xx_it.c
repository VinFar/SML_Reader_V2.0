#include "stm32f0xx_it.h"
#include <stm32f0xx.h>
#include "main.h"

uint32_t sml_raw_data_idx = 0;
uint8_t sml_raw_data[400]={0};

void USART1_IRQHandler() {	//Interrupt is created when transfer is completed

	if ((USART1->ISR & USART_ISR_RXNE)) {

			sml_raw_data[sml_raw_data_idx++] = USART1->RDR;
			if(sml_raw_data_idx > sizeof(sml_raw_data)){
				sml_raw_data_idx=0;
			}
	} else if ((USART1->ISR & USART_ISR_IDLE)) {
		USART1->ICR = USART_ICR_IDLECF;
		sml_raw_data_idx=0;
		flags.new_sml_packet = 1;
	}
	return;
}

void NMI_Handler(void) {
}

void HardFault_Handler(void) {
	while (1) {
	}
}


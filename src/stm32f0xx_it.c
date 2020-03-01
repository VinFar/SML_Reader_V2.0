#include "stm32f0xx_it.h"
#include <stm32f0xx.h>
#include "main.h"

uint32_t sml_main_raw_data_idx = 0;
uint8_t sml_main_raw_data[400] = { 0 };

uint32_t sml_plant_raw_data_idx = 0;
uint8_t sml_plant_raw_data[400] = { 0 };

void USART1_IRQHandler() {

	if ((USART1->ISR & USART_ISR_RXNE)) {

		/*
		 * sml_raw_data is the buffer to receive hte ~400 bytes
		 * from the smart meter, which is evaluated by the main loop later
		 */
		sml_main_raw_data[sml_main_raw_data_idx++] = USART1->RDR;
		if (sml_main_raw_data_idx > sizeof(sml_main_raw_data)) {
			sml_main_raw_data_idx = 0;
		}
	} else if ((USART1->ISR & USART_ISR_IDLE)) {
		USART1->ICR = USART_ICR_IDLECF;
		sml_main_raw_data_idx = 0;
		flags.new_main_sml_packet = 1;
	}
	return;
}

void USART3_8_IRQHandler() {

	if ((USART6->ISR & USART_ISR_RXNE)) {
		uint8_t rx = USART6->RDR;

	} else if ((USART6->ISR & USART_ISR_IDLE)) {
		USART6->ICR = USART_ICR_IDLECF;
	}
	if ((USART3->ISR & USART_ISR_RXNE)) {

		/*
		 * sml_plant_raw_data is the buffer to receive hte ~400 bytes
		 * from the smart meter, which is evaluated by the main loop later
		 */
		sml_plant_raw_data[sml_plant_raw_data_idx++] = USART3->RDR;
		if (sml_plant_raw_data_idx > sizeof(sml_plant_raw_data)) {
			sml_plant_raw_data_idx = 0;
		}
	} else if ((USART3->ISR & USART_ISR_IDLE)) {
		USART3->ICR = USART_ICR_IDLECF;
		sml_plant_raw_data_idx = 0;
		flags.new_plant_sml_packet = 1;
	}
	return;
}

void NMI_Handler(void) {
}

void HardFault_Handler(void) {
	while (1) {
	}
}


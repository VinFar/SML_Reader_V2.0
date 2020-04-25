#include "stm32f0xx_it.h"
#include <stm32f0xx.h>
#include "main.h"
#include "usart.h"
#include "nrf24.h"

uint32_t sml_main_raw_data_idx = 0;
uint8_t sml_main_raw_data[400] = { 0 };

uint32_t sml_plant_raw_data_idx = 0;
uint8_t sml_plant_raw_data[400] = { 0 };

volatile uint8_t usart6_rx_ctr;
uint32_t usart6_crc_data;
uint8_t *usart6_cmd_frame_ptr;
volatile uint8_t usart_tx_ctr;
uint8_t *usart_ack_frame_ptr;

cmd_frame_t usart6_cmd_frame;
ack_frame_t usart6_ack_frame;

void USART1_IRQHandler() {

	if ((USART1->ISR & USART_ISR_RXNE)) {

		/*
		 * sml_raw_data is the buffer to receive hte ~400 bytes
		 * from the smart meter, which is evaluated by the main loop later
		 */

		sml_main_raw_data[sml_main_raw_data_idx] = USART1->RDR;
		sml_main_raw_data_idx++;
		if (sml_main_raw_data_idx > sizeof(sml_main_raw_data)) {
			sml_main_raw_data_idx = 0;
		}
	} else if ((USART1->ISR & USART_ISR_IDLE)) {
		USART1->ICR = USART_ICR_IDLECF;
		flags.new_main_sml_packet = 1;
	} else if (USART1->ISR & USART_ISR_ORE) {
		USART1->ICR = USART_ICR_ORECF;
	}
	return;
}

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

	if ((USART3->ISR & USART_ISR_RXNE)) {

		/*
		 * sml_plant_raw_data is the buffer to receive hte ~400 bytes
		 * from the smart meter, which is evaluated by the main loop later
		 */
		sml_plant_raw_data[sml_plant_raw_data_idx++] = USART3->RDR;
		if (sml_plant_raw_data_idx > sizeof(sml_plant_raw_data)) {
			sml_plant_raw_data_idx = 0;
		}
	} else if (USART3->ISR & USART_ISR_ORE) {
		USART3->ICR = USART_ICR_ORECF;
	} else if ((USART3->ISR & USART_ISR_IDLE)) {
		USART3->ICR = USART_ICR_IDLECF;
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

uint32_t timer_ctr_for_display_tx = 0;
uint32_t timer_ctr_for_1s_flags = 0;
uint32_t timer_ctr_for_wallbox_tx = 0;
uint32_t timer_ctr_for_nrf_rx_windows = 0;

void TIM15_IRQHandler() {

	if ((TIM15->SR & TIM_SR_UIF) == TIM_SR_UIF) {	//Interrupt every 25 ms
		TIM15->SR &= ~TIM_SR_UIF;	//Reset update interrupt flag
		if (flags.display_connected == 1) {
			if (++timer_ctr_for_display_tx > 40) {
				timer_ctr_for_display_tx = 0;
				/*
				 * if the display is in rage then periodically send
				 * data
				 */

				union data_union sm[5];
				sm[NRF_IDX_SM_DATA_MAIN_METER_DEL].uint32_data =
						sm_main_current_data.meter_delivery;
				sm[NRF_IDX_SM_DATA_MAIN_METER_PUR].uint32_data =
						sm_main_current_data.meter_purchase;
				sm[NRF_IDX_SM_DATA_MAIN_POWER].uint32_data =
						sm_main_current_data.power;
				sm[NRF_IDX_SM_DATA_PLANT_DEL].uint32_data =
						sm_plant_current_data.meter_delivery;
				sm[NRF_IDX_SM_DATA_PLANT_POWER].uint32_data =
						sm_plant_current_data.power;
				nrf_add_qeue(NRF24_CMD_SM_DATA, sm, NRF_ADDR_DISP,
				NRF_DISP_PIPE);

			}
		} else {
			/*
			 * if not, ping it until it is in range again
			 */
			if (++timer_ctr_for_display_tx > 40) {
				timer_ctr_for_display_tx = 0;
				union data_union sm[2];
				sm[0].uint32_data = RTC->TR;
				sm[1].uint32_data = RTC->DR;
				nrf_add_qeue(NRF24_CMD_PING, sm, NRF_ADDR_DISP, NRF_DISP_PIPE);
			}
		}
		if (flags.wallbox_connected == 1) {
			if (++timer_ctr_for_wallbox_tx > 40) {
				timer_ctr_for_wallbox_tx = 0;
				/*
				 * if the wallbox is in rage then periodically send
				 * data
				 */
				union data_union sm[5];
				sm[NRF_IDX_SM_DATA_MAIN_METER_DEL].uint32_data =
						sm_main_current_data.meter_delivery;
				sm[NRF_IDX_SM_DATA_MAIN_METER_PUR].uint32_data =
						sm_main_current_data.meter_purchase;
				sm[NRF_IDX_SM_DATA_MAIN_POWER].uint32_data =
						sm_main_current_data.power;
				sm[NRF_IDX_SM_DATA_PLANT_DEL].uint32_data =
						sm_plant_current_data.meter_delivery;
				sm[NRF_IDX_SM_DATA_PLANT_POWER].uint32_data =
						sm_plant_current_data.power;
				nrf_add_qeue(NRF24_CMD_SM_DATA, sm, NRF_ADDR_WALLBOX,
				NRF_WALLBOX_PIPE);

			}
		} else {
			/*
			 * if not, ping it until it is in range again
			 */
			if (++timer_ctr_for_wallbox_tx > 40) {
				timer_ctr_for_wallbox_tx = 0;
				union data_union sm[2];
				sm[0].uint32_data = RTC->TR;
				sm[1].uint32_data = RTC->DR;
				nrf_add_qeue(NRF24_CMD_PING, sm, NRF_ADDR_WALLBOX,
				NRF_WALLBOX_PIPE);
			}
		}

		if (flags.nrf_rx_window) {
			if (++timer_ctr_for_nrf_rx_windows > 1) {
				timer_ctr_for_nrf_rx_windows = 0;
				nrf24_init_tx();
			}
		}

		if (++timer_ctr_for_1s_flags > 40) {
			timer_ctr_for_1s_flags = 0;
			flags.oneHz_flags = 1;
		}

	}
}

void TIM14_IRQHandler() {

	if ((TIM14->SR & TIM_SR_UIF) == TIM_SR_UIF) {	//Interrupt every x ms
		TIM14->SR &= ~TIM_SR_UIF;	//Reset update interrupt flag
		ADC1->CR |= ADC_CR_ADSTART;
	}
}

void RTC_IRQHandler() {

}

void ADC1_COMP_IRQHandler() {

	if (ADC1->ISR & ADC_ISR_EOC) {
		NOP
		ADC1->ISR = ADC_ISR_EOC;
		uint16_t dr = ADC1->DR;
//		printf("%d\r\n", dr);
//		char ch[20];
//		itoa(dr, (char*) ch, 10);
//		int i;
//		for (i = 0; i < 10; i++) {
//			if (ch[i] == '\0') {
//				ch[i] = '\r';
//				ch[i + 1] = '\n';
//				ch[i + 2] = '\0';
//				break;
//			}
//		}
//		usart6_send_data((uint8_t*) ch, i + 2);
		UNUSED(dr);

		NOP
	}
}

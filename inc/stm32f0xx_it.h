#ifndef __STM32F0xx_IT_H
#define __STM32F0xx_IT_H

#ifdef __cplusplus
 extern "C" {
#endif

#include "main.h"
#include "usart.h"

extern uint32_t sml_main_raw_data_idx;
extern uint8_t sml_main_raw_data[400];

extern uint32_t usart6_crc_data;
extern uint8_t *usart6_cmd_frame_ptr;
extern uint8_t *usart_ack_frame_ptr;
extern volatile uint8_t usart6_rx_ctr;
extern volatile uint8_t usart_tx_ctr;

extern cmd_frame_t usart6_cmd_frame;
extern ack_frame_t usart6_ack_frame;

void NMI_Handler(void);
void HardFault_Handler(void);
void SVC_Handler(void);
void SysTick_Handler(void);

#ifdef __cplusplus
}
#endif

#endif /* __STM32F0xx_IT_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

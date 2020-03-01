#ifndef __STM32F0xx_IT_H
#define __STM32F0xx_IT_H

#ifdef __cplusplus
 extern "C" {
#endif

#include "main.h"

extern uint32_t sml_main_raw_data_idx;
extern uint8_t sml_main_raw_data[400];

void NMI_Handler(void);
void HardFault_Handler(void);
void SVC_Handler(void);
void SysTick_Handler(void);

#ifdef __cplusplus
}
#endif

#endif /* __STM32F0xx_IT_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

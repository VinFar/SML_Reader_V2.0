#include "main.h"
#include "stm32f0xx_hal_gpio.h"
#include "gpio.h"
#include "nrf24_hal.h"

void gpio_init(void) {

	GPIO_InitTypeDef GPIO_InitStruct = { 0 };

	/* GPIO Ports Clock Enable */
	__HAL_RCC_GPIOC_CLK_ENABLE()
	;
	__HAL_RCC_GPIOF_CLK_ENABLE()
	;
	__HAL_RCC_GPIOA_CLK_ENABLE()
	;
	__HAL_RCC_GPIOB_CLK_ENABLE()
	;
	__HAL_RCC_GPIOD_CLK_ENABLE()
	;

	/*Configure GPIO pin Output Level */
	HAL_GPIO_WritePin(GPIOB, LED1_Pin | LED2_Pin | LED3_Pin, GPIO_PIN_RESET);

	/*Configure GPIO pin Output Level */
	HAL_GPIO_WritePin(GPIOC, FLASH_CS_Pin | NRF_CE_Pin, GPIO_PIN_RESET);

	GPIO_InitStruct.Pin = USART3_RX_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
	GPIO_InitStruct.Alternate = GPIO_AF1_USART3;
	HAL_GPIO_Init(USART3_RX_GPIO_Port, &GPIO_InitStruct);

	GPIO_InitStruct.Pin = USART1_RX_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
	GPIO_InitStruct.Alternate = GPIO_AF1_USART1;
	HAL_GPIO_Init(USART1_RX_GPIO_Port, &GPIO_InitStruct);

	GPIO_InitStruct.Pin = USART6_RX_Pin | USART6_TX_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
	GPIO_InitStruct.Alternate = GPIO_AF2_USART6;
	HAL_GPIO_Init(USART6_RX_GPIO_Port, &GPIO_InitStruct);

	/*
	 * usart5 is temporarily used to gateway the data from the main unit
	 * to the old system V1 to display it on the display placed in the
	 * kitchen
	 */
	GPIO_InitStruct.Pin = GPIO_PIN_12;
	GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
	GPIO_InitStruct.Alternate = GPIO_AF2_USART5;
	HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

	GPIO_InitStruct.Pin = COMP1_INP_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
	GPIO_InitStruct.Alternate = GPIO_AF7_COMP1;
	HAL_GPIO_Init(COMP1_INP_GPIO_Port, &GPIO_InitStruct);

	GPIO_InitStruct.Pin = COMP2_INP_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
	GPIO_InitStruct.Alternate = GPIO_AF7_COMP2;
	HAL_GPIO_Init(COMP2_INP_GPIO_Port, &GPIO_InitStruct);

	GPIO_InitStruct.Pin = COMP1_OUT_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
	GPIO_InitStruct.Alternate = GPIO_AF7_COMP1;
	HAL_GPIO_Init(COMP1_OUT_GPIO_Port, &GPIO_InitStruct);

	GPIO_InitStruct.Pin = COMP2_OUT_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
	GPIO_InitStruct.Alternate = GPIO_AF7_COMP2;
	HAL_GPIO_Init(COMP2_OUT_GPIO_Port, &GPIO_InitStruct);

	/*Configure GPIO pins : PBPin PBPin PBPin */
	GPIO_InitStruct.Pin = LED1_Pin | LED2_Pin | LED3_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

	/*Configure GPIO pin : PtPin */
	GPIO_InitStruct.Pin = NRF_IRQ_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	HAL_GPIO_Init(NRF_IRQ_GPIO_Port, &GPIO_InitStruct);

	/*Configure GPIO pin : PtPin */
	GPIO_InitStruct.Pin = SPI1_CS_NRF_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(SPI1_CS_NRF_GPIO_Port, &GPIO_InitStruct);

	/*Configure GPIO pin : PtPin */
	GPIO_InitStruct.Pin = NRF_CE_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(NRF_CE_GPIO_Port, &GPIO_InitStruct);

	nRF24_CSN_H;
	nRF24_CE_L;

	GPIO_InitStruct.Pin = ROTARY_1 | ROTARY_2;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Alternate = GPIO_AF1_TIM3;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	HAL_GPIO_Init(ROTARY_BANK, &GPIO_InitStruct);


}

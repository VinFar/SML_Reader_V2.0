/*
 * main.h
 *
 *  Created on: 23.12.2019
 *      Author: vfv13
 */

#ifndef MAIN_H_
#define MAIN_H_

#include <stdint-gcc.h>
#include "stm32f091xc.h"
#include "stm32f0xx_hal.h"

#define delay_us(us)
#define NOP asm("nop");

void Error_Handler(void);
#define RCC_OSC32_IN_Pin GPIO_PIN_14
#define RCC_OSC32_IN_GPIO_Port GPIOC
#define RCC_OSC32_OUT_Pin GPIO_PIN_15
#define RCC_OSC32_OUT_GPIO_Port GPIOC
#define RCC_HSE_IN_Pin GPIO_PIN_0
#define RCC_HSE_IN_GPIO_Port GPIOF
#define RCC_HSE_OUT_Pin GPIO_PIN_1
#define RCC_HSE_OUT_GPIO_Port GPIOF
#define USART6_TX_Pin GPIO_PIN_0
#define USART6_TX_GPIO_Port GPIOC
#define USART6_RX_Pin GPIO_PIN_1
#define USART6_RX_GPIO_Port GPIOC
#define COMP1_OUT_Pin GPIO_PIN_0
#define COMP1_OUT_GPIO_Port GPIOA
#define COMP1_INP_Pin GPIO_PIN_1
#define COMP1_INP_GPIO_Port GPIOA
#define COMP2_INP_Pin GPIO_PIN_3
#define COMP2_INP_GPIO_Port GPIOA
#define DAC_RJ2_IN_Pin GPIO_PIN_4
#define DAC_RJ2_IN_GPIO_Port GPIOA
#define DAC_RJ1_IN_Pin GPIO_PIN_5
#define DAC_RJ1_IN_GPIO_Port GPIOA
#define ADC_RJ2_OUT_Pin GPIO_PIN_6
#define ADC_RJ2_OUT_GPIO_Port GPIOA
#define COMP2_OUT_Pin GPIO_PIN_7
#define COMP2_OUT_GPIO_Port GPIOA
#define USART3_RX_Pin GPIO_PIN_5
#define USART3_RX_GPIO_Port GPIOC
#define ADC_RJ1_OUT_Pin GPIO_PIN_0
#define ADC_RJ1_OUT_GPIO_Port GPIOB
#define LED1_Pin GPIO_PIN_12
#define LED1_GPIO_Port GPIOB
#define LED2_Pin GPIO_PIN_13
#define LED2_GPIO_Port GPIOB
#define LED3_Pin GPIO_PIN_14
#define LED3_GPIO_Port GPIOB
#define FLASH_CS_Pin GPIO_PIN_9
#define FLASH_CS_GPIO_Port GPIOC
#define USART1_TX_Pin GPIO_PIN_9
#define USART1_TX_GPIO_Port GPIOA
#define USART1_RX_Pin GPIO_PIN_10
#define USART1_RX_GPIO_Port GPIOA
#define SWDIO_Pin GPIO_PIN_13
#define SWDIO_GPIO_Port GPIOA
#define SWCLK_Pin GPIO_PIN_14
#define SWCLK_GPIO_Port GPIOA
#define NRF_CE_Pin GPIO_PIN_11
#define NRF_CE_GPIO_Port GPIOC
#define NRF_IRQ_Pin GPIO_PIN_12
#define NRF_IRQ_GPIO_Port GPIOC
#define SPI1_CS_NRF_Pin GPIO_PIN_2
#define SPI1_CS_NRF_GPIO_Port GPIOD
#define SPI1_SCK_Pin GPIO_PIN_3
#define SPI1_SCK_GPIO_Port GPIOB
#define SPI1_MISO_Pin GPIO_PIN_4
#define SPI1_MISO_GPIO_Port GPIOB
#define SPI1_MOSI_Pin GPIO_PIN_5
#define SPI1_MOSI_GPIO_Port GPIOB
#define I2C1_SCL_Pin GPIO_PIN_6
#define I2C1_SCL_GPIO_Port GPIOB
#define I2C1_SDA_Pin GPIO_PIN_7
#define I2C1_SDA_GPIO_Port GPIOB


volatile typedef struct {
	unsigned new_main_sml_packet:1;
	unsigned new_plant_sml_packet:1;
	unsigned usart6_new_cmd:1;
	unsigned usart6_rx_busy:1;
}flags_t;

flags_t flags;

#include "stm32f0xx.h"

#endif /* MAIN_H_ */

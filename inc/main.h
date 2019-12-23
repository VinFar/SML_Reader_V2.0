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

#include "eeprom.h"
#include "string.h"
#include "functions.h"

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "timers.h"
#include "semphr.h"

#define delay_us(us)

void Error_Handler(void);

#define USART6_TX_Pin GPIO_PIN_0
#define USART6_TX_GPIO_Port GPIOC
#define USART6_RX_Pin GPIO_PIN_1
#define USART6_RX_GPIO_Port GPIOC
#define rotary_psuh_Pin GPIO_PIN_15
#define rotary_psuh_GPIO_Port GPIOB
#define rotary_A_Pin GPIO_PIN_6
#define rotary_A_GPIO_Port GPIOC
#define rotary_B_Pin GPIO_PIN_7
#define rotary_B_GPIO_Port GPIOC
#define CSN_NRF24_Pin GPIO_PIN_2
#define CSN_NRF24_GPIO_Port GPIOD
#define I2C1_SDA_Pin GPIO_PIN_7
#define I2C1_SDA_GPIO_Port GPIOB
#define I2C1_SCL_Pin GPIO_PIN_6
#define I2C1_SCL_GPIO_Port GPIOB


#include "stm32f0xx.h"

#endif /* MAIN_H_ */

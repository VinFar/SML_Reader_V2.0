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

#define USART6_TX_Pin GPIO_PIN_0
#define USART6_TX_GPIO_Port GPIOC
#define USART6_RX_Pin GPIO_PIN_1
#define USART6_RX_GPIO_Port GPIOC
#define ADC_IN_0_Pin GPIO_PIN_0
#define ADC_IN_0_GPIO_Port GPIOA
#define ADC_IN_1_Pin GPIO_PIN_1
#define ADC_IN_1_GPIO_Port GPIOA
#define PWM_Out_1_Pin GPIO_PIN_0
#define PWM_Out_1_GPIO_Port GPIOB
#define PWM_Out_2_Pin GPIO_PIN_1
#define PWM_Out_2_GPIO_Port GPIOB
#define Relay1_Pin GPIO_PIN_2
#define Relay1_GPIO_Port GPIOB
#define Relay2_Pin GPIO_PIN_10
#define Relay2_GPIO_Port GPIOB
#define LED1_Pin GPIO_PIN_12
#define LED1_GPIO_Port GPIOB
#define LED2_Pin GPIO_PIN_13
#define LED2_GPIO_Port GPIOB
#define LED3_Pin GPIO_PIN_14
#define LED3_GPIO_Port GPIOB
#define rotary_psuh_Pin GPIO_PIN_15
#define rotary_psuh_GPIO_Port GPIOB
#define rotary_A_Pin GPIO_PIN_6
#define rotary_A_GPIO_Port GPIOC
#define rotary_B_Pin GPIO_PIN_7
#define rotary_B_GPIO_Port GPIOC
#define CE_NRF24_Pin GPIO_PIN_11
#define CE_NRF24_GPIO_Port GPIOC
#define IRQ_NRF24_Pin GPIO_PIN_12
#define IRQ_NRF24_GPIO_Port GPIOC
#define CSN_NRF24_Pin GPIO_PIN_2
#define CSN_NRF24_GPIO_Port GPIOD
#define SPI_SCLK_Pin GPIO_PIN_3
#define SPI_SCLK_GPIO_Port GPIOB
#define SPI_MISO_Pin GPIO_PIN_4
#define SPI_MISO_GPIO_Port GPIOB
#define SPI_MOSI_Pin GPIO_PIN_5
#define SPI_MOSI_GPIO_Port GPIOB
#define I2C_SCL_Pin GPIO_PIN_6
#define I2C_SCL_GPIO_Port GPIOB
#define I2C_SDA_Pin GPIO_PIN_7
#define I2C_SDA_GPIO_Port GPIOB

#define MAX_PAYLOAD_SIZE 34

typedef union data_union {
	float float_data;
	uint32_t uint32_data;
	int32_t int32_data;
	uint16_t uint16_data[2];
	int16_t int16_data[2];
	uint8_t uint8_data[4];
	int8_t int8_data[4];
}data_union_t;

typedef union {
	uint16_t uint16_data;
	int16_t int16_data;
	uint8_t uint8_data[2];
	int8_t int8_data[2];
}uint16_8_t;

typedef struct {
	uint8_t size;
	uint8_t cmd;
	uint16_8_t major_cmd; //position
	uint16_8_t minor_cmd; //length
	data_union_t data[(MAX_PAYLOAD_SIZE/4)-2];
}__attribute__((packed)) cmd_frame_t;

cmd_frame_t cmd_frame;

#define CMD_FRAME_SIZE (sizeof(cmd_frame_t))
#define CMD_FRAME_MIN_SIZE (CMD_FRAME_SIZE - (sizeof(data_union_t)*MAX_PAYLOAD_SIZE) + 5)
#define CMD_FRAME_MAX_SIZE CMD_FRAME_SIZE

typedef struct {
	uint8_t size;
	uint8_t ack;
	data_union_t data[(MAX_PAYLOAD_SIZE/4)-2];
}__attribute__((packed)) ack_frame_t;

ack_frame_t ack_frame;

#define ACK_HEADER_SIZE (sizeof(ack_frame.ack) + sizeof(ack_frame.size))
#define ACK_LOWER_HEADER_SIZE (1 + 4)
#define ACK_FRAME_SIZE (sizeof(ack_frame))
#define ACK_FRAME_MIN_SIZE 11

#define FRAME_DELIMITER 234
#define CMD_ACK 70
#define CMD_NACK 67

enum commands{
	cmd_ping=1,

	MAX_ENUM_CMDS
};

#include "stm32f0xx.h"

#endif /* MAIN_H_ */

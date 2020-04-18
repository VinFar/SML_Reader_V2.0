#ifndef MAIN_H_
#define MAIN_H_

#include <stdint-gcc.h>
#include "stm32f091xc.h"
#include "stm32f0xx_hal.h"
#include "defines.h"
#include "gpio.h"

#define NOP asm("nop");
#define STRUCT_PACKED __attribute__((packed))
#define UUID_BASE_ADDRESS 0x1FFF7A10

extern uint32_t uptime_of_smartmeter;

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
#define ROTARY_PUSH_PIN GPIO_PIN_15
#define ROTARY_PUSH_GPIO_Port GPIOB
#define ROTARY_A_Pin GPIO_PIN_6
#define ROTARY_A_GPIO_Port GPIOC
#define ROTARY_B_Pin GPIO_PIN_7
#define ROTARY_B_GPIO_Port GPIOC
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

#define RELAY_LEFT_GPIO_PORT GPIOB
#define RELAY_LEFT_PIN GPIO_PIN_2

#define RELAY_RIGHT_GPIO_PORT GPIOB
#define RELAY_RIGHT_PIN GPIO_PIN_10


typedef volatile struct {
	unsigned refreshed_rotary :1;
	unsigned currently_in_menu :1;
	unsigned set_power_min_max :1;
	unsigned refreshed_push :1;
	unsigned lcd_light_on_off :1;
	unsigned smu_connected :1;
	unsigned nrf24_new_frame:1;
	unsigned rotary_direction:1;
} flags_t;

flags_t flags;

typedef struct uuid_struct {
	const uint32_t *id[3];
} STRUCT_PACKED uuid_t;

extern uuid_t uuid;


#include "stm32f0xx.h"

#endif /* MAIN_H_ */

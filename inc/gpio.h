#ifndef GPIO_H_
#define GPIO_H_

#include "main.h"

#define LED_ERROR_ON (LED1_GPIO_Port->ODR|=LED1_Pin)
#define LED_ERROR_OFF (LED1_GPIO_Port->ODR&=~LED1_Pin)
#define LED_ERROR_TOGGLE (LED1_GPIO_Port->ODR ^= LED1_Pin)

#define LED_OK_ON (LED2_GPIO_Port->ODR|=LED2_Pin)
#define LED_OK_OFF (LED2_GPIO_Port->ODR&=~LED2_Pin)
#define LED_OK_TOGGLE (LED2_GPIO_Port->ODR^=LED2_Pin)


#define LED_STATUS_ON (LED3_GPIO_Port->ODR|=LED3_Pin)
#define LED_STATUS_OFF (LED3_GPIO_Port->ODR&=~LED3_Pin)

#define SPI_CS_FLASH_LOW (FLASH_CS_GPIO_Port->ODR&=~FLASH_CS_Pin)
#define SPI_CS_FLASH_HIGH (FLASH_CS_GPIO_Port->ODR|=FLASH_CS_Pin)


void gpio_init(void);

#endif /* GPIO_H_ */

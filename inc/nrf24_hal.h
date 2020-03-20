#ifndef __NRF24_HAL_H
#define __NRF24_HAL_H


// Hardware abstraction layer for NRF24L01+ transceiver (hardware depended functions)
// GPIO pins definition
// GPIO pins initialization and control functions
// SPI transmit functions


// Peripheral libraries
#include "gpio.h"
#include "spi.h"


// SPI port peripheral
#define nRF24_SPI_PORT             SPI1

// CE (chip enable) pin (PB11)
#define nRF24_CE_PORT              NRF_CE_GPIO_Port
#define nRF24_CE_PIN               NRF_CE_Pin
#define nRF24_CE_L                 GPIO_RESET(nRF24_CE_PORT, nRF24_CE_PIN)
#define nRF24_CE_H                 GPIO_SET(nRF24_CE_PORT, nRF24_CE_PIN)

// CSN (chip select negative) pin (PB12)
#define nRF24_CSN_PORT             SPI1_CS_NRF_GPIO_Port
#define nRF24_CSN_PIN              SPI1_CS_NRF_Pin
#define nRF24_CSN_L                GPIO_ResetBits(nRF24_CSN_PORT, nRF24_CSN_PIN)
#define nRF24_CSN_H                GPIO_SetBits(nRF24_CSN_PORT, nRF24_CSN_PIN)

// IRQ pin (PB10)
#define nRF24_IRQ_PORT             NRF_IRQ_GPIO_Port
#define nRF24_IRQ_PIN              NRF_IRQ_Pin


// Macros for the RX on/off
#define nRF24_RX_ON                nRF24_CE_H
#define nRF24_RX_OFF               nRF24_CE_L


// Function prototypes
void nRF24_GPIO_Init(void);
uint8_t nRF24_LL_RW(uint8_t data);

#endif // __NRF24_HAL_H

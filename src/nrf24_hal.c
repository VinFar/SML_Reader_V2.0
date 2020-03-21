#include "nrf24_hal.h"
#include "main.h"
#include "stm32f0xx_hal_spi.h"
#include "spi.h"

// Low level SPI transmit/receive function (hardware depended)
// input:
//   data - value to transmit via SPI
// return: value received from SPI
uint8_t nRF24_LL_RW(uint8_t data) {
	uint8_t rx=0;
	spi_transmit_receive(&data, &rx, 1);
	return rx;
}

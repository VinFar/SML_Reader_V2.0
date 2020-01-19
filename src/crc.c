/*
 * crc.c
 *
 *  Created on: 04.06.2018
 *      Author: Nutzer
 */

#include "crc.h"
#include "main.h"

void CRC_Init() {

	__HAL_RCC_CRC_CLK_ENABLE()
	;
	delayUS_ASM(1000);

	//CRC->CR &= ~CRC_CR_POLYSIZE;	//Data width is 32Bit
	CRC->POL = 0x4C11DB7;		//Polynominal value
	CRC->INIT = 0xffffffff;

	CRC->CR |= CRC_CR_REV_OUT;
	CRC->CR |= CRC_CR_REV_IN_0;

	CRC->CR |= CRC_CR_RESET;	//Reset CRC calculator

	return;
}

uint32_t crc32_calc(uint8_t *crc_ptr, uint32_t size) {

	uint32_t idx = 0;
	uint32_t crc_data = 0;
	while (idx < size) {

		crc_data += *crc_ptr++;

		if ((idx % 4) == 3) {
			CRC->DR = crc_data;
			crc_data = 0;
		}
		crc_data <<= 8;
		idx++;
	}
	if ((size) % 4) {
		idx = size % 4;
		idx++;
		while (idx < 4) {	//fill it with zeros
			idx++;
			crc_data <<= 8;
		}
		CRC->DR = crc_data; //let the CRC Hardware compute the CRC Check
	}
	uint32_t ret = ~CRC->DR;
	CRC->CR |= CRC_CR_RESET;


	return ret;

}

/*
 * crc.h
 *
 *  Created on: 04.06.2018
 *      Author: Nutzer
 */

#ifndef CRC_H_
#define CRC_H_

#include "main.h"


void CRC_Init();
uint32_t crc32_calc(uint8_t *crc_ptr, uint32_t size);

#endif /* CRC_H_ */

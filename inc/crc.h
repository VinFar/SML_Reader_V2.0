#ifndef CRC_H_
#define CRC_H_

#include "main.h"
#include <stddef.h>
#include <stdint.h>

/*
 * #define CRC_POLY_xxxx
 *
 * The constants of the form CRC_POLY_xxxx define the polynomials for some well
 * known CRC calculations.
 */
#define		CRC_POLY_16		0xA001
#define		CRC_POLY_CCITT		0x1021
#define		CRC_START_CCITT_FFFF	0xFFFF

/*
 * #define CRC_START_xxxx
 *
 * The constants of the form CRC_START_xxxx define the values that are used for
 * initialization of a CRC value for common used calculation methods.
 */
#define		CRC_START_16		0x0000

uint16_t crc_16(const unsigned char *input_str, size_t num_bytes);
uint16_t update_crc_16(uint16_t crc, unsigned char c);
uint16_t ccrc16(char *data_p, unsigned short length);
void crc_init();
uint32_t crc32_calc(uint8_t *crc_ptr, uint32_t size);



#endif /* CRC_H_ */

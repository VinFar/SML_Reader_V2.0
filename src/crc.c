#include "main.h"
#include "crc.h"

static void init_crc16_tab(void);
uint16_t ccrc16(char *data_p, unsigned short length);

static unsigned crc_tab16_init = 0;
static uint16_t crc_tab16[256];

uint16_t crc_16(const unsigned char *input_str, size_t num_bytes) {

	uint16_t crc;
	const unsigned char *ptr;
	size_t a;

	if (!crc_tab16_init)
		init_crc16_tab();

	crc = CRC_START_CCITT_FFFF;
	ptr = input_str;

	if (ptr != NULL)
		for (a = 0; a < num_bytes; a++) {

			crc = (crc >> 8) ^ crc_tab16[(crc ^ (uint16_t) (*ptr++)) & 0x00FF];
		}

	return crc;

} /* crc_16 */

/*
 * uint16_t update_crc_16( uint16_t crc, unsigned char c );
 *
 * The function update_crc_16() calculates a new CRC-16 value based on the
 * previous value of the CRC and the next byte of data to be checked.
 */

uint16_t update_crc_16(uint16_t crc, unsigned char c) {

	if (!crc_tab16_init)
		init_crc16_tab();

	return (crc >> 8) ^ crc_tab16[(crc ^ (uint16_t) c) & 0x00FF];

} /* update_crc_16 */

/*
 * static void init_crc16_tab( void );
 *
 * For optimal performance uses the CRC16 routine a lookup table with values
 * that can be used directly in the XOR arithmetic in the algorithm. This
 * lookup table is calculated by the init_crc16_tab() routine, the first time
 * the CRC function is called.
 */

static void init_crc16_tab(void) {

	uint16_t i;
	uint16_t j;
	uint16_t crc;
	uint16_t c;

	for (i = 0; i < 256; i++) {

		crc = 0;
		c = i;

		for (j = 0; j < 8; j++) {

			if ((crc ^ c) & 0x0001)
				crc = (crc >> 1) ^ CRC_POLY_CCITT;
			else
				crc = crc >> 1;

			c = c >> 1;
		}

		crc_tab16[i] = crc;
	}

	crc_tab16_init = 1;

}

#define POLY 0x8408
/*
 //                                      16   12   5
 // this is the CCITT CRC 16 polynomial X  + X  + X  + 1.
 // This works out to be 0x1021, but the way the algorithm works
 // lets us use 0x8408 (the reverse of the bit pattern).  The high
 // bit is always assumed to be set, thus we only use 16 bits to
 // represent the 17 bit value.
 */
//CRC-16/X-25
uint16_t ccrc16(char *data_p, unsigned short length) {
	uint8_t i;
	uint16_t data;
	uint16_t crc = 0xffff;

	if (length == 0)
		return (~crc);

	do {
		for (i = 0, data = (unsigned int) 0xff & *data_p++; i < 8; i++, data >>=
				1) {
			if ((crc & 0x0001) ^ (data & 0x0001))
				crc = (crc >> 1) ^ POLY;
			else
				crc >>= 1;
		}
	} while (--length);

	crc = ~crc;
	data = crc;
	crc = (crc << 8) | (data >> 8 & 0xff);

	return (crc);
}

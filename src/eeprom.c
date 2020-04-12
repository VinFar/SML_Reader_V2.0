#include "main.h"
#include "i2c.h"
#include "eeprom.h"
#include "functions.h"

eeprom_i32_t eeprom_powermax_main = EEPROM_INIT_STRUCT(eeprom_addr_powermax_main,power_value_main_max);
eeprom_i32_t eeprom_powermin_main = EEPROM_INIT_STRUCT(eeprom_addr_powermin_main,power_value_main_min);
eeprom_i32_t eeprom_powermax_plant = EEPROM_INIT_STRUCT(eeprom_addr_powermax_plant,power_value_pant_max);
eeprom_i32_t eeprom_meanpower24h;
eeprom_i32_t eeprom_meanpower7d;
eeprom_i32_t eeprom_meanpower30d;
eeprom_i32_t eeprom_meanpower1y;

static int8_t eeprom_calc_crc8(const uint8_t *data, uint8_t length) {
	int crc = 0x00;
	int extract;
	int sum;
	for (int i = 0; i < length; i++) {
		extract = *data;
		for (char tempI = 8; tempI; tempI--) {
			sum = (crc ^ extract) & 0x01;
			crc >>= 1;
			if (sum)
				crc ^= 0x8C;
			extract >>= 1;
		}
		data++;
	}
	return crc;
}

int8_t eeprom_read_serial_numer(uint8_t *serial, uint8_t nbr) {
	if (nbr > 16) {
		/*
		 * error size is too big, abort
		 */
		return -1;
	}

	/*
	 * set address pointer
	 */

	if (i2c1_start(EEPROM_ADDRESS_SERIAL, 1, I2C_WRITE) < 0) {

		return -1;

	}
	if (i2c1_write(0x80) < 0) {

		return -1;
	}
	if (i2c1_stop() < 0) {

		return -1;
	}

	/*
	 * read 16 byte serial number
	 */
	if (i2c1_start(EEPROM_ADDRESS_SERIAL, 16, I2C_READ) < 0) {

		return -1;
	}
	for (int i = 0; i < 15; i++) {
		if (i2c1_readAck(&serial[i]) < 0) {

			return -1;
		}
	}
	if (i2c1_readNack(&serial[15]) < 0) {

		return -1;
	}

	if (i2c1_stop() < 0) {

		return -1;
	}
	return 0;

}

static int8_t eeprom_write_data_addr(uint8_t address, uint8_t *data, uint16_t size) {
	/*
	 * get correct page and byte
	 */
	uint8_t page = address / (EEPROM_MAX_BYTE - 1);
	uint8_t byte = address - (page * (EEPROM_MAX_BYTE - 1));

	/*
	 * if the read of the current page would go beyond the size of the page,
	 * we have to split it into multiply reads
	 */
	if ((address + size)
			> (((EEPROM_MAX_PAGE) * (EEPROM_MAX_BYTE - 1)))) {
		/*
		 * the requested data can't be stored in this eeprom, cause the size of the
		 * eeprom is smaller than the requested data, so abort-
		 */
		return -1;
	}
	uint8_t data_already_written = size;
	if ((byte + size) > (EEPROM_MAX_BYTE - 1)) {
		/*
		 * calculate how many bytes of the entire data is contained in the starting page and
		 * substract it from the current address to do a recursive function call with a new adress.
		 * This address will be the start of the next page.
		 * If the requested data will be also bigger than that page, the function will do a second
		 * recursive call. This goes on until the entire data is read.
		 */
		data_already_written = (EEPROM_MAX_BYTE - 1) - byte;
		uint8_t new_address = address + data_already_written;
		eeprom_write_data_addr(new_address, &data[data_already_written],
				size - data_already_written);
	}

	/*
	 * no recursive call anymore, so read the entire page at this address and
	 * copy only the neccessary data to the passed buffer.
	 */
	uint8_t page_data[15] = { 0 };

	/*
	 * read entire page to prevent overwriting of eeprom data
	 */
	eeprom_read_page(page, page_data);

	memcpy(&page_data[byte], data, data_already_written);

	eeprom_write_page(page, page_data);
	return 0;

}

static int8_t eeprom_write_page(uint8_t page, uint8_t *data) {

	if (page > 64) {
		/*
		 * this page does not exist
		 */
	}
	uint8_t crc8 = eeprom_calc_crc8(data, 15);
	data[15] = crc8;

	/*
	 * get correct device address: 1010 0 P1 P0 RW
	 */
	uint8_t address = EEPROM_ADDRESS + (page & 0b011);
	uint8_t memory_address = ((page & 0b0111100) >> 2) << 4;
	memory_address = memory_address & 0b11110000;

	if (eeprom_poll_ack_for_write(10) < 0) {
		return -1;
	}

	if (i2c1_start(address, 17, I2C_WRITE) < 0) {
		return -1;
	}
	if (i2c1_write(memory_address) < 0) {
		return -1;
	}
	for (uint8_t written = 0; written < 16; written++) {
		if (i2c1_write(data[written]) < 0) {
			return -1;
		}
	}

	if (i2c1_stop() < 0) {
		return -1;
	}

	return 0;
}

static int8_t eeprom_read_page(uint8_t page, uint8_t *data) {

	if (page > EEPROM_MAX_PAGE) {
		return -1;
	}
	if (eeprom_poll_ack_for_read(10) < 0) {
		return -1;
	}

	/*
	 * get correct device address: 1010 0 P1 P0 RW
	 */
	uint8_t address = EEPROM_ADDRESS + (page & 0b011);
	uint8_t memory_address = ((page & 0b0111100) >> 2) << 4;
	memory_address = memory_address & 0b11110000;

	/*
	 * set adress pointe in eeprom to correct adress (byte 0 of requested page)
	 * to read the entire page for checking the checksum
	 */
	if (i2c1_start(address, 1, I2C_WRITE) < 0) {
		return -1;
	}
	if (i2c1_write(memory_address) < 0) {
		return -1;
	}
	if (i2c1_stop() < 0) {
		return -1;
	}

	/*
	 * read the entire page
	 */
	uint8_t page_data[16] = { 0 };
	if (i2c1_start(address, 16, I2C_READ) < 0) {

		return -1;
	}
	for (uint8_t read = 0; read < (15); read++) {
		if (i2c1_readAck(&page_data[read]) < 0) {
			return -1;
		}
	}
	if (i2c1_readNack(&page_data[15]) < 0) {
		return -1;
	}
	if (i2c1_stop() < 0) {
		return -1;
	}

	uint8_t crc8 = eeprom_calc_crc8(page_data, 15);
#if EEPROM_OVERWRITE
	page_data[15] = crc8;
#endif
	if (crc8 != page_data[15]) {
		/*
		 * error: data is corrupted
		 */

		return -1;
	}
	/*
	 * checksum is ok
	 */

	/*
	 * copy data from temporary value to passed pointer
	 */
	memcpy(data, page_data, 15);
	return 0;

}

static int8_t eeprom_read_data_addr(uint8_t address, uint8_t *data, uint16_t size) {

	/*
	 * get correct page and byte
	 */
	uint8_t page = address / (EEPROM_MAX_BYTE - 1);
	uint8_t byte = address - (page * (EEPROM_MAX_BYTE - 1));

	/*
	 * if the read of the current page would go beyond the size of the page,
	 * we have to split it into multiply reads
	 */
	if ((address + size)
			> (((EEPROM_MAX_PAGE) * (EEPROM_MAX_BYTE - 1)))) {
		/*
		 * the requested data can't be stored in this eeprom, cause the size of the
		 * eeprom is smaller than the requested data, so abort-
		 */
		return -1;
	}
	uint8_t data_already_read = size;
	if ((byte + size) > (EEPROM_MAX_BYTE - 1)) {
		/*
		 * calculate how many bytes of the entire data is contained in the starting page and
		 * substract it from the current address to do a recursive function call with a new adress.
		 * This address will be the start of the next page.
		 * If the requested data will be also bigger than that page, the function will do a second
		 * recursive call. This goes on until the entire data is read.
		 */
		data_already_read = (EEPROM_MAX_BYTE - 1) - byte;
		uint8_t new_address = address + data_already_read;
		eeprom_read_data_addr(new_address, &data[data_already_read],
				size - data_already_read);
	}

	/*
	 * no recursive call anymore, so read the entire page at this address and
	 * copy only the neccessary data to the passed buffer.
	 */
	uint8_t page_data[15] = { 0 };

	eeprom_read_page(page, page_data);

	memcpy(data, &page_data[byte], data_already_read);
	return 0;
}

static int8_t eeprom_poll_ack_for_read(int timeout) {
	timeout = 1000;
	while (timeout-- > 0) {
		if (i2c1_start(EEPROM_ADDRESS, 1, I2C_READ) < 0) {

			i2c1_stop();
			delay_us(1000);
			/*
			 * got no ack
			 */
			continue;
		}
		uint8_t tmp_read;
		i2c1_readNack(&tmp_read);
		if (i2c1_stop() < 0) {
			continue;
		}
		/*
		 * got an ack, so go on
		 */
		return 0;

	}
	return -1;
}

static int8_t eeprom_poll_ack_for_write(int timeout) {
	timeout = 10000;
	while (timeout-- > 0) {
		if (i2c1_start(EEPROM_ADDRESS, 0, I2C_WRITE) < 0) {

			i2c1_stop();
			delay_us(1000);
			/*
			 * got no ack
			 */
			continue;
		}
		if (i2c1_stop() < 0) {
			continue;
		}
		/*
		 * got an ack, so go on
		 */
		return 0;

	}
	return -1;
}

int8_t eeprom_erase_page(uint8_t page) {

	uint32_t dummy_data[4] = { 0xdeadbeef, 0xbadeaffe, 0xbadeaffe, 0xbadeaffe };
	dummy_data[0] = 0;
	dummy_data[1] = 0;
	dummy_data[2] = 0;
	dummy_data[3] = 0;
	uint8_t crc8 = eeprom_calc_crc8((uint8_t*) dummy_data, 15);
	uint8_t page_data[16];
	memcpy(page_data, dummy_data, 16);
	page_data[15] = crc8;

	/*
	 * get correct device address: 1010 0 P1 P0 RW
	 */
	uint8_t address = EEPROM_ADDRESS + (page & 0b011);
	uint8_t memory_address = ((page & 0b0111100) >> 2) << 4;

	if (i2c1_start(address, 17, I2C_WRITE) < 0) {
		return -1;
	}
	if (i2c1_write(memory_address) < 0) {
		return -1;
	}
	for (uint8_t written = 0; written < 16; written++) {
		if (i2c1_write(page_data[written]) < 0) {
			;
			return -1;
		}
	}

	if (i2c1_stop() < 0) {
		return -1;
	}
	if (eeprom_poll_ack_for_read(10) < 0) {
		return -1;
	}

	return 0;
}

int8_t eeprom_write_data(void *data) {

	eeprom_t *ptr = (eeprom_t*) data;

	return eeprom_write_data_addr(ptr->address, ptr->data, ptr->size);
}

int8_t eeprom_read_data(void *data) {
	eeprom_t *ptr = (eeprom_t*) data;

	return eeprom_read_data_addr(ptr->address, ptr->data, ptr->size);
}

int8_t eeprom_init_data() {

	eeprom_read_data(&eeprom_powermax_main);
	eeprom_read_data(&eeprom_powermax_plant);
	eeprom_read_data(&eeprom_powermin_main);


	return 0;
}

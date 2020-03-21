#include "main.h"
#include "i2c.h"
#include "eeprom.h"
#include "functions.h"

static int8_t eeprom_poll_ack(int timeout);

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
		I2C1_RESET
		;
		return -1;

	}
	if (i2c1_write(0x80) < 0) {
		I2C1_RESET
		;
		return -1;
	}
	if (i2c1_stop() < 0) {
		I2C1_RESET
		;
		return -1;
	}

	/*
	 * read 16 byte serial number
	 */
	if (i2c1_start(EEPROM_ADDRESS_SERIAL, 16, I2C_READ) < 0) {
		I2C1_RESET
		;
		return -1;
	}
	for (int i = 0; i < 15; i++) {
		if (i2c1_readAck(&serial[i]) < 0) {
			I2C1_RESET
			;
			return -1;
		}
	}
	if (i2c1_readNack(&serial[15]) < 0) {
		I2C1_RESET
		;
		return -1;
	}

	if (i2c1_stop() < 0) {
		I2C1_RESET
		;
		return -1;
	}
	return 0;

}

int8_t eeprom_write_data_struct(void *data) {
	eeprom_t *ptr = (eeprom_t*) data;
	return eeprom_write_data(ptr->page, ptr->byte, &ptr->data, ptr->size);
}

int8_t eeprom_read_data_struct(void *data) {
	eeprom_t *ptr = (eeprom_t*) data;
	return eeprom_read_data(ptr->page, ptr->byte, &ptr->data, ptr->size);
}

int8_t eeprom_write_data(uint8_t page, uint8_t byte, uint8_t *data,
		uint8_t size) {

	if ((uint16_t) size + (uint16_t) byte > 15) {
		/*
		 * error write sequence would be out of range (16th byte is checksum)
		 */
		return -1;
	}
	if (page > 64) {
		/*
		 * this page does not exist
		 */
	}

	uint8_t page_data[16];
	if (eeprom_read_data(page, 0, page_data, 15) < 0) {
		/*
		 * something went wrong
		 */
		return -1;
	}

	for (int i = 0; i < size; i++) {
		page_data[byte + i] = data[i];
	}

	uint8_t crc8 = eeprom_calc_crc8(page_data, 15);
	page_data[15] = crc8;

	/*
	 * get correct device address: 1010 0 P1 P0 RW
	 */
	uint8_t address = EEPROM_ADDRESS + (page & 0b011);
	uint8_t memory_address = ((page & 0b0111100) >> 2) << 4;

	if (i2c1_start(address, 17, I2C_WRITE) < 0) {
		I2C1_RESET
		;
		return -1;
	}
	if (i2c1_write(memory_address) < 0) {
		I2C1_RESET
		;
		return -1;
	}
	for (uint8_t written = 0; written < 16; written++) {
		if (i2c1_write(page_data[written]) < 0) {
			I2C1_RESET
			;
			return -1;
		}
	}

	if (i2c1_stop() < 0) {
		I2C1_RESET
		;
		return -1;
	}
	if (eeprom_poll_ack(10) < 0) {
		return -1;
	}

	return 0;
}

int8_t eeprom_read_data(uint8_t page, uint8_t byte, uint8_t *data, uint8_t size) {

	if ((uint16_t) size + (uint16_t) byte > 15) {
		/*
		 * error write sequence would be out of range(16th byte is checksum)
		 */
		return -1;
	}
	if (page > 64) {
		/*
		 * this page does not exist
		 */
	}

	/*
	 * get correct device address: 1010 0 P1 P0 RW
	 */
	uint8_t address = EEPROM_ADDRESS + (page & 0b011);

	uint8_t memory_address = (((page & 0b0111100) >> 2) << 4);

	/*
	 * set adress pointer in eeprom to correct adress (byte 0 of requested page)
	 * to read the entire page for checking the checksum
	 */
	if (i2c1_start(address, 1, I2C_WRITE) < 0) {
		I2C1_RESET
		;
		return -1;
	}
	if (i2c1_write(memory_address) < 0) {
		I2C1_RESET
		;
		return -1;
	}
	if (i2c1_stop() < 0) {
		I2C1_RESET
		;
		return -1;
	}

	/*
	 * read the entire page
	 */
	uint8_t page_data[16] = { 0 };
	if (i2c1_start(address, 16, I2C_READ) < 0) {
		I2C1_RESET
		;
		return -1;
	}
	for (uint8_t read = 0; read < (15); read++) {
		if (i2c1_readAck(&page_data[read]) < 0) {
			I2C1_RESET
			;
			return -1;
		}
	}
	if (i2c1_readNack(&page_data[15]) < 0) {
		I2C1_RESET
		;
		return -1;
	}
	if (i2c1_stop() < 0) {
		I2C1_RESET
		;
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
	memcpy(data, &page_data[byte], size);
	return 0;
}

static int8_t eeprom_poll_ack(int timeout) {

	while (timeout) {
		if (i2c1_start(EEPROM_ADDRESS, 0, I2C_READ) < 0) {
			I2C1_RESET
			;
			flags.init_lcd = 0;
			timeout--;
			_delay_ms(1);
			/*
			 * got no ack
			 */
			continue;
		}
		if (i2c1_stop() < 0) {
			I2C1_RESET
			;
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
		I2C1_RESET
		;
		return -1;
	}
	if (i2c1_write(memory_address) < 0) {
		I2C1_RESET
		;
		return -1;
	}
	for (uint8_t written = 0; written < 16; written++) {
		if (i2c1_write(page_data[written]) < 0) {
			I2C1_RESET
			;
			return -1;
		}
	}

	if (i2c1_stop() < 0) {
		I2C1_RESET
		;
		return -1;
	}
	if (eeprom_poll_ack(10) < 0) {
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
	 * get correct device address: 1010 A2 A1 P0 RW
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
	if (i2c1_readNAck(&page_data[15]) < 0) {
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

static int8_t eeprom_write_page(uint8_t page, uint8_t *data) {

	if (page > (EEPROM_MAX_PAGE - 1)) {
		/*
		 * this page does not exist
		 */
	}
	uint8_t crc8 = eeprom_calc_crc8(data, 15);
	data[15] = crc8;

	/*
	 * get correct device address: 1010 A2 A1 P0 RW
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

static int8_t eeprom_write_data_addr(uint16_t address, uint8_t *data,
		uint16_t size) {
	/*
	 * get correct page and byte
	 */
	uint8_t page = address / (EEPROM_MAX_BYTE - 1);
	uint8_t byte = address - (page * (EEPROM_MAX_BYTE - 1));

	/*
	 * if the read of the current page would go beyond the size of the page,
	 * we have to split it into multiply reads
	 */
	if ((address + size) > (((EEPROM_MAX_PAGE) * (EEPROM_MAX_BYTE - 1)))) {
		/*
		 * the requested data can't be stored in this eeprom, cause the size of the
		 * eeprom is smaller than the requested data, so abort-
		 */
		return -1;
	}
	uint16_t data_written = size;
	if ((byte + size) > (EEPROM_MAX_BYTE - 1)) {
		/*
		 * calculate how many bytes of the entire data is contained in the starting page and
		 * substract it from the current address to do a recursive function call with a new adress.
		 * This address will be the start of the next page.
		 * If the requested data will be also bigger than that page, the function will do a second
		 * recursive call. This goes on until the entire data is read.
		 */
		data_written = (EEPROM_MAX_BYTE - 1) - byte;
		uint16_t new_address = address + data_written;
		eeprom_write_data_addr(new_address, &data[data_written],
				size - data_written);
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

	memcpy(&page_data[byte], data, data_written);

	eeprom_write_page(page, page_data);
	return 0;

}

static int8_t eeprom_read_data_addr(uint16_t address, uint8_t *data,
		uint16_t size) {

	/*
	 * get correct page and byte
	 */
	uint8_t page = address / (EEPROM_MAX_BYTE - 1);
	uint8_t byte = address - (page * (EEPROM_MAX_BYTE - 1));

	/*
	 * if the read of the current page would go beyond the size of the page,
	 * we have to split it into multiply reads
	 */
	if ((address + size) > (((EEPROM_MAX_PAGE) * (EEPROM_MAX_BYTE - 1)))) {
		/*
		 * the requested data can't be stored in this eeprom, cause the size of the
		 * eeprom is smaller than the requested data, so abort-
		 */
		return -1;
	}
	uint16_t data_already_read = size;
	if ((byte + size) > (EEPROM_MAX_BYTE - 1)) {
		/*
		 * calculate how many bytes of the entire data is contained in the starting page and
		 * substract it from the current address to do a recursive function call with a new adress.
		 * This address will be the start of the next page.
		 * If the requested data will be also bigger than that page, the function will do a second
		 * recursive call. This goes on until the entire data is read.
		 */
		data_already_read = (EEPROM_MAX_BYTE - 1) - byte;
		uint16_t new_address = address + data_already_read;
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


eeprom_i32_t eeprom_powermax = { EEPROM_POWERMAX_PAGE, EEPROM_POWERMAX_BYTE, 4,
		0 };
eeprom_i32_t eeprom_powermin = { EEPROM_POWERMIN_PAGE, EEPROM_POWERMIN_BYTE, 4,
		0 };
eeprom_u16_t eeprom_timemax = { EEPROM_TIMEMAX_PAGE, EEPROM_TIMEMAX_BYTE, 2, 0 };
eeprom_u16_t eeprom_timemin = { EEPROM_TIMEMIN_PAGE, EEPROM_TIMEMIN_BYTE, 2, 0 };
eeprom_i32_t eeprom_meanpower24h = { EEPROM_MEANPOWER_24H_PAGE,
EEPROM_MEANPOWER_24H_BYTE, 4, 0 };
eeprom_i32_t eeprom_meanpower7d = { EEPROM_MEANPOWER_7D_PAGE,
EEPROM_MEANPOWER_7D_BYTE, 4, 0 };
eeprom_i32_t eeprom_meanpower30d = { EEPROM_MEANPOWER_30D_PAGE,
EEPROM_MEANPOWER_30D_BYTE, 4, 0 };
eeprom_i32_t eeprom_meanpower1y = { EEPROM_MEANPOWER_1Y_PAGE,
EEPROM_MEANPOWER_1Y_BYTE, 4, 0 };

eeprom_u16_t eeprom_meantime24h = { EEPROM_MEANTIME_24H_PAGE,
EEPROM_MEANTIME_24H_BYTE, 2, 0 };
eeprom_u16_t eeprom_meantime7d = { EEPROM_MEANTIME_7D_PAGE,
EEPROM_MEANTIME_7D_BYTE, 2, 0 };
eeprom_u16_t eeprom_meantime30d = { EEPROM_MEANTIME_30D_PAGE,
EEPROM_MEANTIME_30D_BYTE, 2, 0 };
eeprom_u16_t eeprom_meantime1y = { EEPROM_MEANTIME_1Y_PAGE,
EEPROM_MEANTIME_1Y_BYTE, 2, 0 };

eeprom_u32_t eeprom_meter_delivery = { EEPROM_CONSUMPTION_DELIVERY_PAGE,
EEPROM_CONSUMPTION_DELIVERY_BYTE, 4, 0 };
eeprom_u32_t eeprom_meter_purchase = { EEPROM_CONSUMPTION_PURCHASE_PAGE,
EEPROM_CONSUMPTION_PURCHASE_BYTE, 4, 0 };
eeprom_float_t eeprom_consumption_by_system = { EEPROM_CONSUMPTION_SYSTEM_PAGE,
EEPROM_CONSUMPTION_SYSTEM_BYTE, 4, 0 };
eeprom_float_t eeprom_consumption_balance = { EEPROM_CONSUMPTION_BALANCE_PAGE,
EEPROM_CONSUMPTION_BALANCE_BYTE, 4, 0 };
eeprom_u32_t eeprom_write_ctr = { eeprom_page_63, eeprom_byte_0, 4, 0 };

eeprom_u32_t runtime_seconds;

void eeprom_init_data() {

	/*
	 * init all eeprom data
	 */
	eeprom_read_data_struct(&eeprom_consumption_balance);
	eeprom_read_data_struct(&eeprom_consumption_by_system);
	eeprom_read_data_struct(&eeprom_meanpower1y);
	eeprom_read_data_struct(&eeprom_meanpower24h);
	eeprom_read_data_struct(&eeprom_meanpower30d);
	eeprom_read_data_struct(&eeprom_meanpower7d);
	eeprom_read_data_struct(&eeprom_meantime1y);
	eeprom_read_data_struct(&eeprom_meantime24h);
	eeprom_read_data_struct(&eeprom_meantime30d);
	eeprom_read_data_struct(&eeprom_meantime7d);
	eeprom_read_data_struct(&eeprom_meter_delivery);
	eeprom_read_data_struct(&eeprom_meter_purchase);
	eeprom_read_data_struct(&eeprom_powermax);
	eeprom_read_data_struct(&eeprom_powermin);
	eeprom_read_data_struct(&eeprom_timemax);
	eeprom_read_data_struct(&eeprom_timemin);
	eeprom_read_data_struct(&eeprom_write_ctr);


	outlets[0].lock = 0;
	outlets[0].state = 0;
	outlets[0].PORT = PO_1_OUTPUT;
	outlets[0].BANK = GPIOB;
	outlets[0].prio = 1;	//
	outlets[0].eeprom_byte = EEPROM_PO_0_BYTE;
	outlets[0].eeprom_page = EEPROM_PO_0_PAGE;
	eeprom_read_data(outlets[0].eeprom_page, outlets[0].eeprom_byte,
			(uint8_t*) &outlets[0].union_value.value,
			sizeof(outlets[0].union_value.value));
	outlets[0].io_exp_output = 0;

	outlets[1].lock = 0;
	outlets[1].state = 0;
	outlets[1].PORT = PO_2_OUTPUT;
	outlets[1].BANK = GPIOB;
	outlets[1].prio = 2;
	outlets[1].eeprom_byte = EEPROM_PO_1_BYTE;
	outlets[1].eeprom_page = EEPROM_PO_1_PAGE;
	eeprom_read_data(outlets[1].eeprom_page, outlets[1].eeprom_byte,
			(uint8_t*) &outlets[1].union_value.value,
			sizeof(outlets[1].union_value.value));
	outlets[1].io_exp_output = 0;

	outlets[2].lock = 0;
	outlets[2].state = 0;
	outlets[2].PORT = PO_3_OUTPUT;
	outlets[2].BANK = GPIOB;
	outlets[2].prio = 3;
	outlets[2].eeprom_byte = EEPROM_PO_2_BYTE;
	outlets[2].eeprom_page = EEPROM_PO_2_PAGE;
	eeprom_read_data(outlets[2].eeprom_page, outlets[2].eeprom_byte,
			(uint8_t*) &outlets[2].union_value.value,
			sizeof(outlets[2].union_value.value));
	outlets[2].io_exp_output = 0;

	outlets[3].lock = 0;
	outlets[3].state = 0;
	outlets[3].PORT = PO_4_OUTPUT;
	outlets[3].BANK = GPIOB;
	outlets[3].prio = 3;
	outlets[3].eeprom_byte = EEPROM_PO_3_BYTE;
	outlets[3].eeprom_page = EEPROM_PO_3_PAGE;
	eeprom_read_data(outlets[3].eeprom_page, outlets[3].eeprom_byte,
			(uint8_t*) &outlets[3].union_value.value,
			sizeof(outlets[3].union_value.value));
	outlets[3].io_exp_output = 0;

	outlets[4].lock = 0;
	outlets[4].state = 0;
	outlets[4].PORT = PO_5_OUTPUT;
	outlets[4].BANK = GPIOB;
	outlets[4].prio = 3;
	outlets[4].eeprom_byte = EEPROM_PO_4_BYTE;
	outlets[4].eeprom_page = EEPROM_PO_4_PAGE;
	eeprom_read_data(outlets[4].eeprom_page, outlets[4].eeprom_byte,
			(uint8_t*) &outlets[4].union_value.value,
			sizeof(outlets[4].union_value.value));
	outlets[4].io_exp_output = 0;

	outlets[5].lock = 0;
	outlets[5].state = 0;
	outlets[5].PORT = PO_6_OUTPUT;
	outlets[5].BANK = GPIOB;
	outlets[5].prio = 3;
	outlets[5].eeprom_byte = EEPROM_PO_5_BYTE;
	outlets[5].eeprom_page = EEPROM_PO_5_PAGE;
	eeprom_read_data(outlets[5].eeprom_page, outlets[5].eeprom_byte,
			(uint8_t*) &outlets[5].union_value.value,
			sizeof(outlets[5].union_value.value));

	outlets[5].io_exp_output = 0;

}

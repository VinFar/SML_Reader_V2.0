#include "flash.h"
#include "spi.h"
#include "gpio.h"
#include "main.h"

void flash_send_data(uint8_t * buf, uint32_t len) {
	SPI_CS_FLASH_LOW;
	spi_transmit_receive(buf,buf,len);
	SPI_CS_FLASH_HIGH;
}

int8_t flash_init() {

	flash_reset();

	uint8_t jedec[5] = { W25N_JEDEC_ID, 0x00, 0x00, 0x00, 0x00 };
	flash_send_data(jedec, sizeof(jedec));
	if (jedec[2] == WINBOND_MAIN_ID) {
		if ((uint16_t) (jedec[3] << 8 | jedec[4]) == W25N01GV_DEV_ID) {
			flash_setStatusReg(W25N_PROT_REG, 0x00);
			flash_writeEnable();
			uint8_t reg;
			reg = flash_readStatusReg(W25N_CONFIG_REG);
			/*
			 * enable buffe read mode
			 */
			reg |= flash_config_reg_buffer_mode;
			reg &= ~flash_config_reg_status_reg1_lock;
			flash_setStatusReg(W25N_CONFIG_REG, reg);
			reg = flash_readStatusReg(W25N_PROT_REG);
			reg &= ~(flash_prot_reg_block_protect0
					| flash_prot_reg_block_protect1
					| flash_prot_reg_block_protect2
					| flash_prot_reg_block_protect3
					| flash_prot_reg_status_reg_protect1
					| flash_prot_reg_status_reg_protect0
					| flash_prot_reg_wp_pin_enable);
			flash_setStatusReg(W25N_PROT_REG, reg);
			reg = flash_readStatusReg(W25N_PROT_REG);
			return 0;
		}
	}

	return -1;
}

void flash_reset() {
	uint8_t buf[] = { W25N_RESET };
	flash_send_data(buf, sizeof(buf));
}

uint8_t flash_readStatusReg(uint8_t reg) {
	uint8_t buf[3] = { W25N_READ_STATUS_REG, reg, 0x00 };
	flash_send_data(buf, sizeof(buf));
	return buf[2];
}

void flash_setStatusReg(uint8_t reg, uint8_t set) {
	uint8_t buf[3] = { W25N_WRITE_STATUS_REG, reg, set };
	flash_send_data(buf, sizeof(buf));
}

uint32_t flash_getMaxPage() {
	return W25N_MAX_PAGE;
}

void flash_writeEnable() {
	uint8_t buf[] = { W25N_WRITE_ENABLE };
	flash_send_data(buf, sizeof(buf));
}

void flash_writeDisable() {
	uint8_t buf[] = { W25N_WRITE_DISABLE };
	flash_send_data(buf, sizeof(buf));
}

int8_t flash_blockErase(uint32_t pageAdd) {
	if (pageAdd > flash_getMaxPage()) {
		return -1;
	}
	uint8_t pageHigh = (uint8_t) ((pageAdd & 0xFF00) >> 8);
	uint8_t pageLow = (uint8_t) (pageAdd);
	uint8_t buf[4] = { W25N_BLOCK_ERASE, 0x00, pageHigh, pageLow };

	flash_writeEnable();
	flash_send_data(buf, sizeof(buf));
	return 0;
}

int8_t flash_bulkErase() {
	int error = 0;
	for (uint32_t i = 0; i < flash_getMaxPage(); i++) {
		if ((error = flash_blockErase(i)) != 0)
			return error;
	}
	return 0;
}

int8_t flash_ProgramExecute(uint32_t pageAdd) {
	if (pageAdd > flash_getMaxPage())
		return -1;
	uint8_t pageHigh = (uint8_t) ((pageAdd & 0xFF00) >> 8);
	uint8_t pageLow = (uint8_t) (pageAdd);
	flash_writeEnable();
	uint8_t buf[4] = { W25N_PROG_EXECUTE, 0x00, pageHigh, pageLow };
	flash_send_data(buf, sizeof(buf));
	return 0;
}

int8_t flash_write_data(uint32_t address, uint8_t *buf, uint32_t datalen) {
	address += W25N_START_OF_USER_PAGE * W25N_MAX_CLOUMN;
	/*
	 * the first 11 pages are factory programmed and can't be written,
	 * so step this this
	 */
	/*
	 * to write data to the flash, we first have to write the data
	 * into the internal buffer of the flash and then execute
	 * the cmd Program Execute 0x10 to write the data into the array
	 */
	/*
	 * the first 12 bytes are the byte address of the page
	 * These 12 bits are used later
	 * Here we just need the upper 16 bits to address the page
	 */
	uint16_t page = (uint16_t) (address >> 11);
	address -= page * 2048;
	uint16_t byte = (address & 0xfff);
//	flash_blockErase(page);
//	flash_block_WIP();
	flash_writeEnable();

	uint8_t cmdbuf_load_data[4];
	cmdbuf_load_data[0] = W25N_RAND_PROG_DATA_LOAD;
	cmdbuf_load_data[1] = (uint8_t) (byte >> 8);
	cmdbuf_load_data[2] = (uint8_t) (byte & 0xff);

	uint8_t cmdbuf_prog_exec[5];
	cmdbuf_prog_exec[0] = W25N_PROG_EXECUTE;
	cmdbuf_prog_exec[1] = 0;
	cmdbuf_prog_exec[2] = (uint8_t) (page >> 8);
	cmdbuf_prog_exec[3] = (uint8_t) (page & 0xff);

	SPI_CS_FLASH_LOW;
	spi_transmit(cmdbuf_load_data,3);
	spi_transmit(buf,datalen);

	SPI_CS_FLASH_HIGH;
	for (int i = 0; i < 1000; i++)
		;
	SPI_CS_FLASH_LOW;
	spi_transmit(cmdbuf_prog_exec,4);

	SPI_CS_FLASH_HIGH;

	return 0;
}

int8_t flash_read_data(uint32_t address, uint8_t *buf, uint32_t datalen) {
	address += W25N_START_OF_USER_PAGE * W25N_MAX_CLOUMN;
	/*
	 * To read data from the flash, we first have to issue the flash
	 * to load the page of the data into the internal buffer and
	 * then read the data buffer.
	 *
	 * To transfer the data into the buffer the cmd 0x13 is used
	 */
	if (address > (W25N_MAX_CLOUMN * W25N_MAX_PAGE)) {
		/*
		 * address is out of range
		 */
		return -2;
	}
	if (datalen > (W25N_MAX_CLOUMN * W25N_MAX_PAGE)) {
		/*
		 * datalen is too big
		 */
		return -2;
	}
	if ((address + datalen) > (W25N_MAX_CLOUMN * W25N_MAX_PAGE)) {
		/*
		 * address + data would be out of range
		 */
		return -2;
	}
	/*
	 * the first 12 bytes are the byte address of the page
	 * These 12 bits are used later
	 * Here we just need the upper 16 bits to address the page
	 */
	uint16_t page = (uint16_t) (address >> 11);
	address -= page * 2048;
	uint16_t byte = (address & 0xfff);

	uint8_t cmdbuf_page_data_read[4];
	cmdbuf_page_data_read[0] = W25N_PAGE_DATA_READ;
	cmdbuf_page_data_read[1] = 0; //8 dummy clocks
	cmdbuf_page_data_read[2] = (uint8_t) (page >> 8);
	cmdbuf_page_data_read[3] = (uint8_t) (page & 0xff);

	uint8_t cmdbuf_read_data[5];
	cmdbuf_read_data[0] = W25N_READ;
	cmdbuf_read_data[1] = (uint8_t) (byte >> 8);
	cmdbuf_read_data[2] = (uint8_t) (byte & 0xff);
	cmdbuf_read_data[3] = 0;

	SPI_CS_FLASH_LOW;
	/*
	 * transmit page address
	 */
	spi_transmit(cmdbuf_page_data_read,4);
	SPI_CS_FLASH_HIGH;
	flash_block_WIP();
	SPI_CS_FLASH_LOW;

	/*
	 * transmit byte address of the selected page
	 */
	spi_transmit(cmdbuf_read_data,4);
	spi_receive(buf,datalen);

	SPI_CS_FLASH_HIGH;

	return 0;
}

//Returns the Write In Progress bit from flash.
int8_t flash_check_WIP() {
	uint8_t status = flash_readStatusReg(W25N_STAT_REG);
	if (status & 0x01) {
		return 1;
	}
	return 0;
}

int8_t flash_block_WIP() {
	//Max WIP time is 10ms for block erase so 15 should be a max.
	while (flash_check_WIP())
		;
	return 0;
}

int8_t flash_address_get_plant() {
	uint32_t flash_address = W25N_START_ADDRESS_PLANT;
	for (uint32_t i = W25N_START_PAGE_PLANT; i < W25N_MAX_PAGE_PLANT;
			i++, flash_address +=
			W25N_MAX_CLOUMN) {
		uint8_t first_byte[2] = { 0 };
		flash_read_data(flash_address, first_byte, 1);
		if (first_byte[0] != BEGIN_DELIMITER) {
			/*
			 * this may be the first page that is empty.
			 * We have to check if the previous page was fully written
			 */
			flash_address -= W25N_MAX_CLOUMN;
			smartmeter_flash_data_t flash_data = { 0 };
			for (uint32_t j = 0;
					j < (W25N_MAX_CLOUMN / sizeof(smartmeter_flash_data_t));
					j++) {
				flash_read_data(
						flash_address + j * sizeof(smartmeter_flash_data_t),
						&flash_data, sizeof(flash_data));

				if (flash_data.begin != BEGIN_DELIMITER
						|| flash_data.delimiter != END_DELIMITER) {
					/*
					 * this means that the page was not fully written and is corrupted,
					 * so delete it and begin with this page
					 */
					//					flash_blockErase(flash_address / W25N_MAX_CLOUMN);
					flash_current_address_plant_sml = flash_address;
					break;
				}
			}
			/*
			 * the page was checked and is valid, so the following page is the correct page for writing
			 */
			flash_current_address_plant_sml = flash_address + W25N_MAX_CLOUMN;
			break;
		}
	}
}

int8_t flash_address_get_main() {
	/*
	 * in order to find the last page that was written before system reset,
	 * we have to search the entire flah for a non 0xFF page:
	 * 1. Read page n and search for delimiters (137,78) with and distance of 16 bytes
	 * 2. this is the last page that was written
	 */
	uint32_t flash_address = 0;
	for (uint32_t i = 0; i < W25N_MAX_PAGE_MAIN; i++, flash_address +=
	W25N_MAX_CLOUMN) {
		uint8_t first_byte[1] = { 0 };
		flash_read_data(flash_address, first_byte, 1);
		if (first_byte[0] != BEGIN_DELIMITER) {
			/*
			 * this may be the first page that is empty.
			 * We have to check if the previous page was fully written
			 */
			if (flash_address == 0) {
				flash_current_address_main_sml = 0;
				break;
			}
			flash_address -= W25N_MAX_CLOUMN;
			smartmeter_flash_data_t flash_data = { 0 };
			for (uint32_t j = 0;
					j < (W25N_MAX_CLOUMN / sizeof(smartmeter_flash_data_t));
					j++) {
				flash_read_data(
						flash_address + j * sizeof(smartmeter_flash_data_t),
						&flash_data, sizeof(flash_data));

				if (flash_data.begin != BEGIN_DELIMITER
						|| flash_data.delimiter != END_DELIMITER) {
					/*
					 * this means that the page was not fully written and is corrupted,
					 * so delete it and begin with this page
					 */
//					flash_blockErase(flash_address / W25N_MAX_CLOUMN);
					flash_current_address_main_sml = flash_address;
					break;
				}
			}
			/*
			 * the page was checked and is valid, so the following page is the correct page for writing
			 */
			flash_current_address_main_sml = flash_address + W25N_MAX_CLOUMN;
			break;
		}
	}
}

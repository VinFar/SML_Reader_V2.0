#include "flash.h"
#include "spi.h"
#include "gpio.h"
#include "main.h"

void flash_send_data(uint8_t * buf, uint32_t len) {
	SPI_CS_FLASH_LOW;
	HAL_SPI_TransmitReceive(&hspi1, buf,buf, len, 50);
	SPI_CS_FLASH_HIGH;
}

int8_t flash_init() {

	reset();

	uint8_t jedec[5] = { W25N_JEDEC_ID, 0x00, 0x00, 0x00, 0x00 };
	flash_send_data(jedec, sizeof(jedec));
	if (jedec[2] == WINBOND_MAIN_ID) {
		if ((uint16_t) (jedec[3] << 8 | jedec[4]) == W25N01GV_DEV_ID) {
			setStatusReg(W25N_PROT_REG, 0x00);
			return 0;
		}
	}
	return -1;
}

void reset() {
	uint8_t buf[] = { W25N_RESET };
	flash_send_data(buf, sizeof(buf));
}

uint8_t getStatusReg(uint8_t reg) {
	uint8_t buf[3] = { W25N_READ_STATUS_REG, reg, 0x00 };
	flash_send_data(buf, sizeof(buf));
	return buf[2];
}

void setStatusReg(uint8_t reg, uint8_t set) {
	uint8_t buf[3] = { W25N_WRITE_STATUS_REG, reg, set };
	flash_send_data(buf, sizeof(buf));
}

uint32_t getMaxPage() {
	return W25N01GV_MAX_PAGE;
}

void writeEnable() {
	uint8_t buf[] = { W25N_WRITE_ENABLE };
	flash_send_data(buf, sizeof(buf));
}

void writeDisable() {
	uint8_t buf[] = { W25N_WRITE_DISABLE };
	flash_send_data(buf, sizeof(buf));
}

int8_t blockErase(uint32_t pageAdd) {
	if (pageAdd > getMaxPage()) {
		return -1;
	}
	uint8_t pageHigh = (uint8_t) ((pageAdd & 0xFF00) >> 8);
	uint8_t pageLow = (uint8_t) (pageAdd);
	uint8_t buf[4] = { W25N_BLOCK_ERASE, 0x00, pageHigh, pageLow };
	block_WIP();
	writeEnable();
	flash_send_data(buf, sizeof(buf));
	return 0;
}

int8_t bulkErase() {
	int error = 0;
	for (uint32_t i = 0; i < getMaxPage(); i++) {
		if ((error = blockErase(i)) != 0)
			return error;
	}
	return 0;
}

int8_t loadProgData(uint16_t columnAdd, uint8_t* buf, uint32_t dataLen) {
	if (columnAdd > (uint32_t) W25N_MAX_COLUMN)
		return -1;
	if (dataLen > (uint32_t) W25N_MAX_COLUMN - columnAdd)
		return -1;
	uint8_t columnHigh = (columnAdd & 0xFF00) >> 8;
	uint8_t columnLow = columnAdd & 0xff;
	uint8_t cmdbuf[3] = { W25N_PROG_DATA_LOAD, columnHigh, columnLow };
	block_WIP();
	writeEnable();

	SPI_CS_FLASH_LOW;
	HAL_SPI_Transmit(&hspi1, cmdbuf, sizeof(cmdbuf), 50);
	HAL_SPI_Transmit(&hspi1, buf, dataLen, 50);
	SPI_CS_FLASH_HIGH;

	return 0;
}

int8_t loadRandProgData(uint16_t columnAdd, uint8_t* buf, uint32_t dataLen) {
	if (columnAdd > (uint32_t) W25N_MAX_COLUMN)
		return 1;
	if (dataLen > (uint32_t) W25N_MAX_COLUMN - columnAdd)
		return 1;
	uint8_t columnHigh = (columnAdd & 0xFF00) >> 8;
	uint8_t columnLow = columnAdd & 0xff;
	uint8_t cmdbuf[3] = { W25N_RAND_PROG_DATA_LOAD, columnHigh, columnLow };
	block_WIP();
	writeEnable();

	SPI_CS_FLASH_LOW;
	HAL_SPI_Transmit(&hspi1, cmdbuf, sizeof(cmdbuf), 50);
	HAL_SPI_Transmit(&hspi1, buf, dataLen, 50);
	SPI_CS_FLASH_HIGH;
	return 0;
}


int8_t ProgramExecute(uint32_t pageAdd) {
	if (pageAdd > getMaxPage())
		return -1;
	uint8_t pageHigh = (uint8_t) ((pageAdd & 0xFF00) >> 8);
	uint8_t pageLow = (uint8_t) (pageAdd);
	writeEnable();
	uint8_t buf[4] = { W25N_PROG_EXECUTE, 0x00, pageHigh, pageLow };
	flash_send_data(buf, sizeof(buf));
	return 0;
}

int8_t pageDataRead(uint32_t pageAdd) {
	if (pageAdd > getMaxPage())
		return -1;
	uint8_t pageHigh = (uint8_t) ((pageAdd & 0xFF00) >> 8);
	uint8_t pageLow = (uint8_t) (pageAdd);
	uint8_t buf[4] = { W25N_PAGE_DATA_READ, 0x00, pageHigh, pageLow };
	block_WIP();
	flash_send_data(buf, sizeof(buf));
	return 0;

}

int8_t read(uint16_t columnAdd, uint8_t* buf, uint32_t dataLen) {
	if (columnAdd > (uint32_t) W25N_MAX_COLUMN)
		return -1;
	if (dataLen > (uint32_t) W25N_MAX_COLUMN - columnAdd)
		return -1;
	uint8_t columnHigh = (columnAdd & 0xFF00) >> 8;
	uint8_t columnLow = columnAdd & 0xff;
	uint8_t cmdbuf[4] = { W25N_READ, columnHigh, columnLow, 0x00 };
	block_WIP();

	SPI_CS_FLASH_LOW;
	HAL_SPI_Transmit(&hspi1, cmdbuf, sizeof(cmdbuf), 50);
	HAL_SPI_Transmit(&hspi1, buf, dataLen, 50);
	SPI_CS_FLASH_HIGH;
	return 0;
}
//Returns the Write In Progress bit from flash.
int8_t check_WIP() {
	uint8_t status = getStatusReg(W25N_STAT_REG);
	if (status & 0x01) {
		return 1;
	}
	return 0;
}

int8_t check_status() {
	return (getStatusReg(W25N_STAT_REG));
}

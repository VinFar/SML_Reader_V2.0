/*
 * flash.h
 *
 *  Created on: 25.02.2020
 *      Author: Vincent
 */

#ifndef FLASH_H_
#define FLASH_H_

#include "spi.h"

#define W25M_DIE_SELECT           0xC2

#define W25N_RESET                0xFF
#define W25N_JEDEC_ID             0x9F
#define W25N_READ_STATUS_REG      0x05
#define W25N_WRITE_STATUS_REG     0x01
#define W25N_WRITE_ENABLE         0x06
#define W25N_WRITE_DISABLE        0x04
#define W25N_BB_MANAGE            0xA1
#define W25N_READ_BBM             0xA5
#define W25N_LAST_ECC_FAIL        0xA9
#define W25N_BLOCK_ERASE          0xD8
#define W25N_PROG_DATA_LOAD       0x02
#define W25N_RAND_PROG_DATA_LOAD  0x84
#define W25N_PROG_EXECUTE         0x10
#define W25N_PAGE_DATA_READ       0x13
#define W25N_READ                 0x03
#define W25N_FAST_READ            0x0B

#define W25N_PROT_REG             0xA0
#define W25N_CONFIG_REG           0xB0
#define W25N_STAT_REG             0xC0

#define WINBOND_MAIN_ID            0xEF
#define W25N01GV_DEV_ID           0xAA21

#define W25M02GV_DEV_ID           0xAB21

#define W25N_MAX_PAGE         65535
#define W25N_MAX_COLUMN_ECC           2112
#define W25N_MAX_CLOUMN				2048

enum protection_reg_bits {
	flash_prot_reg_status_reg_protect1 = 1,
	flash_prot_reg_wp_pin_enable = 2,
	flash_prot_reg_top_bottom_protect = 4,
	flash_prot_reg_block_protect0 = 8,
	flash_prot_reg_block_protect1 = 16,
	flash_prot_reg_block_protect2 = 32,
	flash_prot_reg_block_protect3 = 64,
	flash_prot_reg_status_reg_protect0 = 128,
};

enum configuration_reg_bits {
	flash_config_reg_buffer_mode = 8,
	flash_config_reg_enable_ecc = 16,
	flash_config_reg_status_reg1_lock = 32,
	flash_config_reg_enter_otp_mode = 64,
	flash_config_reg_otp_data_pages_lock = 128,
};

enum status_reg_bits {
	flash_status_reg_busy = 1,
	flash_status_reg_write_enable_latch = 2,
	flash_status_reg_erase_failure = 4,
	flash_status_reg_program_failure = 8,
	flash_status_reg_ecc_status0 = 16,
	flash_status_reg_ecc_status1 = 32,
	flash_status_reg_bbm_lut_full = 64,
};

enum chipModels {
	W25N01GV, W25M02GV
};

/* sendData(uint8_t * buf, int len) -- Sends/recieves data to the flash chip.
 * The buffer that is passed to the function will have its dat sent to the
 * flash chip, and the data recieved will be written back to that same
 * buffer. */
void sendData(uint8_t * buf, uint32_t len);

/* begin(int cs) -- initialises the flash and checks that the flash is
 * functioning and is the right model.
 * Output -- 0 if working, 1 if error*/
int8_t flash_init();

/* reset() -- resets the device. */
void flash_reset();

/* int dieSelectOnAdd(uint32_t pageAdd) -- auto changes selected die based on requested address
 * Input - full range (across all dies) page address
 * Output - error output, 0 for success
 */
int8_t dieSelectOnAdd(uint32_t pageAdd);

/* getStatusReg(uint8_t reg) -- gets the uint8_t value from one of the registers:
 * W25N_STAT_REG / W25N_CONFIG_REG / W25N_PROT_REG
 * Output -- register byte value
 */
uint8_t flash_readStatusReg(uint8_t reg);

/* setStatusReg(uint8_t reg, uint8_t set) -- Sets one of the status registers:
 * W25N_STAT_REG / W25N_CONFIG_REG / W25N_PROT_REG
 * set input -- uint8_t input to set the reg to */
void flash_setStatusReg(uint8_t reg, uint8_t set);

/* getMaxPage() Returns the max page for the given chip
 */
uint32_t flash_getMaxPage();

/* writeEnable() -- enables write opperations on the chip.
 * Is disabled after a write operation and must be recalled.
 */
void flash_writeEnable();

/* writeDisable() -- disables all write opperations on the chip */
void flash_writeDisable();

/* blockErase(uint32_t pageAdd) -- Erases one block of data on the flash chip. One block is 64 Pages, and any given
 * page address within the block will erase that block.
 * Rerturns 0 if successful
 * */
int8_t flash_blockErase(uint32_t pageAdd);

/* bulkErase() -- Erases the entire chip
 * THIS TAKES A VERY LONG TIME, ~30 SECONDS
 * Returns 0 if successful
 * */
int8_t flash_bulkErase();

/* loadRandProgData(uint16_t columnAdd, uint8_t* buf, uint32_t dataLen) -- Transfers datalen number of bytes from the
 * given buffer to the internal flash buffer, to be programed once a ProgramExecute command is sent.
 * datalLen cannot be more than the internal buffer size of 2111 bytes, or 2048 if ECC is enabled on chip.
 * When called any data in the internal buffer beforehand will be nullified.
 * WILL ERASE THE DATA IN BUF OF LENGTH DATALEN BYTES
 * */
int8_t flash_loadProgData(uint16_t columnAdd, uint8_t* buf, uint32_t dataLen);

/* loadRandProgData(uint16_t columnAdd, uint8_t* buf, uint32_t dataLen) -- Transfers datalen number of bytes from the
 * given buffer to the internal flash buffer, to be programed once a ProgramExecute command is sent.
 * datalLen cannot be more than the internal buffer size of 2111 bytes, or 2048 if ECC is enabled on chip.
 * Unlike the normal loadProgData the loadRandProgData function allows multiple transfers to the internal buffer
 * without the nulling of the currently kept data.
 * WILL ERASE THE DATA IN BUF OF LENGTH DATALEN BYTES
 */
int8_t flash_loadRandProgData(uint16_t columnAdd, uint8_t* buf,
		uint32_t dataLen);

/* ProgramExecute(uint32_t add) -- Commands the flash to program the internal buffer contents to the addres page
 * given after a loadProgData or loadRandProgData has been called.
 * The selected page needs to be erased prior to use as the falsh chip can only change 1's to 0's
 * This command will put the flash in a busy state for a time, so busy checking is required ater use.  */
int8_t flash_ProgramExecute(uint32_t pageAdd);

//pageDataRead(uint32_t add) -- Commands the flash to read from the given page address into
//its internal buffer, to be read using the read() function.
//This command will put the flash in a busy state for a time, so busy checking is required after use.
int8_t flash_pageDataRead(uint32_t pageAdd);

//read(uint16_t columnAdd, uint8_t* buf, uint32_t dataLen) -- Reads data from the flash internal buffer
//columnAdd is a buffer index (0-2047) or (0 - 2111) including ECC bits
//uint8_t* buf is a pointer to the buffer to be read into
//datalen is the length of data that should be read from the buffer (up to 2111)
int8_t flash_read(uint16_t columnAdd, uint8_t* buf, uint32_t dataLen);

//check_WIP() -- checks if the flash is busy with an operation
//Output: true if busy, false if free
int8_t flash_check_WIP();

//block_WIP() -- checks if the flash is busy and only returns once free
//Has a 15ms timeout
int8_t flash_block_WIP();

int8_t flash_check_status();

#endif /* FLASH_H_ */

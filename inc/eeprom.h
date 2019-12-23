#ifndef EEPROM_H_
#define EEPROM_H_

#include "main.h"

#define EEPROM_ADDRESS_SERIAL (0b1011000)
#define EEPROM_ADDRESS (0b1010000)

#define EEPROM_OVERWRITE 0

#define EEPROM_DATA_VALID 23
#define EEPROM_DATA_NOT_VALID 178

enum eeprom_addresses {
	eeprom_0_address=0,
};

union eeprom_union_data {
	float float_data;
	uint32_t uint32_data;
	int32_t int32_data;
	uint16_t uint16_data[2];
	int16_t int16_data[2];
	uint8_t uint8_data[4];
	int8_t int8_data[4];
};

typedef struct {
	const uint8_t address;
	const uint8_t size;
	int8_t *data;
} eeprom_i8_t;

typedef struct {
	const uint8_t address;
	const uint8_t size;
	uint8_t *data;
} eeprom_u8_t;

typedef struct {
	const uint8_t address;
	const uint8_t size;
	const int32_t *data;
} eeprom_i32_t;

typedef struct {
	const uint8_t address;
	const uint8_t size;
	const uint32_t *data;
} eeprom_u32_t;

typedef struct {
	const uint8_t address;
	const uint8_t size;
	const float *data;
} eeprom_float_t;

typedef eeprom_u8_t eeprom_t;

#define EEPROM_MAX_PAGE 64
#define EEPROM_MAX_BYTE 16

#define INIT_EEPROM_STRUCT(addr,inst) {addr,sizeof(inst),&(inst)}

int8_t eeprom_read_serial_numer(uint8_t address, uint8_t *serial, uint8_t nbr);
int8_t eeprom_write_data_struct(void *data);
int8_t eeprom_read_data_struct(void *data);
int8_t eeprom_init_data();
int8_t eeprom_write_data(void *data);
int8_t eeprom_read_data(void *data);
int8_t eeprom_erase_page(uint8_t page);


#endif /* EEPROM_H_ */

#ifndef EEPROM_H_
#define EEPROM_H_

#include "main.h"

#define EEPROM_ADDRESS_SERIAL (0b1011000)
#define EEPROM_ADDRESS (0b1010000)

#define EEPROM_DATA_VALID 23
#define EEPROM_DATA_NOT_VALID 178

#define EEPROM_MAX_PAGE 64
#define EEPROM_MAX_BYTE 16

#define INIT_EEPROM_STRUCT(addr,inst) {addr,sizeof(inst),&(inst)}

int8_t eeprom_read_serial_numer(uint8_t *serial, uint8_t nbr);
int8_t eeprom_write_data_struct(void *data);
int8_t eeprom_read_data_struct(void *data);
void eeprom_init_data();
int8_t eeprom_erase_page(uint8_t page);
void eeprom_init_data();
int8_t eeprom_read_data(uint8_t page, uint8_t byte, uint8_t *data, uint8_t size);
int8_t eeprom_write_data(uint8_t page, uint8_t byte, uint8_t *data,
		uint8_t size);


#define EEPROM_OVERWRITE 1

#define EEPROM_ADDRESS_SERIAL (0b1011000)
#define EEPROM_ADDRESS (0b1010000)

#define STRUCT_PACKED __attribute__((packed))

enum eeprom_pages {
	eeprom_page_0 = 0,
	eeprom_page_1,
	eeprom_page_2,
	eeprom_page_3,
	eeprom_page_4,
	eeprom_page_5,
	eeprom_page_6,
	eeprom_page_7,
	eeprom_page_8,
	eeprom_page_9,
	eeprom_page_10,
	eeprom_page_11,
	eeprom_page_12,
	eeprom_page_13,
	eeprom_page_14,
	eeprom_page_15,
	eeprom_page_16,
	eeprom_page_17,
	eeprom_page_18,
	eeprom_page_19,
	eeprom_page_20,
	eeprom_page_21,
	eeprom_page_22,
	eeprom_page_23,
	eeprom_page_24,
	eeprom_page_25,
	eeprom_page_26,
	eeprom_page_27,
	eeprom_page_28,
	eeprom_page_29,
	eeprom_page_30,
	eeprom_page_31,
	eeprom_page_32,
	eeprom_page_33,
	eeprom_page_34,
	eeprom_page_35,
	eeprom_page_36,
	eeprom_page_37,
	eeprom_page_38,
	eeprom_page_39,
	eeprom_page_40,
	eeprom_page_41,
	eeprom_page_42,
	eeprom_page_43,
	eeprom_page_44,
	eeprom_page_45,
	eeprom_page_46,
	eeprom_page_47,
	eeprom_page_48,
	eeprom_page_49,
	eeprom_page_50,
	eeprom_page_51,
	eeprom_page_52,
	eeprom_page_53,
	eeprom_page_54,
	eeprom_page_55,
	eeprom_page_56,
	eeprom_page_57,
	eeprom_page_58,
	eeprom_page_59,
	eeprom_page_60,
	eeprom_page_61,
	eeprom_page_62,
	eeprom_page_63,
	eeprom_max_page

};

enum eeprom_bytes {
	eeprom_byte_0 = 0,
	eeprom_byte_1,
	eeprom_byte_2,
	eeprom_byte_3,
	eeprom_byte_4,
	eeprom_byte_5,
	eeprom_byte_6,
	eeprom_byte_7,
	eeprom_byte_8,
	eeprom_byte_9,
	eeprom_byte_10,
	eeprom_byte_11,
	eeprom_byte_12,
	eeprom_byte_13,
	eeprom_byte_14,
	eeprom_byte_15,
	eeprom_max_byte

};

#define EEPROM_PAGES (eeprom_max_page-1)

#define EEPROM_BYTES (eeprom_max_byte-1)

typedef struct {
	const uint8_t page;
	const uint8_t byte;
	const uint8_t size;
	int8_t data;
} STRUCT_PACKED eeprom_i8_t;

typedef struct {
	const uint8_t page;
	const uint8_t byte;
	const uint8_t size;
	uint8_t data;
}STRUCT_PACKED  eeprom_u8_t;

typedef struct {
	const uint8_t page;
	const uint8_t byte;
	const uint8_t size;
	int32_t data;
}STRUCT_PACKED eeprom_i32_t;

typedef struct {
	const uint8_t page;
	const uint8_t byte;
	const uint8_t size;
	uint32_t data;
}STRUCT_PACKED eeprom_u32_t;

typedef struct {
	const uint8_t page;
	const uint8_t byte;
	const uint8_t size;
	uint16_t data;
}STRUCT_PACKED eeprom_u16_t;

typedef struct {
	const uint8_t page;
	const uint8_t byte;
	const uint8_t size;
	int16_t data;
}STRUCT_PACKED eeprom_i16_t;

typedef struct {
	const uint8_t page;
	const uint8_t byte;
	const uint8_t size;
	float data;
}STRUCT_PACKED eeprom_float_t;

typedef eeprom_u8_t eeprom_t;

extern eeprom_i32_t eeprom_powermax;
extern eeprom_i32_t eeprom_powermin;
extern eeprom_i32_t eeprom_meanpower24h;
extern eeprom_i32_t eeprom_meanpower7d;
extern eeprom_i32_t eeprom_meanpower30d;
extern eeprom_i32_t eeprom_meanpower1y;

extern eeprom_u16_t eeprom_meantime24h;
extern eeprom_u16_t eeprom_meantime7d;
extern eeprom_u16_t eeprom_meantime30d;
extern eeprom_u16_t eeprom_meantime1y;

extern eeprom_u32_t eeprom_meter_delivery;
extern eeprom_u32_t eeprom_meter_purchase;
extern eeprom_float_t eeprom_consumption_by_system;
extern eeprom_float_t eeprom_consumption_balance;

extern eeprom_u32_t eeprom_write_ctr;




#endif /* EEPROM_H_ */

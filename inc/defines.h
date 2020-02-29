/*
 * defines.h
 *
 *  Created on: 05.02.2020
 *      Author: Vincent
 */

#ifndef DEFINES_H_
#define DEFINES_H_

#define SMART_METER_BAUDRATE 9600

typedef struct smart_meter_struct {
	uint32_t meter_purchase;
	uint32_t meter_delivery;
	int32_t power;
	uint32_t uptime;
} smartmeter_data_t;

#define BEGIN_DELIMITER 137
#define END_DELIMITER 78

typedef struct {
	static uint8_t begin = BEGIN_DELIMITER;
	smartmeter_data_t data;
	uint32_t packet_ctr;
	static uint8_t delimiter = END_DELIMITER;
} __attribute__((aligned)) smartmeter_flash_data_t;

#endif /* DEFINES_H_ */

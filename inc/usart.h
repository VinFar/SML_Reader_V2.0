/*
 * usart.h
 *
 *  Created on: 26.12.2019
 *      Author: vfv13
 */

#ifndef USART_H_
#define USART_H_

void usart1_init();
void usart6_init();
void usart3_init();

union uint16_8 {
	uint16_t uint16_data;
	int16_t int16_data;
	uint8_t uint8_data[2];
	int8_t int8_data[2];
};

typedef union usart_data_union {
	float float_data;
	uint32_t uint32_data;
	int32_t int32_data;
	uint16_t uint16_data[2];
	int16_t int16_data[2];
	uint8_t uint8_data[4];
	int8_t int8_data[4];
} usart_data_union_t;


typedef struct {
	uint8_t size;
	uint8_t ack;
	union usart_data_union data[(10 / 4) + 1];
}__attribute__((packed)) usart_ack_frame_host_pc_t;

typedef struct {
	uint8_t size;
	uint8_t cmd;
	union uint16_8 major_cmd; //position
	union uint16_8 minor_cmd; //length
	union usart_data_union data[(10 / 4) + 2];
}__attribute__((packed)) usart_cmd_frame_t;



#endif /* USART_H_ */

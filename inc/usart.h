/*
 * usart.h
 *
 *  Created on: 26.12.2019
 *      Author: vfv13
 */

#ifndef USART_H_
#define USART_H_

#define MAX_PAYLOAD_SIZE 2048

typedef union data_union {
	float float_data;
	uint32_t uint32_data;
	int32_t int32_data;
	uint16_t uint16_data[2];
	int16_t int16_data[2];
	uint8_t uint8_data[4];
	int8_t int8_data[4];
}data_union_t;

typedef union {
	uint16_t uint16_data;
	int16_t int16_data;
	uint8_t uint8_data[2];
	int8_t int8_data[2];
}uint16_8_t;

typedef struct {
	uint8_t size;
	uint8_t cmd;
	uint16_8_t major_cmd; //position
	uint16_8_t minor_cmd; //length
	data_union_t data[(MAX_PAYLOAD_SIZE/4)+2];
}__attribute__((packed)) cmd_frame_t;

cmd_frame_t cmd_frame;

#define CMD_FRAME_SIZE (sizeof(cmd_frame_t))
#define CMD_FRAME_MIN_SIZE (CMD_FRAME_SIZE - (sizeof(data_union_t)*MAX_PAYLOAD_SIZE) + 5)
#define CMD_FRAME_MAX_SIZE CMD_FRAME_SIZE

typedef struct {
	uint16_t size;
	uint8_t ack;
	uint8_t cmd;
	data_union_t data[(MAX_PAYLOAD_SIZE/4)+4];
}__attribute__((packed)) ack_frame_t;

ack_frame_t ack_frame;

#define FRAME_DELIMITER 234
#define CMD_ACK 70
#define CMD_NACK 67

#define CMD_FRAME_MIN_SIZE 11
#define ACK_FRAME_MIN_SIZE 11
#define CMD_FRAME_MAX_SIZE (CMD_FRAME_MIN_SIZE+MAX_PAYLOAD_SIZE-4)
#define ACK_FRAME_MAX_SIZE (ACK_FRAME_MIN_SIZE+MAX_PAYLOAD_SIZE-4)


enum commands{
	CMD_PING=1,
	CMD_READ_FLASH_ADDRESS_MAIN,
	CMD_READ_FLASH_ADDRESS_PLANT,
	CMD_READ_FLASH,
	CMD_READ_PLANT_POWER,
	CMD_READ_MAIN_POWER,
	CMD_READ_UUID,
	CMD_SET_RTC,
	CMD_GET_UNIX_TIME,
	CMD_DO_BULK_ERASE,
	MAX_ENUM_CMDS
};

void usart1_init();
void usart6_init();
void usart3_init();
void usart5_init();
void usart6_send_data(uint8_t *ptr, uint32_t nbr);
void usart6_send_ack_frame(ack_frame_t *ack);

#define HEX_CHARS      "0123456789ABCDEF"

void UART_SendChar(char ch);

void UART_SendInt(int32_t num);
void UART_SendInt0(int32_t num);
void UART_SendHex8(uint16_t num);
void UART_SendHex16(uint16_t num);
void UART_SendHex32(uint32_t num);

void UART_SendStr(char *str);

void UART_SendBuf(char *buf, uint16_t bufsize);
void UART_SendBufPrintable(char *buf, uint16_t bufsize, char subst);
void UART_SendBufHex(char *buf, uint16_t bufsize);
void UART_SendBufHexFancy(char *buf, uint16_t bufsize, uint8_t column_width,
		char subst);

#endif /* USART_H_ */

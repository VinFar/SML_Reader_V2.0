#include "main.h"
#include "FreeRTOS.h"

uint16_t Log2n(uint16_t n) {
	return (n > 1) ? 1 + Log2n(n / 2) : 0;
}

int16_t isPowerOfTwo(uint16_t n) {
	return n && (!(n & (n - 1)));
}

int16_t findPosition(uint16_t n) {
	if (!isPowerOfTwo(n))
		return -1;
	return Log2n(n) + 1;
}

void check_cmd_frame() {
	if (flags.usart6_new_cmd == 1) {

		/*
		 * A new command frame was received from the Host PC and has to be evaluated.
		 */
		/*
		 * depending on the received amounts of bytes, it may be that a last CRC calculation wiht filled zeros
		 * has to be calculated
		 */
		uint8_t err = 0;
		uint8_t *del_ptr;
		del_ptr = (uint8_t*) &usart6_cmd_frame.size + (usart6_cmd_frame.size - 5);
		uint8_t size = usart6_cmd_frame.size;
		uint8_t cmd = usart6_cmd_frame.cmd;
		if (size < CMD_FRAME_MIN_SIZE) {
			/*
			 * size is not valid
			 */
			error_bit_set(error_ic_size_small);
			err = 1;
		} else {
			if (size > SIZE_MXV_DATA) {
				/*
				 * error size is too big
				 */
				error_bit_set(error_ic_size_big);
				err = 1;
			} else {

				if (*del_ptr != USART_DELIMITER) {
					/*
					 * delimiter is not valid
					 */
					error_bit_set(error_ic_no_del);
					err = 1;
				} else {
					if (cmd >= IC_MAX_ENUM_MV_CMDS) {
						/*
						 * command is not valid
						 */
						err = 1;
						error_bit_set(error_ic_cmd);
					} else {
						uint32_t crc_check =
								*(uint32_t*) ((uint8_t*) &usart_cmd_frame
										+ (usart_cmd_frame.size - 4));
						uint32_t crc_result = crc32_calc(
								(uint8_t*) &usart_cmd_frame,
								usart_cmd_frame.size - 4);
						NOP
						NOP
						if (crc_result != crc_check) {
							/*
							 * CRC32 Check is not valid
							 */
							error_bit_set(error_ic_crc32);
							err = 1;
						} else {
						}
					}
				}

			}
		}
		usart_ack_frame.ack = CMD_NACK;
		usart_ack_frame.size = ACK_FRAME_MIN_SIZE;
		uint8_t data_size = 0;
		if (!err) {
			switch (usart_cmd_frame.cmd) {
			default:
				NOP
				NOP
				break;
			}

			/*
			 * fill CRC with dummy
			 */
		}
		usart_ack_frame.size = ACK_FRAME_MIN_SIZE + data_size;
		(((uint8_t*) (&usart_ack_frame))[usart_ack_frame.size - 5]) =
		USART_DELIMITER;
		usart_cmd_frame_ptr = (uint8_t*) &usart_cmd_frame;
		usart_rx_ctr = 0;
		flags.usart_new_cmd = 0;
		USART1->CR1 |= USART_CR1_RE;
		USART1->CR1 |= USART_CR1_RXNEIE;
		usart_ack_frame.cmd = usart_cmd_frame.cmd;
		usart2_send_ack_frame(&usart_ack_frame);

	}
}



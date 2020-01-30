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

void check_cmd_struct(void *param) {

	while (1) {
		NOP
		NOP
//
//		uint32_t usart6_state_machine_error = 0;
//		uint8_t err = 0;
//		uint8_t *del_ptr;
//		uint32_t event = ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(3000));
//		if (event != 0) {
//			if (cmd_frame.size < CMD_FRAME_MIN_SIZE) {
//				/*
//				 * size is not valid
//				 */
//				err = 1;
//			} else {
//				if (cmd_frame.size > CMD_FRAME_MAX_SIZE) {
//					/*
//					 * error size is too big
//					 */
//					err = 1;
//				} else {
//					del_ptr = (uint8_t*) &cmd_frame.size + (cmd_frame.size - 5);
//
//					if (*del_ptr != FRAME_DELIMITER) {
//						/*
//						 * delimiter is not valid
//						 */
//						err = 1;
//					} else {
//						if (cmd_frame.cmd >= MAX_ENUM_CMDS
//								|| cmd_frame.cmd == 0) {
//							/*
//							 * command is not valid
//							 */
//							err = 1;
//						} else {
//							uint32_t crc_check =
//									*(uint32_t*) ((uint8_t*) &cmd_frame
//											+ (cmd_frame.size - 4));
//							uint32_t crc_result = crc32_calc(
//									(uint8_t*) &cmd_frame, cmd_frame.size - 4);
//							if (crc_result != crc_check) {
//								/*
//								 * CRC32 Check is not valid
//								 */
//								err = 1;
//							} else {
//
//							}
//						}
//					}
//
//				}
//			}
//			ack_frame.ack = CMD_NACK;
//			ack_frame.size = ACK_FRAME_MIN_SIZE;
//			if (!err) {
//				switch (cmd_frame.cmd) {
//				case cmd_ping:
//					break;
//				default:
//					break;
//				}
//			} else {
//				ack_frame.ack = CMD_NACK;
//			}
//			/*
//			 * send ack frame
//			 */
//		} else {
//			/*
//			 * communication ping timeout
//			 */
//			NOP
//		}
	}

	/*
	 * self destruction
	 */
	vTaskDelete(NULL);

}

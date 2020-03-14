/*
 * functions.h
 *
 *  Created on: 23.12.2019
 *      Author: vfv13
 */

#ifndef FUNCTIONS_H_
#define FUNCTIONS_H_

#include "main.h"


uint16_t Log2n(uint16_t n);
int16_t isPowerOfTwo(uint16_t n);
int16_t findPosition(uint16_t n);
void check_cmd_frame();
void sm_main_extract_data();
void sm_plant_extract_data();
void flash_main_store_data_in_cache(uint32_t timestamp);
void flash_plant_store_data_in_cache(uint32_t timestamp);

void check_cmd_struct(void *param);

#endif /* FUNCTIONS_H_ */

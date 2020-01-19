/*
 * dac.h
 *
 *  Created on: 24.12.2019
 *      Author: vfv13
 */

#ifndef DAC_H_
#define DAC_H_

#define DAC_I2C_ADDRESS (0b1100000)

int8_t init_dac47();
int8_t set_dac47_out1(uint16_t value);
int8_t set_dac47_out2(uint16_t value);

#endif /* DAC_H_ */

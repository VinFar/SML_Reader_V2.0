/*
 * dac.h
 *
 *  Created on: 24.12.2019
 *      Author: vfv13
 */

#ifndef DAC_H_
#define DAC_H_

#define DAC_I2C_ADDRESS (0b1100000)

void dac_init();
void dac1_set(uint16_t value);
void dac2_set(uint16_t value);

#endif /* DAC_H_ */

/*
 * dac.c
 *
 *  Created on: 24.12.2019
 *      Author: vfv13
 */

#include "main.h"
#include "dac.h"
#include "i2c.h"

int8_t init_dac47() {

	/*
	 * set voltage reference to Vref pin with buffer
	 */
	if (i2c1_start(DAC_I2C_ADDRESS, 2, I2C_WRITE) < 0) {
		return -1;
	}
	if (i2c1_write(0x08) < 0) {
		return -1;
	}
	if (i2c1_write(0b011) < 0) {
		return -1;
	}
	if (i2c1_stop() < 0) {
		return -1;
	}
	return 0;
}

int8_t set_dac47_out1(uint16_t value) {

	if (value > 1 << 12) {
		value = 1 << 12;
	}

	if (i2c1_start(DAC_I2C_ADDRESS, 3, I2C_WRITE) < 0) {
		return -1;
	}
	if (i2c1_write(0x00) < 0) {
		return -1;
	}
	if (i2c1_write(0xff & value) < 0) {
		return -1;
	}
	if (i2c1_write((0xff00 & value) >> 8)) {
		return -1;
	}
	if (i2c1_stop() < 0) {
		return -1;
	}
	return 0;

}

int8_t set_dac47_out2(uint16_t value) {

	if (value > 1 << 12) {
		value = 1 << 12;
	}

	if (i2c1_start(DAC_I2C_ADDRESS, 3, I2C_WRITE) < 0) {
		return -1;
	}
	if (i2c1_write(0x01) < 0) {
		return -1;
	}
	if (i2c1_write(0xff & value) < 0) {
		return -1;
	}
	if (i2c1_write((0xff00 & value) >> 8)) {
		return -1;
	}
	if (i2c1_stop() < 0) {
		return -1;
	}
	return 0;

}


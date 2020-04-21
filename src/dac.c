/*
 * dac.c
 *
 *  Created on: 24.12.2019
 *      Author: vfv13
 */

#include "main.h"
#include "dac.h"
#include "i2c.h"


void dac_init(){

	__HAL_RCC_DAC1_CLK_ENABLE();

	/*
	 * the dac is used to set the trigger voltage for the comparator
	 * of the internal comparator. this comparator is used to
	 * transform the incoming signal from the IR photodiode into
	 * an usart signal.
	 *
	 * Output buffer for the DACs are switched on
	 */
//	DAC->CR |= DAC_CR_EN1;
	DAC1->DHR12R1 = 1600;
	DAC->CR |= DAC_CR_EN2;
	DAC1->DHR12R2 = 2000;


}


void dac1_set(uint16_t value){
	DAC->DHR12R1 = value;

}

void dac2_set(uint16_t value){
	DAC->DHR12R2 = value;
}

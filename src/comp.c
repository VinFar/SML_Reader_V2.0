/*
 * comp.c
 *
 *  Created on: 01.03.2020
 *      Author: vfv13
 */

#include "main.h"
#include "defines.h"
#include "comp.h"

void comp_init(){

	__HAL_RCC_SYSCFG_CLK_ENABLE();

	/*
	 * comparator is used to convert the incoming signal from the
	 * photodiode into a digital signal for the usart hardware
	 *
	 * High hysterisis is used
	 * Output is not inverted
	 * positive input is the signal on the pin
	 * negative inout is either DAC or Vref/2
	 * Vrefint is typ. 1.2V
	 *
	 * polarity must be inverted for photo transistor
	 */

	/*
	 * Vref /2 as reference
	 */
	COMP->CSR |= 0b11 << COMP_CSR_COMP2HYST_Pos;
	COMP->CSR |= COMP_CSR_COMP2POL;
	COMP->CSR |= 0b001 << COMP_CSR_COMP2INSEL_Pos;;
	COMP->CSR |= COMP_CSR_COMP2EN;

	/*
	 * Vref/2 as Reference
	 */
	COMP->CSR |= 0b11 << COMP_CSR_COMP1HYST_Pos;
	COMP->CSR |= COMP_CSR_COMP1POL;
	COMP->CSR |= 0b01 << COMP_CSR_COMP1INSEL_Pos;
//	COMP->CSR |= COMP_CSR_COMP1INSEL_2;
	COMP->CSR |= COMP_CSR_COMP1EN;


}

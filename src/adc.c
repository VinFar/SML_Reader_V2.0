#include "adc.h"

void adc_init(void) {
	__HAL_RCC_ADC1_CLK_ENABLE();

	if ((ADC1->ISR & ADC_ISR_ADRDY) != 0) {
		ADC1->ISR |= ADC_ISR_ADRDY;
	}

	/*
	 * enable ADC1
	 */
	ADC1->CR |= ADC_CR_ADEN;

	/*
	 * wait until RDY bit is set
	 */
	while ((ADC1->ISR & ADC_ISR_ADRDY) == 0)
		;

	/*
	 * program the channels to scan
	 */
	ADC1->CHSELR = 	ADC_RJ1;

	/*
	 * enable continuous mode
	 */
//	ADC1->CFGR1 |= ADC_CFGR1_CONT;

	/*
	 * enable end of conversion interrupt
	 */
	ADC1->IER |= ADC_IER_EOCIE;
	NVIC_EnableIRQ(ADC1_COMP_IRQn);

	/*
	 * overrun mode to overwrite the last data by the new data
	 */
	ADC1->CR |= ADC_CFGR1_OVRMOD;

	/*
	 * sampling time to highest
	 */
	ADC1->SMPR = 0x7;

	/*
	 * start the ADC continuous conversion
	 */
	ADC1->CR |= ADC_CR_ADSTART;


}

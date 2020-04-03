#include "main.h"
#include "timer.h"
#include "stm32f0xx_hal_tim.h"


/*
 * Timer for delay function
 */
void timer16_init() {

	__HAL_RCC_TIM16_CLK_ENABLE();

	TIM16->PSC = SystemCoreClock / 1000000; //timer frequency is 1Mhz
	TIM16->CNT = 0;
	TIM16->EGR |= TIM_EGR_UG;
	TIM16->CR1 |= TIM_CR1_CEN;
	TIM16->SR &= ~TIM_SR_UIF;
	TIM16->CR1 |= TIM_CR1_CEN;

}

/*
 *
 * TIM2 is configured to generate a Interrupt exactly after 1000ms for correct Data Logging
 *
 */

void TIM2_Init() {

	__HAL_RCC_TIM2_CLK_ENABLE();

	TIM2->CR1 &= ~(TIM_CR1_CKD_1 | TIM_CR1_CKD_0);//Clock Divison on 0 tDTS = tCK_INT
	TIM2->CR1 |= TIM_CR1_URS;//Only over or underflow generates an Update-Event
	TIM2->DIER |= TIM_DIER_UIE;						//Enable Update Interrupt
	TIM2->PSC = 48000;					//Prescaler at 48000 for 1 millisecond
	TIM2->ARR = 300;					//Overflow after exactly 10 s
	TIM2->CNT = 0;
	TIM2->CR1 |= TIM_CR1_CEN;	//Enable Timer 2

	NVIC_SetPriority(TIM2_IRQn, 3);				//Set NVIC Priority on low level
	NVIC_EnableIRQ(TIM2_IRQn);						//Enable Overflow Interrupt

	return;
}


/*
 *
 * Timer 1 is configured to create an Interrupt 10ms after no USART1 Action was detected
 *
 */

void TIM_Init() {

	__HAL_RCC_TIM1_CLK_ENABLE();
	TIM1->CR1 &= ~(TIM_CR1_CKD_1 | TIM_CR1_CKD_0);//Clock Divison on 0 tDTS = tCK_INT
	TIM1->CR1 |= TIM_CR1_URS; //Only over or underflow generates an Update-Event
	TIM1->DIER = TIM_DIER_UIE;						//Enable Update Interrupt
	TIM1->PSC = (uint32_t) 48000;			//Prescaler at 440 for 1 millisecond
	TIM1->ARR = (uint32_t) 200;						//Overflow after 100ms
	TIM1->CNT = 0;
//	TIM1->CR1 |= TIM_CR1_CEN;						//Enable Timer 1
	NVIC_SetPriority(TIM1_BRK_UP_TRG_COM_IRQn, 3);//Set NVIC Priority on low level
	//Don't enable Tim1 Interrupt at first, cause it is enabled by external falling edge interrupt later
	NVIC_EnableIRQ(TIM1_BRK_UP_TRG_COM_IRQn);		//Enable Overflow Interrupt

	return;
}

/*
 * timer for rotary encoder
 */
void TIM3_Init() {

	__HAL_RCC_TIM3_CLK_ENABLE();

	//Timer 3 Encoder Interface for Rotary encoder

//	SYSCFG->EXTICR[1] |= SYSCFG_EXTICR2_EXTI7_PC;	//Map PC7 on EXTI 2 line
//	SYSCFG->EXTICR[1] |= SYSCFG_EXTICR2_EXTI6_PC;	//Map PC6 on EXTI2 Line
	EXTI->IMR |= EXTI_IMR_MR2 | EXTI_IMR_MR1;		//Don't mask TI1 and TI2

	TIM3->SMCR |= TIM_SMCR_SMS_0 ;//Configure both inputs are active on both rising and falling edges
	TIM3->CCMR1 |= TIM_CCMR1_CC1S_0 ; //Configure TI1FP1 on TI1 (CC1S = 01), configure TI1FP2 on TI2 (CC2S = 01)
	TIM3->CCER &= (uint16_t) (~(TIM_CCER_CC1P | TIM_CCER_CC2P)); // Configure TI1FP1 and TI1FP2 non inverted
	TIM3->CCER &= (uint16_t) (~(TIM_CCER_CC1NP | TIM_CCER_CC2NP));
	TIM3->SMCR |= TIM_SMCR_SMS_0 | TIM_SMCR_SMS_1; /* (3) */

	TIM3->CCMR1 |= 0b0000 << 4;				//Set Input Filters
	TIM3->CCMR1 |= 0b0000 << 12;
	TIM3->CCMR1 |= TIM_CCMR1_IC1PSC;
	TIM3->CCMR1 |= TIM_CCMR1_IC2PSC;
	TIM3->ARR = 0xffff;

	TIM3->CR1 |= TIM_CR1_CEN; /* (4) */

	TIM3->CNT = 32000;

	return;
}


void TIM6_Init() {

	__HAL_RCC_TIM16_CLK_ENABLE();

//	TIM6->ARR = 0; //interrupt every 1 second
	TIM6->PSC = 48000; //1khz
	TIM6->CR1 &= ~(TIM_CR1_CKD_1 | TIM_CR1_CKD_0); //Clock Divison on 0 tDTS = tCK_INT
	TIM6->CR1 |= TIM_CR1_URS; //Only over or underflow generates an Update-Event
//	TIM6->DIER |= TIM_DIER_UIE;						//Enable Update Interrupt
	TIM6->CNT = 0;

//	NVIC_EnableIRQ(TIM6_DAC_IRQn);

}


void TIM14_Init() {

	__HAL_RCC_TIM14_CLK_ENABLE();

	TIM14->ARR = 2000;//Auto reload register on 10s for checking correct data transmission
	TIM14->PSC = 48000;			//1us counter
//	TIM14->CR1 |= TIM_CR1_UDIS;
	TIM14->DIER |= TIM_DIER_UIE;
		TIM14->CR1 |= TIM_CR1_URS;

//	TIM14->DIER |= TIM_DIER_CC1IE; //caputer interrupt 1 enable
	TIM14->CNT = 0;
	TIM14->EGR |= TIM_EGR_UG;

	NVIC_EnableIRQ(TIM14_IRQn);
	return;

}

void TIM15_Init() {

	__HAL_RCC_TIM15_CLK_ENABLE()
	;
	delay_us(10);
	TIM15->CR1 |= TIM_CR1_URS;	//Only overflow generates an itnerrupt
	TIM15->DIER |= TIM_DIER_UIE;	//Update interrupt enable
	TIM15->PSC = (SystemCoreClock)/1000000;			//Prescaler at 108 for 1us tick
	TIM15->ARR = 25000;//Auto reload register at 25000 for 25ms interrupt

	NVIC_EnableIRQ(TIM15_IRQn);
	NVIC_SetPriority(TIM15_IRQn, 5);

	TIM15->CNT = 0;
	TIM15->CR1 |= TIM_CR1_CEN;


}

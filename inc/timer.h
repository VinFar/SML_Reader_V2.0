#ifndef TIMER_H_
#define TIMER_H_

#include "main.h"

#define TIM17_DISABLE TIM17->CR1 &= ~TIM_CR1_CEN
#define TIM17_ENABLE TIM17->CR1 |= TIM_CR1_CEN

#define TIM15_DISABLE (TIM15->CR1 &= ~TIM_CR1_CEN)
#define TIM15_ENABLE (TIM15->CR1 |= TIM_CR1_CEN)


void timer16_init();
void TIM_Init();
void TIM2_Init();
void TIM3_Init();
void TIM6_Init();
void TIM14_Init();
void TIM15_Init();
void TIM17_Init();

#endif /* TIMER_H_ */

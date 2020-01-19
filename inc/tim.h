#ifndef __tim_H
#define __tim_H
#ifdef __cplusplus
 extern "C" {
#endif

#include "main.h"

void tim1_init(void);
void tim3_init(void);
void tim14_init(void);

void pwm2_set_dutycycle(float duty);
void pwm1_set_dutycycle(float duty);
                        
void HAL_TIM_MspPostInit(TIM_HandleTypeDef *htim);

#ifdef __cplusplus
}
#endif
#endif /*__ tim_H */

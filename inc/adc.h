#ifndef __adc_H
#define __adc_H
#ifdef __cplusplus
 extern "C" {
#endif
#include "main.h"


extern ADC_HandleTypeDef hadc;

void adc_init(void);

#ifdef __cplusplus
}
#endif
#endif


#ifndef RTC_H_
#define RTC_H_

#include "main.h"
#include "stm32f0xx_hal_rtc.h"

#define JULIAN_DATE_BASE     2440588   // Unix epoch time in Julian calendar (UnixTime = 00:00:00 01.01.1970 => JDN = 2440588)

static const uint16_t week_day[] = { 0x4263, 0xA8BD, 0x42BF, 0x4370, 0xABBF, 0xA8BF, 0x43B2 };

extern RTC_TimeTypeDef sm_time;
extern RTC_DateTypeDef sm_date;

extern uint32_t rtc_current_time_unix;
extern uint32_t rtc_old_time_unix;


#define RTC_Format_BIN                    ((uint32_t)0x000000000)
#define RTC_Format_BCD                    ((uint32_t)0x000000001)

#define RTC_DISABLE_WP RTC->WPR = 0xCA;\
						RTC->WPR = 0x53

#define RTC_ENABLE_WP RTC->WPR = 0xFF

#define RTC_INIT_WAIT RTC->ISR |= RTC_ISR_INIT;\
						while ((RTC->ISR & RTC_ISR_INITF) == 0){}


uint32_t rtc_get_unix_time(RTC_TimeTypeDef *time, RTC_DateTypeDef *date);
void RTC_GetDate(uint32_t RTC_Format, RTC_DateTypeDef* RTC_DateStruct);
void RTC_GetTime(uint32_t RTC_Format, RTC_TimeTypeDef* RTC_TimeStruct);
void rtc_init();

#endif /* RTC_H_ */

#include "rtc.h"
#include "main.h"
#include "stm32f0xx_hal_rtc.h"

RTC_TimeTypeDef sm_time;
RTC_DateTypeDef sm_date;

static uint32_t rtc_current_time_unix;
static uint32_t rtc_old_time_unix;

void rtc_init() {
	__HAL_RCC_RTC_ENABLE();
	/*
	 * at first we have to disable the write protection of
	 * the RTC registers by writing 0xCA followed by 0x53 into
	 * the WPR Register
	 */
	RTC->WPR = 0xCA;
	RTC->WPR = 0x53;

	/*
	 * Enter init mode by setting th init bit in the ISR Register
	 */
	RTC->ISR |= RTC_ISR_INIT;
	while ((RTC->ISR & RTC_ISR_INITF) == 0)
		;

	/*
	 * load time and date
	 * and set 24h format
	 */
//	RTC->TR =
	RTC->CR &= ~RTC_CR_FMT;

	/*
	 * exit init mode by clearing the init bit and enable write protection
	 */
	RTC->ISR &= ~RTC_ISR_INIT;

	while ((RTC->ISR & RTC_ISR_RECALPF) == RTC_ISR_RECALPF)
		;

	RTC->CALR = RTC_CALR_CALP | 1;
	RTC->CALR = RTC_CALR_CALM | 639;

	RTC->WPR = 0xFE;
	RTC->WPR = 0x64;

	NVIC_EnableIRQ(RTC_IRQn);

	rtc_calc_new_time();

}

// Convert Date/Time structures to epoch time
uint32_t rtc_get_unix_time(RTC_TimeTypeDef *time, RTC_DateTypeDef *date) {
	uint8_t a;
	uint16_t y;
	uint8_t m;
	uint32_t JDN;

	// These hardcore math's are taken from http://en.wikipedia.org/wiki/Julian_day

	// Calculate some coefficients
	a = (14 - date->Month) / 12;
	y = (date->Year + 2000) + 4800 - a; // years since 1 March, 4801 BC
	m = date->Month + (12 * a) - 3; // since 1 March, 4801 BC

	// Gregorian calendar date compute
	JDN = date->Date;
	JDN += (153 * m + 2) / 5;
	JDN += 365 * y;
	JDN += y / 4;
	JDN += -y / 100;
	JDN += y / 400;
	JDN = JDN - 32045;
	JDN = JDN - JULIAN_DATE_BASE;    // Calculate from base date
	JDN *= 86400;                     // Days to seconds
	JDN += time->Hours * 3600;    // ... and today seconds
	JDN += time->Minutes * 60;
	JDN += time->Seconds;

	return JDN;
}

void RTC_GetTime(uint32_t RTC_Format, RTC_TimeTypeDef *RTC_TimeStruct) {
	uint32_t tmpreg = 0;

	/* Check the parameters */
	assert_param(IS_RTC_FORMAT(RTC_Format));

	/* Get the RTC_TR register */
	tmpreg = (uint32_t) (RTC->TR & RTC_TR_RESERVED_MASK);

	/* Fill the structure fields with the read parameters */
	RTC_TimeStruct->Hours =
			(uint8_t) ((tmpreg & (RTC_TR_HT | RTC_TR_HU)) >> 16);
	RTC_TimeStruct->Minutes = (uint8_t) ((tmpreg & (RTC_TR_MNT | RTC_TR_MNU))
			>> 8);
	RTC_TimeStruct->Seconds = (uint8_t) (tmpreg & (RTC_TR_ST | RTC_TR_SU));
	RTC_TimeStruct->TimeFormat = (uint8_t) ((tmpreg & (RTC_TR_PM)) >> 16);

	/* Check the input parameters format */
	if (RTC_Format == RTC_Format_BIN) {
		/* Convert the structure parameters to Binary format */
		RTC_TimeStruct->Hours = (uint8_t) RTC_Bcd2ToByte(RTC_TimeStruct->Hours);
		RTC_TimeStruct->Minutes = (uint8_t) RTC_Bcd2ToByte(
				RTC_TimeStruct->Minutes);
		RTC_TimeStruct->Seconds = (uint8_t) RTC_Bcd2ToByte(
				RTC_TimeStruct->Seconds);
	}
}

void RTC_GetDate(uint32_t RTC_Format, RTC_DateTypeDef *RTC_DateStruct) {
	uint32_t tmpreg = 0;

	/* Check the parameters */
	assert_param(IS_RTC_FORMAT(RTC_Format));

	/* Get the RTC_TR register */
	tmpreg = (uint32_t) (RTC->DR & RTC_DR_RESERVED_MASK);

	/* Fill the structure fields with the read parameters */
	RTC_DateStruct->Year = (uint8_t) ((tmpreg & (RTC_DR_YT | RTC_DR_YU)) >> 16);
	RTC_DateStruct->Month = (uint8_t) ((tmpreg & (RTC_DR_MT | RTC_DR_MU)) >> 8);
	RTC_DateStruct->Date = (uint8_t) (tmpreg & (RTC_DR_DT | RTC_DR_DU));
	RTC_DateStruct->WeekDay = (uint8_t) ((tmpreg & (RTC_DR_WDU)) >> 13);

	/* Check the input parameters format */
	if (RTC_Format == RTC_Format_BIN) {
		/* Convert the structure parameters to Binary format */
		RTC_DateStruct->Year = (uint8_t) RTC_Bcd2ToByte(RTC_DateStruct->Year);
		RTC_DateStruct->Month = (uint8_t) RTC_Bcd2ToByte(RTC_DateStruct->Month);
		RTC_DateStruct->Date = (uint8_t) RTC_Bcd2ToByte(RTC_DateStruct->Date);
		RTC_DateStruct->WeekDay = (uint8_t) (RTC_DateStruct->WeekDay);
	}
}

uint32_t rtc_calc_new_time() {
	RTC_GetTime(RTC_FORMAT_BIN, &sm_time);
	RTC_GetDate(RTC_FORMAT_BIN, &sm_date);
	rtc_current_time_unix = rtc_get_unix_time(&sm_time, &sm_date);
	return rtc_current_time_unix;
}

uint32_t rtc_get_current_unix_time() {
	return rtc_current_time_unix;
}

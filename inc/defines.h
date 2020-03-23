/*
 * defines.h
 *
 *  Created on: 05.02.2020
 *      Author: Vincent
 */

#ifndef DEFINES_H_
#define DEFINES_H_

#include "main.h"

#define SMART_METER_BAUDRATE 9600

#define ARRAY_LEN(x) (sizeof(x) / sizeof(x[0]))

typedef struct smart_meter_struct {
	uint32_t meter_purchase;
	uint32_t meter_delivery;
	int32_t power;
	uint32_t uptime;
}__attribute__((packed)) smartmeter_data_t;

#define BEGIN_DELIMITER 137
#define END_DELIMITER 78

typedef struct {
	uint8_t begin;
	smartmeter_data_t data;
	uint8_t delimiter;
} __attribute__((packed)) smartmeter_flash_data_t;

/*
 *
 * Flags for Menu
 *
 */
#define NUMBER_OF_ITEMS_MAIN_MENU 6

//Define LED pins
#define LED_OK GPIO_PIN_11
#define LED_ERROR GPIO_PIN_8
#define LED_FAULT GPIO_PIN_9

#define LED_OK_BANK GPIOA
#define LED_ERROR_BANK GPIOA
#define LED_FAULT_BANK GPIOA

//Number of connected outlets
#define NUMBER_OF_OUTLETS 6

#define PO_1_OUTPUT GPIO_PIN_8 //PO_3_OUTPUT is PC10
#define PO_2_OUTPUT GPIO_PIN_9	//PO_2_OUTPUT is PC11
#define PO_3_OUTPUT GPIO_PIN_10	//PO_1_OUTPUT is PC12
#define PO_4_OUTPUT GPIO_PIN_11	//PO_1_OUTPUT is PC12
#define PO_5_OUTPUT GPIO_PIN_12	//PO_1_OUTPUT is PC12
#define PO_6_OUTPUT GPIO_PIN_13	//PO_1_OUTPUT is PC12


#define SML_ERROR -1
#define SML_SUCCESS 0


/*
 * Configure the Parity Bit of your Smart Meter
 * PARITY: 0=No Parity, 1=Parity
 * E_O_PARITY: 0=Even Parity, 1=Odd Parity
 *
 */
#define PARITY 0
#define E_O_PARITY 0

/*
 * This Table defines the individually Power Ooutlets.
 * PO_x defines the actual consumed Power of this Outlet, when it is
 * switched on.
 * PO_x_PRIO stands for the Priority of this Outlet.
 * 0 stands for the highest Priority.
 *
 *
 * With MAX_CONSUMED_PWR you can define if you want to switch on a Outlet
 * even though there is not enough Power from the Solar Panels.
 * Example:
 * PO_1 700
 *
 * So PO_1 is switched on if the avaiable Power is 600W. So you buy 100W
 * from the Provider.
 * Put this on -1 if you don't want this configured.
 */

#define MAX_CONSUMED_PWR 100
#define PO_1_PRIO 0
#define PO_2_PRIO 1
#define PO_3_PRIO 2

#define TOGGLE_LED_OK GPIOA->ODR ^= LED_OK
#define TOGGLE_LED_ERROR GPIOA->ODR ^= LED_ERROR
#define TOGGLE_LED_FAULT GPIOA->ODR ^= LED_FAULT

#define LED_OK_OFF GPIOA->ODR &= ~LED_OK
#define LED_ERROR_OFF GPIOA->ODR &= ~LED_ERROR; \
		io_exp_set_output(IO_EXP_ERROR_LED, 0)

#define LED_FAULT_OFF GPIOA->ODR &= ~LED_FAULT

#define LED_OK_ON GPIOA->ODR |= LED_OK
#define LED_ERROR_ON GPIOA->ODR |= LED_ERROR; \
					io_exp_set_output(IO_EXP_ERROR_LED, 1)



#define LED_FAULT_ON GPIOA->ODR |= LED_FAULT

/*
 *
 * Define your Fee for Selling one kWh Energy in cents e.g. 25
 *
 */
#define SELL_COST 28.74
#define CONSUME_FEE_U30 12.36
#define CONSUME_FEE_O30	16.74
#define PRICE 21.18
#define BASIC_CHARGE 782

/*
 * Define the Baudrate of your Smart Meter
 * Normally this is 9600 Baud
 */
#define SMART_METER_BAUDRATE 9600

/*
 *
 * Define for Speed of USART2
 *
 */
#define USART2_SPEED 115200

/*
 *
 * Define the maximum Power of your Solar Plant
 *
 */
#define SOLARPOWER 10000

#endif /* DEFINES_H_ */

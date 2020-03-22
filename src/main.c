#include <stdint-gcc.h>
#include "stm32f091xc.h"
#include "stm32f0xx_hal.h"
#include "stm32f0xx_it.h"

#include "eeprom.h"
#include "string.h"
#include "functions.h"
#include <time.h>

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "timers.h"
#include "semphr.h"
#include "main.h"
#include "gpio.h"
#include "spi.h"
#include "flash.h"
#include "defines.h"
#include "comp.h"
#include "dac.h"
#include "rtc.h"
#include "nrf24.h"
#include "nrf24_hal.h"

/* Priorities at which the tasks are created.  The event semaphore task is
 given the maximum priority of ( configMAX_PRIORITIES - 1 ) to ensure it runs as
 soon as the semaphore is given. */
#define mainQUEUE_RECEIVE_TASK_PRIORITY		( tskIDLE_PRIORITY + 2 )
#define	mainQUEUE_SEND_TASK_PRIORITY		( tskIDLE_PRIORITY + 1 )
#define mainEVENT_SEMAPHORE_TASK_PRIORITY	( configMAX_PRIORITIES - 1 )

/* The rate at which data is sent to the queue, specified in milliseconds, and
 converted to ticks using the portTICK_RATE_MS constant. */
#define mainQUEUE_SEND_PERIOD_MS			( 200 / portTICK_RATE_MS )

/* The period of the example software timer, specified in milliseconds, and
 converted to ticks using the portTICK_RATE_MS constant. */
#define mainSOFTWARE_TIMER_PERIOD_MS		( 1000 / portTICK_RATE_MS )

/* The number of items the queue can hold.  This is 1 as the receive task
 will remove items as they are added, meaning the send task should always find
 the queue empty. */
#define mainQUEUE_LENGTH					( 1 )

void SystemClock_Config(void);
static void prvSetupHardware(void);
void vApplicationMallocFailedHook(void);
void vApplicationStackOverflowHook( xTaskHandle *pxTask,
		signed char *pcTaskName);
void vApplicationTickHook();

TaskHandle_t xcheck_cmd_struct;

//uint8_t ucHeap[ configTOTAL_HEAP_SIZE ]={0};

uint32_t meter_purchase, meter_delivery, meter_uptime;
int32_t meter_power;
uint32_t error_counter = 0;

uint8_t sml_tx_data[400] = { 0 };

uuid_t uuid = { { ((uint32_t*) UUID_BASE_ADDRESS),
		(((uint32_t*) UUID_BASE_ADDRESS + 1)), ((uint32_t*) (UUID_BASE_ADDRESS
				+ 2)) } };

uint32_t rtc_current_time_unix;
uint32_t rtc_old_time_unix;

const uint8_t nrf24_tx_size = NRF24_TX_SIZE;
data_union_t nrf24_tx_buf[(NRF24_TX_SIZE / 4)];

uint32_t i,j,k;

// Buffer to store a payload of maximum width
uint8_t nRF24_payload[32];

// Pipe number
nRF24_RXResult pipe;

// Length of received payload
uint8_t payload_length;

nRF24_TXResult tx_res;


int main(void) {

	prvSetupHardware();
	flags.new_main_sml_packet = 0;

	flash_init();
//	flash_bulkErase();
	flags.gateway = 1;

	RTC_GetTime(RTC_Format_BIN, &sm_time);
	RTC_GetDate(RTC_Format_BIN, &sm_date);

	flash_address_get_main();
	flash_address_get_plant();

	rtc_current_time_unix = rtc_old_time_unix = rtc_get_unix_time(&sm_time,
			&sm_date);

	nrf24_init_tx();

	// Some variables
	uint32_t packets_lost = 0; // global counter of lost packets
	uint8_t otx;
	uint8_t otx_plos_cnt; // lost packet count
	uint8_t otx_arc_cnt; // retransmit count

	// The main loop
	payload_length = 10;
	j = 0;
	while (1) {

			RTC_GetTime(RTC_FORMAT_BIN, &sm_time);
			RTC_GetDate(RTC_FORMAT_BIN, &sm_date);
			rtc_current_time_unix = rtc_get_unix_time(&sm_time, &sm_date);

			if (flags.new_plant_sml_packet) {
				flags.new_plant_sml_packet = 0;
				sm_plant_extract_data();
			}

			if (flags.new_main_sml_packet) {
				flags.new_main_sml_packet = 0;
				sm_main_extract_data();
			}

			if ((rtc_current_time_unix - rtc_old_time_unix) >= FLASH_SAVE_INTERVALL) {
				rtc_old_time_unix = rtc_current_time_unix;
				/*
				 * save data every 2 seconds
				 */

				nrf24_tx_buf[0].uint32_data = sm_main_current_data.power;
				nrf24_tx_buf[1].uint32_data = sm_plant_current_data.power;
				tx_res = nRF24_TransmitPacket(nrf24_tx_buf, nrf24_tx_size);
				otx = nRF24_GetRetransmitCounters();
				if (tx_res == nRF24_TX_SUCCESS) {
					/*
					 * OK
					 */
					NOP
				} else {
					/*
					 * not ok
					 */
					NOP
				}
				switch (tx_res) {
				case nRF24_TX_SUCCESS:
					LED_STATUS_TOGGLE;
					UART_SendStr("OK");
					break;
				case nRF24_TX_TIMEOUT:
					LED_ERROR_TOGGLE;
					UART_SendStr("TIMEOUT");
					break;
				case nRF24_TX_MAXRT:
					LED_ERROR_TOGGLE;
					UART_SendStr("MAX RETRANSMIT");
					nRF24_ResetPLOS();
					break;
				default:
					LED_ERROR_TOGGLE;
					UART_SendStr("ERROR");
					break;
				}


				flash_main_store_data_in_cache(rtc_current_time_unix);
				flash_plant_store_data_in_cache(rtc_current_time_unix);

			}

			if (flags.usart6_new_cmd) {
				check_cmd_frame();
			}

		}
}

void vApplicationMallocFailedHook(void) {
	while (1)
		;
}

void vApplicationStackOverflowHook( xTaskHandle *pxTask,
		signed char *pcTaskName) {
	while (1)
		;
}

void vApplicationTickHook() {
	while (1)
		;
}

static void prvSetupHardware(void) {

	SystemClock_Config();

	gpio_init();

	/*
	 * communication for DAC and EEPROM
	 */
	i2c1_init();

	/*
	 * communication for NRF24 Wireless Chip and FLASH IC
	 */
	spi1_init();

	usart1_init();
	usart6_init();
	usart3_init();
//	usart5_init();

	dac_init();
	comp_init();

	crc_init();

	rtc_init();

	sml_tx_data[0] = 0x1B;
	sml_tx_data[1] = 0x1B;
	sml_tx_data[2] = 0x1B;
	sml_tx_data[3] = 0x1B;
	sml_tx_data[4] = 0x01;
	sml_tx_data[5] = 0x01;
	sml_tx_data[6] = 0x01;
	sml_tx_data[7] = 0x01;
	sml_tx_data[8] = 0x76;
	sml_tx_data[9] = 0x07;
	sml_tx_data[10] = 0x00;
	sml_tx_data[11] = 0x0E;
	sml_tx_data[12] = 0x03;
	sml_tx_data[13] = 0xAD;
	sml_tx_data[14] = 0x1E;
	sml_tx_data[15] = 0x67;
	sml_tx_data[16] = 0x62;
	sml_tx_data[17] = 0x00;
	sml_tx_data[18] = 0x62;
	sml_tx_data[19] = 0x00;
	sml_tx_data[20] = 0x72;
	sml_tx_data[21] = 0x63;
	sml_tx_data[22] = 0x01;
	sml_tx_data[23] = 0x01;
	sml_tx_data[24] = 0x76;
	sml_tx_data[25] = 0x01;
	sml_tx_data[26] = 0x01;
	sml_tx_data[27] = 0x07;
	sml_tx_data[28] = 0x00;
	sml_tx_data[29] = 0x0E;
	sml_tx_data[30] = 0x07;
	sml_tx_data[31] = 0xA7;
	sml_tx_data[32] = 0xB4;
	sml_tx_data[33] = 0xCD;
	sml_tx_data[34] = 0x0B;
	sml_tx_data[35] = 0x09;
	sml_tx_data[36] = 0x01;
	sml_tx_data[37] = 0x45;
	sml_tx_data[38] = 0x4D;
	sml_tx_data[39] = 0x48;
	sml_tx_data[40] = 0x00;
	sml_tx_data[41] = 0x00;
	sml_tx_data[42] = 0x41;
	sml_tx_data[43] = 0xA7;
	sml_tx_data[44] = 0x9F;
	sml_tx_data[45] = 0x01;
	sml_tx_data[46] = 0x01;
	sml_tx_data[47] = 0x63;
	sml_tx_data[48] = 0x77;
	sml_tx_data[49] = 0x00;
	sml_tx_data[50] = 0x76;
	sml_tx_data[51] = 0x07;
	sml_tx_data[52] = 0x00;
	sml_tx_data[53] = 0x0E;
	sml_tx_data[54] = 0x03;
	sml_tx_data[55] = 0xAD;
	sml_tx_data[56] = 0x1E;
	sml_tx_data[57] = 0x68;
	sml_tx_data[58] = 0x62;
	sml_tx_data[59] = 0x00;
	sml_tx_data[60] = 0x62;
	sml_tx_data[61] = 0x00;
	sml_tx_data[62] = 0x72;
	sml_tx_data[63] = 0x63;
	sml_tx_data[64] = 0x07;
	sml_tx_data[65] = 0x01;
	sml_tx_data[66] = 0x77;
	sml_tx_data[67] = 0x01;
	sml_tx_data[68] = 0x0B;
	sml_tx_data[69] = 0x09;
	sml_tx_data[70] = 0x01;
	sml_tx_data[71] = 0x45;
	sml_tx_data[72] = 0x4D;
	sml_tx_data[73] = 0x48;
	sml_tx_data[74] = 0x00;
	sml_tx_data[75] = 0x00;
	sml_tx_data[76] = 0x41;
	sml_tx_data[77] = 0xA7;
	sml_tx_data[78] = 0x9F;
	sml_tx_data[79] = 0x07;
	sml_tx_data[80] = 0x01;
	sml_tx_data[81] = 0x00;
	sml_tx_data[82] = 0x62;
	sml_tx_data[83] = 0x0A;
	sml_tx_data[84] = 0xFF;
	sml_tx_data[85] = 0xFF;
	sml_tx_data[86] = 0x72;
	sml_tx_data[87] = 0x62;
	sml_tx_data[88] = 0x01;
	sml_tx_data[89] = 0x65;
	sml_tx_data[90] = 0x07;
	sml_tx_data[91] = 0xA7;
	sml_tx_data[92] = 0x1A;
	sml_tx_data[93] = 0x79;
	sml_tx_data[94] = 0x7A;
	sml_tx_data[95] = 0x77;
	sml_tx_data[96] = 0x07;
	sml_tx_data[97] = 0x81;
	sml_tx_data[98] = 0x81;
	sml_tx_data[99] = 0xC7;
	sml_tx_data[100] = 0x82;
	sml_tx_data[101] = 0x03;
	sml_tx_data[102] = 0xFF;
	sml_tx_data[103] = 0x01;
	sml_tx_data[104] = 0x01;
	sml_tx_data[105] = 0x01;
	sml_tx_data[106] = 0x01;
	sml_tx_data[107] = 0x04;
	sml_tx_data[108] = 0x45;
	sml_tx_data[109] = 0x4D;
	sml_tx_data[110] = 0x48;
	sml_tx_data[111] = 0x01;
	sml_tx_data[112] = 0x77;
	sml_tx_data[113] = 0x07;
	sml_tx_data[114] = 0x01;
	sml_tx_data[115] = 0x00;
	sml_tx_data[116] = 0x00;
	sml_tx_data[117] = 0x00;
	sml_tx_data[118] = 0x09;
	sml_tx_data[119] = 0xFF;
	sml_tx_data[120] = 0x01;
	sml_tx_data[121] = 0x01;
	sml_tx_data[122] = 0x01;
	sml_tx_data[123] = 0x01;
	sml_tx_data[124] = 0x0B;
	sml_tx_data[125] = 0x09;
	sml_tx_data[126] = 0x01;
	sml_tx_data[127] = 0x45;
	sml_tx_data[128] = 0x4D;
	sml_tx_data[129] = 0x48;
	sml_tx_data[130] = 0x00;
	sml_tx_data[131] = 0x00;
	sml_tx_data[132] = 0x41;
	sml_tx_data[133] = 0xA7;
	sml_tx_data[134] = 0x9F;
	sml_tx_data[135] = 0x01;
	sml_tx_data[136] = 0x77;
	sml_tx_data[137] = 0x07;
	sml_tx_data[138] = 0x01;
	sml_tx_data[139] = 0x00;
	sml_tx_data[140] = 0x01;
	sml_tx_data[141] = 0x08;
	sml_tx_data[142] = 0x00;
	sml_tx_data[143] = 0xFF;
	sml_tx_data[144] = 0x64;
	sml_tx_data[145] = 0x01;
	sml_tx_data[146] = 0x01;
	sml_tx_data[147] = 0xC2;
	sml_tx_data[148] = 0x01;
	sml_tx_data[149] = 0x62;
	sml_tx_data[150] = 0x1E;
	sml_tx_data[151] = 0x52;
	sml_tx_data[152] = 0xFF;
	sml_tx_data[153] = 0x56;
	sml_tx_data[154] = 0x00;
	sml_tx_data[155] = 0x04;
	sml_tx_data[156] = 0x97;
	sml_tx_data[157] = 0xD5;
	sml_tx_data[158] = 0x1E;
	sml_tx_data[159] = 0x01;
	sml_tx_data[160] = 0x77;
	sml_tx_data[161] = 0x07;
	sml_tx_data[162] = 0x01;
	sml_tx_data[163] = 0x00;
	sml_tx_data[164] = 0x02;
	sml_tx_data[165] = 0x08;
	sml_tx_data[166] = 0x00;
	sml_tx_data[167] = 0xFF;
	sml_tx_data[168] = 0x64;
	sml_tx_data[169] = 0x01;
	sml_tx_data[170] = 0x01;
	sml_tx_data[171] = 0xC2;
	sml_tx_data[172] = 0x01;
	sml_tx_data[173] = 0x62;
	sml_tx_data[174] = 0x1E;
	sml_tx_data[175] = 0x52;
	sml_tx_data[176] = 0xFF;
	sml_tx_data[177] = 0x56;
	sml_tx_data[178] = 0x00;
	sml_tx_data[179] = 0x12;
	sml_tx_data[180] = 0x7E;
	sml_tx_data[181] = 0x43;
	sml_tx_data[182] = 0xB6;
	sml_tx_data[183] = 0x01;
	sml_tx_data[184] = 0x77;
	sml_tx_data[185] = 0x07;
	sml_tx_data[186] = 0x01;
	sml_tx_data[187] = 0x00;
	sml_tx_data[188] = 0x01;
	sml_tx_data[189] = 0x08;
	sml_tx_data[190] = 0x01;
	sml_tx_data[191] = 0xFF;
	sml_tx_data[192] = 0x01;
	sml_tx_data[193] = 0x01;
	sml_tx_data[194] = 0x62;
	sml_tx_data[195] = 0x1E;
	sml_tx_data[196] = 0x52;
	sml_tx_data[197] = 0xFF;
	sml_tx_data[198] = 0x56;
	sml_tx_data[199] = 0x00;
	sml_tx_data[200] = 0x04;
	sml_tx_data[201] = 0x97;
	sml_tx_data[202] = 0xD5;
	sml_tx_data[203] = 0x1E;
	sml_tx_data[204] = 0x01;
	sml_tx_data[205] = 0x77;
	sml_tx_data[206] = 0x07;
	sml_tx_data[207] = 0x01;
	sml_tx_data[208] = 0x00;
	sml_tx_data[209] = 0x02;
	sml_tx_data[210] = 0x08;
	sml_tx_data[211] = 0x01;
	sml_tx_data[212] = 0xFF;
	sml_tx_data[213] = 0x01;
	sml_tx_data[214] = 0x01;
	sml_tx_data[215] = 0x62;
	sml_tx_data[216] = 0x1E;
	sml_tx_data[217] = 0x52;
	sml_tx_data[218] = 0xFF;
	sml_tx_data[219] = 0x56;
	sml_tx_data[220] = 0x00;
	sml_tx_data[221] = 0x12;
	sml_tx_data[222] = 0x7E;
	sml_tx_data[223] = 0x43;
	sml_tx_data[224] = 0xB6;
	sml_tx_data[225] = 0x01;
	sml_tx_data[226] = 0x77;
	sml_tx_data[227] = 0x07;
	sml_tx_data[228] = 0x01;
	sml_tx_data[229] = 0x00;
	sml_tx_data[230] = 0x01;
	sml_tx_data[231] = 0x08;
	sml_tx_data[232] = 0x02;
	sml_tx_data[233] = 0xFF;
	sml_tx_data[234] = 0x01;
	sml_tx_data[235] = 0x01;
	sml_tx_data[236] = 0x62;
	sml_tx_data[237] = 0x1E;
	sml_tx_data[238] = 0x52;
	sml_tx_data[239] = 0xFF;
	sml_tx_data[240] = 0x56;
	sml_tx_data[241] = 0x00;
	sml_tx_data[242] = 0x00;
	sml_tx_data[243] = 0x00;
	sml_tx_data[244] = 0x00;
	sml_tx_data[245] = 0x00;
	sml_tx_data[246] = 0x01;
	sml_tx_data[247] = 0x77;
	sml_tx_data[248] = 0x07;
	sml_tx_data[249] = 0x01;
	sml_tx_data[250] = 0x00;
	sml_tx_data[251] = 0x02;
	sml_tx_data[252] = 0x08;
	sml_tx_data[253] = 0x02;
	sml_tx_data[254] = 0xFF;
	sml_tx_data[255] = 0x01;
	sml_tx_data[256] = 0x01;
	sml_tx_data[257] = 0x62;
	sml_tx_data[258] = 0x1E;
	sml_tx_data[259] = 0x52;
	sml_tx_data[260] = 0xFF;
	sml_tx_data[261] = 0x56;
	sml_tx_data[262] = 0x00;
	sml_tx_data[263] = 0x00;
	sml_tx_data[264] = 0x00;
	sml_tx_data[265] = 0x00;
	sml_tx_data[266] = 0x00;
	sml_tx_data[267] = 0x01;
	sml_tx_data[268] = 0x77;
	sml_tx_data[269] = 0x07;
	sml_tx_data[270] = 0x01;
	sml_tx_data[271] = 0x00;
	sml_tx_data[272] = 0x10;
	sml_tx_data[273] = 0x07;
	sml_tx_data[274] = 0x00;
	sml_tx_data[275] = 0xFF;
	sml_tx_data[276] = 0x01;
	sml_tx_data[277] = 0x01;
	sml_tx_data[278] = 0x62;
	sml_tx_data[279] = 0x1B;
	sml_tx_data[280] = 0x52;
	sml_tx_data[281] = 0xFF;
	sml_tx_data[282] = 0x55;
	sml_tx_data[283] = 0x00;
	sml_tx_data[284] = 0x00;
	sml_tx_data[285] = 0x1E;
	sml_tx_data[286] = 0x5F;
	sml_tx_data[287] = 0x01;
	sml_tx_data[288] = 0x77;
	sml_tx_data[289] = 0x07;
	sml_tx_data[290] = 0x81;
	sml_tx_data[291] = 0x81;
	sml_tx_data[292] = 0xC7;
	sml_tx_data[293] = 0x82;
	sml_tx_data[294] = 0x05;
	sml_tx_data[295] = 0xFF;
	sml_tx_data[296] = 0x01;
	sml_tx_data[297] = 0x72;
	sml_tx_data[298] = 0x62;
	sml_tx_data[299] = 0x01;
	sml_tx_data[300] = 0x65;
	sml_tx_data[301] = 0x07;
	sml_tx_data[302] = 0xA7;
	sml_tx_data[303] = 0x1A;
	sml_tx_data[304] = 0x79;
	sml_tx_data[305] = 0x01;
	sml_tx_data[306] = 0x01;
	sml_tx_data[307] = 0x83;
	sml_tx_data[308] = 0x02;
	sml_tx_data[309] = 0xEA;
	sml_tx_data[310] = 0x21;
	sml_tx_data[311] = 0x30;
	sml_tx_data[312] = 0xD5;
	sml_tx_data[313] = 0x39;
	sml_tx_data[314] = 0x99;
	sml_tx_data[315] = 0x0D;
	sml_tx_data[316] = 0xA8;
	sml_tx_data[317] = 0x6A;
	sml_tx_data[318] = 0x5B;
	sml_tx_data[319] = 0xD1;
	sml_tx_data[320] = 0x82;
	sml_tx_data[321] = 0xA2;
	sml_tx_data[322] = 0x3D;
	sml_tx_data[323] = 0x70;
	sml_tx_data[324] = 0xEC;
	sml_tx_data[325] = 0x6A;
	sml_tx_data[326] = 0xE7;
	sml_tx_data[327] = 0x4B;
	sml_tx_data[328] = 0x58;
	sml_tx_data[329] = 0x18;
	sml_tx_data[330] = 0x53;
	sml_tx_data[331] = 0x0D;
	sml_tx_data[332] = 0x6D;
	sml_tx_data[333] = 0xE2;
	sml_tx_data[334] = 0x02;
	sml_tx_data[335] = 0xDC;
	sml_tx_data[336] = 0x8C;
	sml_tx_data[337] = 0x16;
	sml_tx_data[338] = 0x4E;
	sml_tx_data[339] = 0x18;
	sml_tx_data[340] = 0x3C;
	sml_tx_data[341] = 0x4D;
	sml_tx_data[342] = 0xB7;
	sml_tx_data[343] = 0x79;
	sml_tx_data[344] = 0xF8;
	sml_tx_data[345] = 0xEA;
	sml_tx_data[346] = 0x6B;
	sml_tx_data[347] = 0xF5;
	sml_tx_data[348] = 0x8E;
	sml_tx_data[349] = 0x49;
	sml_tx_data[350] = 0xFF;
	sml_tx_data[351] = 0x05;
	sml_tx_data[352] = 0x21;
	sml_tx_data[353] = 0xC1;
	sml_tx_data[354] = 0xC8;
	sml_tx_data[355] = 0x67;
	sml_tx_data[356] = 0x00;
	sml_tx_data[357] = 0x01;
	sml_tx_data[358] = 0x01;
	sml_tx_data[359] = 0x01;
	sml_tx_data[360] = 0x63;
	sml_tx_data[361] = 0xEC;
	sml_tx_data[362] = 0xA9;
	sml_tx_data[363] = 0x00;
	sml_tx_data[364] = 0x76;
	sml_tx_data[365] = 0x07;
	sml_tx_data[366] = 0x00;
	sml_tx_data[367] = 0x0E;
	sml_tx_data[368] = 0x03;
	sml_tx_data[369] = 0xAD;
	sml_tx_data[370] = 0x1E;
	sml_tx_data[371] = 0x6B;
	sml_tx_data[372] = 0x62;
	sml_tx_data[373] = 0x00;
	sml_tx_data[374] = 0x62;
	sml_tx_data[375] = 0x00;
	sml_tx_data[376] = 0x72;
	sml_tx_data[377] = 0x63;
	sml_tx_data[378] = 0x02;
	sml_tx_data[379] = 0x01;
	sml_tx_data[380] = 0x71;
	sml_tx_data[381] = 0x01;
	sml_tx_data[382] = 0x63;
	sml_tx_data[383] = 0xCB;
	sml_tx_data[384] = 0x4C;
	sml_tx_data[385] = 0x00;
	sml_tx_data[386] = 0x00;
	sml_tx_data[387] = 0x1B;
	sml_tx_data[388] = 0x1B;
	sml_tx_data[389] = 0x1B;
	sml_tx_data[390] = 0x1B;
	sml_tx_data[391] = 0x1A;
	sml_tx_data[392] = 0x01;
	sml_tx_data[393] = 0xF6;
	sml_tx_data[394] = 0xA5;
	sml_tx_data[395] = 0x00;
	sml_tx_data[396] = 0x00;
	sml_tx_data[397] = 0x00;
	sml_tx_data[398] = 0x00;

}

void SystemClock_Config(void) {

	RCC_OscInitTypeDef RCC_OscInitStruct = { 0 };
	RCC_ClkInitTypeDef RCC_ClkInitStruct = { 0 };
	RCC_PeriphCLKInitTypeDef PeriphClkInit = { 0 };

	/** Initializes the CPU, AHB and APB busses clocks
	 */
	RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI
			| RCC_OSCILLATORTYPE_HSI14 | RCC_OSCILLATORTYPE_LSE;
	RCC_OscInitStruct.HSIState = RCC_HSI_ON;
	RCC_OscInitStruct.LSEState = RCC_LSE_ON;
	RCC_OscInitStruct.HSI14State = RCC_HSI14_ON;
	RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
	RCC_OscInitStruct.HSI14CalibrationValue = 16;
	RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
	RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
	RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL6;
	RCC_OscInitStruct.PLL.PREDIV = RCC_PREDIV_DIV1;
	if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
		Error_Handler();
	}
	/** Initializes the CPU, AHB and APB busses clocks
	 */
	RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
			| RCC_CLOCKTYPE_PCLK1;
	RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
	RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
	RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;

	if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK) {
		Error_Handler();
	}
	PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_USART1
			| RCC_PERIPHCLK_I2C1 | RCC_PERIPHCLK_RTC;
	PeriphClkInit.Usart1ClockSelection = RCC_USART1CLKSOURCE_PCLK1;
	PeriphClkInit.I2c1ClockSelection = RCC_I2C1CLKSOURCE_HSI;
	PeriphClkInit.RTCClockSelection = RCC_RTCCLKSOURCE_LSE;
	if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK) {
		Error_Handler();
	}
}

void Error_Handler(void) {
	while (1)
		;
}

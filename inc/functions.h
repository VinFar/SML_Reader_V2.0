#ifndef FUNCTIONS_H_
#define FUNCTIONS_H_

#include "main.h"
#include "nrf24.h"
#include "shared_defines.h"

#define ARRAY_LENGTH(A) (sizeof(A)/sizeof((A)[0]))

uint16_t Log2n(uint16_t n);
int16_t isPowerOfTwo(uint16_t n);
int16_t findPosition(uint16_t n);
void check_cmd_frame();
void sm_main_extract_data();
void sm_plant_extract_data();
void flash_main_store_data_in_cache(uint32_t timestamp);
void flash_plant_store_data_in_cache(uint32_t timestamp);
void check_cmd_struct(void *param);

struct nrf24_queue_struct {
  uint16_t read_idx;
  uint16_t write_idx;
  nrf24_frame_queue_t items[4];
};

enum enqueue_result {
  ENQUEUE_RESULT_SUCCESS,
  ENQUEUE_RESULT_FULL,
};

enum dequeue_result {
  DEQUEUE_RESULT_SUCCESS,
  DEQUEUE_RESULT_EMPTY,
};



void nrf_queue_init();
enum enqueue_result nrf_queue_enqueue(nrf24_frame_queue_t * p_new_item);
enum dequeue_result nrf_queue_dequeue(nrf24_frame_queue_t * p_item_out);
uint8_t nrf_queue_is_empty();

#endif /* FUNCTIONS_H_ */

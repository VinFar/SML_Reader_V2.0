#ifndef I2C_H_
#define I2C_H_

#include "main.h"

#define I2C_TIMEOUT 25000

#define I2C1_RESET	I2C1->CR1&=~I2C_CR1_PE;\
		while((I2C1->CR1 & I2C_CR1_PE));\
		I2C1->CR1|=I2C_CR1_PE;\

#define I2C_READ    1
#define I2C_WRITE   0

extern void i2c1_init();
extern int8_t i2c1_stop(void);

extern int8_t i2c1_start(uint8_t address, uint8_t NOB, unsigned RW);
extern unsigned char i2c1_rep_start(unsigned char addr, unsigned RW);
extern void i2c1_start_wait(unsigned char addr);
extern int8_t i2c1_write(char data);
extern int8_t i2c1_readAck(uint8_t *ptr) ;
extern int8_t i2c1_readNack(uint8_t *ptr);

#endif /* I2C_H_ */

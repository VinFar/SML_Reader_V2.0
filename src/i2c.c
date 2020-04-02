#include "i2c.h"

I2C_HandleTypeDef hi2c1;

/* I2C1 init function */
void i2c1_init(void) {

	hi2c1.Instance = I2C1;
	  hi2c1.Init.Timing = 0x20000209; //1Mhz
//	hi2c1.Init.Timing = 0x2010091A; //400kHz
//	hi2c1.Init.Timing = 0x20303E5D; //100kHz
	hi2c1.Init.OwnAddress1 = 0;
	hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
	hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
	hi2c1.Init.OwnAddress2 = 0;
	hi2c1.Init.OwnAddress2Masks = I2C_OA2_NOMASK;
	hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
	hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
	if (HAL_I2C_Init(&hi2c1) != HAL_OK) {
		Error_Handler();
	}
	/** Configure Analogue filter
	 */
	if (HAL_I2CEx_ConfigAnalogFilter(&hi2c1, I2C_ANALOGFILTER_ENABLE)
			!= HAL_OK) {
		Error_Handler();
	}
	/** Configure Digital filter
	 */
	if (HAL_I2CEx_ConfigDigitalFilter(&hi2c1, 0) != HAL_OK) {
		Error_Handler();
	}

}

void HAL_I2C_MspInit(I2C_HandleTypeDef *i2cHandle) {

	GPIO_InitTypeDef GPIO_InitStruct = { 0 };
	if (i2cHandle->Instance == I2C1) {
		/* USER CODE BEGIN I2C1_MspInit 0 */

		/* USER CODE END I2C1_MspInit 0 */

		__HAL_RCC_GPIOB_CLK_ENABLE();
		/**I2C1 GPIO Configuration
		 PB6     ------> I2C1_SCL
		 PB7     ------> I2C1_SDA
		 */
		GPIO_InitStruct.Pin = I2C1_SCL_Pin | I2C1_SDA_Pin;
		GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;
		GPIO_InitStruct.Pull = GPIO_PULLUP;
		GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
		GPIO_InitStruct.Alternate = GPIO_AF1_I2C1;
		HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

		/* I2C1 clock enable */
		__HAL_RCC_I2C1_CLK_ENABLE();
		/* USER CODE BEGIN I2C1_MspInit 1 */

		/* USER CODE END I2C1_MspInit 1 */
	}
}

void HAL_I2C_MspDeInit(I2C_HandleTypeDef *i2cHandle) {

	if (i2cHandle->Instance == I2C1) {
		/* USER CODE BEGIN I2C1_MspDeInit 0 */

		/* USER CODE END I2C1_MspDeInit 0 */
		/* Peripheral clock disable */
		__HAL_RCC_I2C1_CLK_DISABLE();

		/**I2C1 GPIO Configuration
		 PB6     ------> I2C1_SCL
		 PB7     ------> I2C1_SDA
		 */
		HAL_GPIO_DeInit(GPIOB, I2C1_SCL_Pin | I2C1_SDA_Pin);

		/* USER CODE BEGIN I2C1_MspDeInit 1 */

		/* USER CODE END I2C1_MspDeInit 1 */
	}
}


/*************************************************************************
 Issues a start condition and sends address and transfer direction.
 return 0 = device accessible, 1= failed to access device
 *************************************************************************/
int8_t i2c1_start(uint8_t address, uint8_t NOB, unsigned RW) {

	uint32_t timeout;

	timeout = I2C_TIMEOUT;
	while ((I2C1->ISR & I2C_ISR_BUSY) == I2C_ISR_BUSY) {
		if ((--timeout) != 0) {	//branch not equal

		} else {
			I2C1_RESET
			;
			return -1;		//ERROR
		}
	}
	I2C1->CR2 = (I2C_CR2_SADD & ((uint32_t) address << 1))
			| ((uint32_t) RW << 10);		//Set Device Address
	I2C1->CR2 &= ~I2C_CR2_NBYTES;
	I2C1->CR2 |= (I2C_CR2_NBYTES & (NOB << 16));

	I2C1->CR2 |= I2C_CR2_START;	//Send Start-Condition and Slave Address

	timeout = I2C_TIMEOUT;
	while ((I2C1->ISR & I2C_ISR_BUSY) != I2C_ISR_BUSY) {//Wait till Bus is busy
		if ((--timeout) != 0) {

		} else {
			I2C1_RESET
			;
			return -1;		//ERROR
		}
	}
	timeout = I2C_TIMEOUT;
	while ((I2C1->CR2 & I2C_CR2_START) == I2C_CR2_START) {
		if ((--timeout) != 0) {

		} else {
			I2C1_RESET
			;
			return -1;		//ERROR
		}
	}	//Wait until start and address are sent

	if ((I2C1->ISR & I2C_ISR_NACKF) == I2C_ISR_NACKF) {

		I2C1->ICR &= ~I2C_ICR_NACKCF;	//Clear NACK Flag
		I2C1_RESET
		;
		return -1;	//If NACK received return 1 (Error)
	}

	return 0;
}

/*************************************************************************
 Terminates the data transfer and releases the I2C bus
 *************************************************************************/
int8_t i2c1_stop(void) {

	uint32_t timeout;
	timeout = I2C_TIMEOUT;

	while (!(I2C1->ISR && I2C_ISR_TXE)) {
		if ((--timeout) != 0) {
		} else {
			I2C1_RESET
			;
			return -1;		//ERROR
		}
	}
	timeout = I2C_TIMEOUT;
	I2C1->CR2 |= I2C_CR2_STOP;			//Send Stop Bit to terminate Transmision
	while ((I2C1->CR2 & I2C_CR2_STOP) == I2C_CR2_STOP) {
		if ((--timeout) != 0) {
		} else {
			I2C1_RESET
			;
			return -1;		//ERROR
		}
	}

	return 0;
}

/*************************************************************************
 Send one byte to I2C device

 Input:    byte to be transfered
 Return:   0 write successful
 1 write failed
 *************************************************************************/
int8_t i2c1_write(char data) {

	uint32_t timeout;
	timeout = I2C_TIMEOUT;

	while (!(I2C1->ISR && I2C_ISR_TXE)) {
		if ((timeout--) == 0) {

			return -1;
		}
	}

	I2C1->TXDR = data;

	timeout = I2C_TIMEOUT;

	return 0;

}

/*************************************************************************
 Read one byte from the I2C device, request more data from device

 Return:  byte read from I2C device
 *************************************************************************/
int8_t i2c1_readAck(uint8_t *ptr) {

	uint32_t timeout = I2C_TIMEOUT;
	while ((I2C1->ISR & I2C_ISR_RXNE) != I2C_ISR_RXNE) {
		if ((--timeout) != 0) {

		} else {
			I2C1_RESET
			;
			*ptr = 0;
			return -1;		//ERROR
		}
	}

	*ptr = I2C1->RXDR;
	return 0;

}

/*************************************************************************
 Read one byte from the I2C device, read is followed by a stop condition

 Return:  byte read from I2C device
 *************************************************************************/
int8_t i2c1_readNack(uint8_t *ptr) {
	I2C1->CR2 |= I2C_CR2_NACK;
	uint32_t timeout = I2C_TIMEOUT;
	while ((I2C1->ISR & I2C_ISR_RXNE) != I2C_ISR_RXNE) {
		if ((--timeout) != 0) {
		} else {
			I2C1_RESET
			;
			return -1;		//ERROR
		}
	}

	I2C1->CR2 &= ~I2C_CR2_NACK;
	*ptr = I2C1->RXDR;
	return 0;

}


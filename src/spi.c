#include "spi.h"

SPI_HandleTypeDef hspi1;

/* SPI1 init function */
void spi1_init(void) {

	hspi1.Instance = SPI1;
	hspi1.Init.Mode = SPI_MODE_MASTER;
	hspi1.Init.Direction = SPI_DIRECTION_2LINES;
	hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
	hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
	hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
	hspi1.Init.NSS = SPI_NSS_SOFT;
	hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_8;
	hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
	hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
	hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
	hspi1.Init.CRCPolynomial = 7;
	hspi1.Init.CRCLength = SPI_CRC_LENGTH_DATASIZE;
	hspi1.Init.NSSPMode = SPI_NSS_PULSE_DISABLE;
	if (HAL_SPI_Init(&hspi1) != HAL_OK) {
		Error_Handler();
	}
	SPI1->CR1 |= SPI_CR1_SPE;

}

void HAL_SPI_MspInit(SPI_HandleTypeDef* spiHandle) {

	GPIO_InitTypeDef GPIO_InitStruct = { 0 };
	if (spiHandle->Instance == SPI1) {
		/* USER CODE BEGIN SPI1_MspInit 0 */

		/* USER CODE END SPI1_MspInit 0 */
		/* SPI1 clock enable */
		__HAL_RCC_SPI1_CLK_ENABLE()
		;

		__HAL_RCC_GPIOB_CLK_ENABLE()
		;
		/**SPI1 GPIO Configuration
		 PB3     ------> SPI1_SCK
		 PB4     ------> SPI1_MISO
		 PB5     ------> SPI1_MOSI
		 */
		GPIO_InitStruct.Pin = SPI1_SCK_Pin | SPI1_MOSI_Pin | SPI1_MISO_Pin;
		GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
		GPIO_InitStruct.Pull = GPIO_NOPULL;
		GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
		GPIO_InitStruct.Alternate = GPIO_AF0_SPI1;
		HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

		/* USER CODE BEGIN SPI1_MspInit 1 */

		/* USER CODE END SPI1_MspInit 1 */
	}
}

void HAL_SPI_MspDeInit(SPI_HandleTypeDef* spiHandle) {

	if (spiHandle->Instance == SPI1) {
		/* USER CODE BEGIN SPI1_MspDeInit 0 */

		/* USER CODE END SPI1_MspDeInit 0 */
		/* Peripheral clock disable */
		__HAL_RCC_SPI1_CLK_DISABLE();

		/**SPI1 GPIO Configuration
		 PB3     ------> SPI1_SCK
		 PB4     ------> SPI1_MISO
		 PB5     ------> SPI1_MOSI
		 */
		HAL_GPIO_DeInit(GPIOB, SPI1_SCK_Pin | SPI1_MISO_Pin | SPI1_MOSI_Pin);

		/* USER CODE BEGIN SPI1_MspDeInit 1 */

		/* USER CODE END SPI1_MspDeInit 1 */
	}
}

void spi_transmit(uint8_t *outpp, uint32_t count) {

	uint8_t *outp = outpp;

	while (count) {
		while (!(SPI1->SR & SPI_SR_TXE))
			;
		if (count > 1U) {
			SPI1->DR = *((uint16_t *) outp);
			outp += sizeof(uint16_t);
			count -= 2U;
		} else {
			*((__IO uint8_t *) &SPI1->DR) = (*outp++);
			count--;
		}


	}

	/* Check the end of the transaction */
	if (SPI_EndRxTxTransaction(&hspi1, 50, 0) != HAL_OK) {
		NOP
	}

	/* Clear overrun flag in 2 Lines communication mode because received is not read */
	if (&hspi1.Init.Direction == SPI_DIRECTION_2LINES) {
		__HAL_SPI_CLEAR_OVRFLAG(&hspi1);
	}


}

void spi_transmit_receive(uint8_t *outpp, uint8_t *inpp, uint32_t count) {

	uint8_t *outp = outpp;
	uint8_t *inp = inpp;

	if ((count > 1U)) {
		/* set fiforxthreshold according the reception data length: 16bit */
		SPI1->CR2 &= ~SPI_RXFIFO_THRESHOLD;
	} else {
		/* set fiforxthreshold according the reception data length: 8bit */
		SPI1->CR2 |= SPI_RXFIFO_THRESHOLD;
	}

	while (count) {
		while (!(SPI1->SR & SPI_SR_TXE))
			;
		if (count > 1U) {
			SPI1->DR = *((uint16_t *) outp);
			outp += sizeof(uint16_t);

		} else {
			*(__IO uint8_t *) &SPI1->DR = (*outp++);

		}
		while (!(SPI1->SR & SPI_SR_RXNE))
			;
		if (count > 1U) {
			*((uint16_t *) inp) = SPI1->DR;
			inp += sizeof(uint16_t);
			count -= 2U;
			if (count <= 1U) {
				/* set fiforxthresold before to switch on 8 bit data size */
				SPI1->CR2 |= SPI_RXFIFO_THRESHOLD;

			}
		} else {
			(*(uint8_t *) inp++) = *(__IO uint8_t *) &SPI1->DR;
			count--;
		}
	}

}

void spi_receive(uint8_t *inpp, uint32_t count) {

	uint8_t *inp = inpp;

	if ((count > 1U)) {
		/* set fiforxthreshold according the reception data length: 16bit */
		SPI1->CR2 &= ~SPI_RXFIFO_THRESHOLD;
	} else {
		/* set fiforxthreshold according the reception data length: 8bit */
		SPI1->CR2 |= SPI_RXFIFO_THRESHOLD;
	}

	while (count) {
		while (!(SPI1->SR & SPI_SR_TXE))
			;
		if (count > 1U) {
			SPI1->DR = *((uint16_t *) 0);
		} else {
			*(__IO uint8_t *) &SPI1->DR = 0;

		}
		while (!(SPI1->SR & SPI_SR_RXNE))
			;
		if (count > 1U) {
			*((uint16_t *) inp) = SPI1->DR;
			inp += sizeof(uint16_t);
			count -= 2U;
			if (count <= 1U) {
				/* set fiforxthresold before to switch on 8 bit data size */
				SPI1->CR2 |= SPI_RXFIFO_THRESHOLD;

			}
		} else {
			(*(uint8_t *) inp++) = *(__IO uint8_t *) &SPI1->DR;
			count--;
		}
	}
}

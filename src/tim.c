#include "tim.h"
TIM_HandleTypeDef htim1;
TIM_HandleTypeDef htim3;
TIM_HandleTypeDef htim14;

void tim1_init(void) {
	TIM_MasterConfigTypeDef sMasterConfig = { 0 };
	TIM_OC_InitTypeDef sConfigOC = { 0 };
	TIM_BreakDeadTimeConfigTypeDef sBreakDeadTimeConfig = { 0 };

	htim1.Instance = TIM1;
	htim1.Init.Prescaler = 48;
	htim1.Init.CounterMode = TIM_COUNTERMODE_UP;
	htim1.Init.Period = 10000;
	htim1.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
	htim1.Init.RepetitionCounter = 0;
	htim1.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
	if (HAL_TIM_PWM_Init(&htim1) != HAL_OK) {
		Error_Handler();
	}
	sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
	sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
	if (HAL_TIMEx_MasterConfigSynchronization(&htim1, &sMasterConfig)
			!= HAL_OK) {
		Error_Handler();
	}
	sConfigOC.OCMode = TIM_OCMODE_PWM1;
	sConfigOC.Pulse = 5000;
	sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
	sConfigOC.OCNPolarity = TIM_OCNPOLARITY_HIGH;
	sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
	sConfigOC.OCIdleState = TIM_OCIDLESTATE_RESET;
	sConfigOC.OCNIdleState = TIM_OCNIDLESTATE_RESET;
	if (HAL_TIM_PWM_ConfigChannel(&htim1, &sConfigOC, TIM_CHANNEL_2)
			!= HAL_OK) {
		Error_Handler();
	}
	sBreakDeadTimeConfig.OffStateRunMode = TIM_OSSR_DISABLE;
	sBreakDeadTimeConfig.OffStateIDLEMode = TIM_OSSI_DISABLE;
	sBreakDeadTimeConfig.LockLevel = TIM_LOCKLEVEL_OFF;
	sBreakDeadTimeConfig.DeadTime = 0;
	sBreakDeadTimeConfig.BreakState = TIM_BREAK_DISABLE;
	sBreakDeadTimeConfig.BreakPolarity = TIM_BREAKPOLARITY_HIGH;
	sBreakDeadTimeConfig.AutomaticOutput = TIM_AUTOMATICOUTPUT_DISABLE;
	if (HAL_TIMEx_ConfigBreakDeadTime(&htim1, &sBreakDeadTimeConfig)
			!= HAL_OK) {
		Error_Handler();
	}
	HAL_TIM_MspPostInit(&htim1);

	pwm1_set_dutycycle(0);
	TIM1->CR1 |= TIM_CR1_CEN;

}
/* TIM3 init function */
void tim3_init(void) {
	TIM_MasterConfigTypeDef sMasterConfig = { 0 };
	TIM_IC_InitTypeDef sConfigIC = { 0 };

	htim3.Instance = TIM3;
	htim3.Init.Prescaler = 0;
	htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
	htim3.Init.Period = 0;
	htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
	htim3.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
	if (HAL_TIM_IC_Init(&htim3) != HAL_OK) {
		Error_Handler();
	}
	sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
	sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
	if (HAL_TIMEx_MasterConfigSynchronization(&htim3, &sMasterConfig)
			!= HAL_OK) {
		Error_Handler();
	}
	sConfigIC.ICPolarity = TIM_INPUTCHANNELPOLARITY_RISING;
	sConfigIC.ICSelection = TIM_ICSELECTION_DIRECTTI;
	sConfigIC.ICPrescaler = TIM_ICPSC_DIV1;
	sConfigIC.ICFilter = 0;
	if (HAL_TIM_IC_ConfigChannel(&htim3, &sConfigIC, TIM_CHANNEL_1) != HAL_OK) {
		Error_Handler();
	}
	if (HAL_TIM_IC_ConfigChannel(&htim3, &sConfigIC, TIM_CHANNEL_2) != HAL_OK) {
		Error_Handler();
	}

}
/* TIM14 init function */
void tim14_init(void) {
	TIM_OC_InitTypeDef sConfigOC = { 0 };

	htim14.Instance = TIM14;
	htim14.Init.Prescaler = 48;
	htim14.Init.CounterMode = TIM_COUNTERMODE_UP;
	htim14.Init.Period = 10000;
	htim14.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
	htim14.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
	if (HAL_TIM_Base_Init(&htim14) != HAL_OK) {
		Error_Handler();
	}
	if (HAL_TIM_PWM_Init(&htim14) != HAL_OK) {
		Error_Handler();
	}
	sConfigOC.OCMode = TIM_OCMODE_PWM1;
	sConfigOC.Pulse = 3000;
	sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
	sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
	if (HAL_TIM_PWM_ConfigChannel(&htim14, &sConfigOC, TIM_CHANNEL_1)
			!= HAL_OK) {
		Error_Handler();
	}
	HAL_TIM_MspPostInit(&htim14);

	pwm2_set_dutycycle(0);

	TIM14->CR1 |= TIM_CR1_CEN;

}

void HAL_TIM_PWM_MspInit(TIM_HandleTypeDef* tim_pwmHandle) {

	if (tim_pwmHandle->Instance == TIM1) {
		/* USER CODE BEGIN TIM1_MspInit 0 */

		/* USER CODE END TIM1_MspInit 0 */
		/* TIM1 clock enable */
		__HAL_RCC_TIM1_CLK_ENABLE()
		;
		/* USER CODE BEGIN TIM1_MspInit 1 */

		/* USER CODE END TIM1_MspInit 1 */
	}
}

void HAL_TIM_IC_MspInit(TIM_HandleTypeDef* tim_icHandle) {

	GPIO_InitTypeDef GPIO_InitStruct = { 0 };
	if (tim_icHandle->Instance == TIM3) {
		/* USER CODE BEGIN TIM3_MspInit 0 */

		/* USER CODE END TIM3_MspInit 0 */
		/* TIM3 clock enable */
		__HAL_RCC_TIM3_CLK_ENABLE()
		;

		__HAL_RCC_GPIOC_CLK_ENABLE()
		;
		/**TIM3 GPIO Configuration
		 PC6     ------> TIM3_CH1
		 PC7     ------> TIM3_CH2
		 */
		GPIO_InitStruct.Pin = rotary_A_Pin | rotary_B_Pin;
		GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
		GPIO_InitStruct.Pull = GPIO_NOPULL;
		GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
		GPIO_InitStruct.Alternate = GPIO_AF0_TIM3;
		HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

		/* USER CODE BEGIN TIM3_MspInit 1 */

		/* USER CODE END TIM3_MspInit 1 */
	}
}

void HAL_TIM_Base_MspInit(TIM_HandleTypeDef* tim_baseHandle) {

	if (tim_baseHandle->Instance == TIM14) {
		/* USER CODE BEGIN TIM14_MspInit 0 */

		/* USER CODE END TIM14_MspInit 0 */
		/* TIM14 clock enable */
		__HAL_RCC_TIM14_CLK_ENABLE()
		;
		/* USER CODE BEGIN TIM14_MspInit 1 */

		/* USER CODE END TIM14_MspInit 1 */
	}
}
void HAL_TIM_MspPostInit(TIM_HandleTypeDef* timHandle) {

	GPIO_InitTypeDef GPIO_InitStruct = { 0 };
	if (timHandle->Instance == TIM1) {
		/* USER CODE BEGIN TIM1_MspPostInit 0 */

		/* USER CODE END TIM1_MspPostInit 0 */
		__HAL_RCC_GPIOB_CLK_ENABLE()
		;
		/**TIM1 GPIO Configuration
		 PB0     ------> TIM1_CH2N
		 */
		GPIO_InitStruct.Pin = PWM_Out_1_Pin;
		GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
		GPIO_InitStruct.Pull = GPIO_NOPULL;
		GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
		GPIO_InitStruct.Alternate = GPIO_AF2_TIM1;
		HAL_GPIO_Init(PWM_Out_1_GPIO_Port, &GPIO_InitStruct);

		/* USER CODE BEGIN TIM1_MspPostInit 1 */

		/* USER CODE END TIM1_MspPostInit 1 */
	} else if (timHandle->Instance == TIM14) {
		/* USER CODE BEGIN TIM14_MspPostInit 0 */

		/* USER CODE END TIM14_MspPostInit 0 */

		__HAL_RCC_GPIOB_CLK_ENABLE()
		;
		/**TIM14 GPIO Configuration
		 PB1     ------> TIM14_CH1
		 */
		GPIO_InitStruct.Pin = PWM_Out_2_Pin;
		GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
		GPIO_InitStruct.Pull = GPIO_NOPULL;
		GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
		GPIO_InitStruct.Alternate = GPIO_AF0_TIM14;
		HAL_GPIO_Init(PWM_Out_2_GPIO_Port, &GPIO_InitStruct);

		/* USER CODE BEGIN TIM14_MspPostInit 1 */

		/* USER CODE END TIM14_MspPostInit 1 */
	}

}

void HAL_TIM_PWM_MspDeInit(TIM_HandleTypeDef* tim_pwmHandle) {

	if (tim_pwmHandle->Instance == TIM1) {
		/* USER CODE BEGIN TIM1_MspDeInit 0 */

		/* USER CODE END TIM1_MspDeInit 0 */
		/* Peripheral clock disable */
		__HAL_RCC_TIM1_CLK_DISABLE();
		/* USER CODE BEGIN TIM1_MspDeInit 1 */

		/* USER CODE END TIM1_MspDeInit 1 */
	}
}

void HAL_TIM_IC_MspDeInit(TIM_HandleTypeDef* tim_icHandle) {

	if (tim_icHandle->Instance == TIM3) {
		/* USER CODE BEGIN TIM3_MspDeInit 0 */

		/* USER CODE END TIM3_MspDeInit 0 */
		/* Peripheral clock disable */
		__HAL_RCC_TIM3_CLK_DISABLE();

		/**TIM3 GPIO Configuration
		 PC6     ------> TIM3_CH1
		 PC7     ------> TIM3_CH2
		 */
		HAL_GPIO_DeInit(GPIOC, rotary_A_Pin | rotary_B_Pin);

		/* USER CODE BEGIN TIM3_MspDeInit 1 */

		/* USER CODE END TIM3_MspDeInit 1 */
	}
}

void HAL_TIM_Base_MspDeInit(TIM_HandleTypeDef* tim_baseHandle) {

	if (tim_baseHandle->Instance == TIM14) {
		/* USER CODE BEGIN TIM14_MspDeInit 0 */

		/* USER CODE END TIM14_MspDeInit 0 */
		/* Peripheral clock disable */
		__HAL_RCC_TIM14_CLK_DISABLE();
		/* USER CODE BEGIN TIM14_MspDeInit 1 */

		/* USER CODE END TIM14_MspDeInit 1 */
	}
}

void pwm1_set_dutycycle(float duty) {

	if (duty > 100) {
		duty = 100;
		TIM1->CCR2 = TIM1->ARR;
	} else if (duty < 0) {
		duty = 0;
		TIM1->CCR2 = 0;
	}

	TIM1->CCR2 = (uint16_t) (((float) TIM1->ARR * duty) / 100);

}

void pwm2_set_dutycycle(float duty) {

	if (duty > 100) {
		duty = 100;
		TIM14->CCR1 = TIM14->ARR;
	} else if (duty < 0) {
		duty = 0;
		TIM14->CCR1 = 0;
	}

	TIM14->CCR1 = (uint16_t) (((float) TIM14->ARR * duty) / 100);

}


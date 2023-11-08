/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2023 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "uart.h"
#include "btn.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
UART_HandleTypeDef huart6;

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART6_UART_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */

typedef enum {
	READ_NUM1,
	READ_NUM2,
	CALC_WRITE
} CalcState;

typedef enum {
	PLUS,
	MINUS,
	MUL,
	DIV
} Operation;

int main(void)
{
  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_USART6_UART_Init();
  /* USER CODE BEGIN 2 */
  UART_Init(&huart6);
  UART_Set_Mode(INT_DISABLE);
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  unsigned char input;
  char res_string[64];
  CalcState cur_state = READ_NUM1;
  CalcState next_state = READ_NUM1;
  int16_t num1 = 0;
  int16_t num2 = 0;
  Operation op;
  UART_Mode uart_mode = INT_DISABLE;
  UART_Ret_Code status;
  unsigned int digits_read = 0;
  int16_t result = 0;
  UART_Send_Data((uint8_t*)"Hail!\n\r", 7);
  bool error = false;
  while (1)
  {
	enum ButtonState btn = Get_Button_State();
	if (btn != NONE) {
		next_state = READ_NUM1;
		num1 = 0;
		num2 = 0;
		digits_read = 0;
		uart_mode = uart_mode == INT_DISABLE ? INT_ENABLE : INT_DISABLE;
		UART_Set_Mode(uart_mode);
		int l = sprintf(res_string, "Mode was switched to %s\n\r", uart_mode == INT_DISABLE ? "INT_DISABLE" : "INT_ENABLE");
		UART_Send_Data((uint8_t*)res_string, l);

	}
    switch (cur_state) {
    case READ_NUM1:
    	status = UART_Read_Data(&input, 1);
    	if (status != UART_SUCCESS) {
    		next_state = READ_NUM1;
    		break;
    	}
    	if ('0' <= input && input <= '9') {
    		next_state = READ_NUM1;
    		++digits_read;
    		if (digits_read > 5) {
    			break;
    		}
    	    UART_Send_Data(&input, 1);
    		num1 = num1 * 10 + (input - '0');
    	}
    	if (input == '+' && digits_read > 0) {
    		op = PLUS;
    		next_state = READ_NUM2;
    		UART_Send_Data(&input, 1);
    	}
    	if (input == '-' && digits_read > 0) {
    		op = MINUS;
    		next_state = READ_NUM2;
    		UART_Send_Data(&input, 1);
    	}
    	if (input == '*' && digits_read > 0) {
    		op = MUL;
    		next_state = READ_NUM2;
    		UART_Send_Data(&input, 1);
    	}
    	if (input == '/' && digits_read > 0) {
    		op = DIV;
			next_state = READ_NUM2;
			UART_Send_Data(&input, 1);
    	}
    	break;
    case READ_NUM2:
		status = UART_Read_Data(&input, 1);
		if (status != UART_SUCCESS) {
			next_state = READ_NUM2;
			break;
		}
		if ('0' <= input && input <= '9') {
			next_state = READ_NUM2;
			++digits_read;
			if (digits_read > 5) {
				break;
			}
			UART_Send_Data(&input, 1);
			num2 = num2 * 10 + (input - '0');
		}
		if (input == '=' && digits_read > 0) {
			next_state = CALC_WRITE;
			UART_Send_Data(&input, 1);
		}
		break;
    case CALC_WRITE:
    	if (num1 < 0 || num2 < 0) {
    		error = true;
    	}
    	switch (op) {
    	case PLUS:
    		result = num1 + num2;
    		if (result < 0) {
    			error = true;
    		}
    		break;
    	case MINUS:
    		result = num1 - num2;
    		if ((num1 > num2 && result < 0) || (num1 < num2 && result > 0)) {
    			error = true;
    		}
    		break;
    	case MUL:
    		result = num1 * num2;
    		if (result < 0) {
    			error = true;
    		}
    		break;
    	case DIV:
    		if (num2 == 0) {
    			error = true;
    		}
    		result = num1 / num2;
    		break;
    	}
    	int l = error ? sprintf(res_string, "error!\n\r") : sprintf(res_string, "%d\n\r", result);
    	while (UART_Get_State() == BUSY_SENDING);
    	UART_Send_Data((uint8_t*)res_string, l);
    	next_state = READ_NUM1;
    	if (error) {
    		HAL_GPIO_WritePin(GPIOD, GPIO_PIN_15, GPIO_PIN_SET);
    		HAL_Delay(1000);
    		HAL_GPIO_WritePin(GPIOD, GPIO_PIN_15, GPIO_PIN_RESET);
    	}
    	error = false;
    	num1 = 0;
    	num2 = 0;
    }
    if (cur_state != next_state) {
    	digits_read = 0;
    	cur_state = next_state;
    }
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE3);
  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief USART6 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART6_UART_Init(void)
{

  /* USER CODE BEGIN USART6_Init 0 */

  /* USER CODE END USART6_Init 0 */

  /* USER CODE BEGIN USART6_Init 1 */

  /* USER CODE END USART6_Init 1 */
  huart6.Instance = USART6;
  huart6.Init.BaudRate = 19200;
  huart6.Init.WordLength = UART_WORDLENGTH_8B;
  huart6.Init.StopBits = UART_STOPBITS_1;
  huart6.Init.Parity = UART_PARITY_NONE;
  huart6.Init.Mode = UART_MODE_TX_RX;
  huart6.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart6.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart6) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART6_Init 2 */

  /* USER CODE END USART6_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOD, GPIO_PIN_13|GPIO_PIN_14|GPIO_PIN_15, GPIO_PIN_RESET);

  /*Configure GPIO pin : PC15 */
  GPIO_InitStruct.Pin = GPIO_PIN_15;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pins : PD13 PD14 PD15 */
  GPIO_InitStruct.Pin = GPIO_PIN_13|GPIO_PIN_14|GPIO_PIN_15;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

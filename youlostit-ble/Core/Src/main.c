/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2021 STMicroelectronics.
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
//#include "ble_commands.h"
#include "ble.h"
#include "timer.h"
#include "i2c.h"
#include "lsm6dsl.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define LSM6DSL_ADDR 0x6A      			// Accelerometer address
#define WHO_AM_I_REG 0x0F      			// WHO_AM_I register for LSM6DSL

#define MOVEMENT_THRESHOLD 2000     	// Max movement until movement triggered
#define LOST_TIME_THRESHOLD 60000  		// 60 seconds in milliseconds

#define PREAMBLE 0b01100110       		// 8-bit preamble
#define USER_ID  0b0001101011011100  	// 16-bit ID (6876)
#define TOTAL_BITS 32					// 8 bit preamble + 16 bit userID + 8 bit time since lost

// Global Variables for states
volatile uint8_t timer_flag = 0;	 	// timer flag which is set by interrupt handler, read/cleared by main loop
volatile uint8_t is_lost = 0;		 	// flag for lost state.
volatile uint32_t time_still = 0;	 	// value for how long device has been still

unsigned char device_name[] = "TannTag";

int dataAvailable = 0;

SPI_HandleTypeDef hspi3;

void privtag_run();
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_SPI3_Init(void);

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* Configure the system clock */
  SystemClock_Config();

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_SPI3_Init();

  //RESET BLE MODULE
  HAL_GPIO_WritePin(BLE_RESET_GPIO_Port,BLE_RESET_Pin,GPIO_PIN_RESET);
  HAL_Delay(10);
  HAL_GPIO_WritePin(BLE_RESET_GPIO_Port,BLE_RESET_Pin,GPIO_PIN_SET);

  ble_init();

  HAL_Delay(10);

  privtag_run();

//uint8_t nonDiscoverable = 0;

//  while (1)
//  {
//	  if(!nonDiscoverable && HAL_GPIO_ReadPin(BLE_INT_GPIO_Port,BLE_INT_Pin)){
//	    catchBLE();
//	  }else{
//		  HAL_Delay(1000);
//		  // Send a string to the NORDIC UART service, remember to not include the newline
//		  unsigned char test_str[] = "youlostit BLE (tanner)";
//		  updateCharValue(NORDIC_UART_SERVICE_HANDLE, READ_CHAR_HANDLE, 0, sizeof(test_str)-1, test_str);
//	  }
//	  // Wait for interrupt, only uncomment if low power is needed
//	  //__WFI();
//  }
}

void privtag_run() {
	//Initialize peripherals
	i2c_init();
	lsm6dsl_init();

	//Initialize timer to be in 100 ms intervals
	timer_init(TIM2);
	timer_set_ms(TIM2, 50);

	int16_t x, y, z;
	int16_t prev_x, prev_y, prev_z;

	uint8_t device_moved_flag;


	while (1) {

		if (timer_flag) { // Every time there is a timer tick (currently set at 100ns intervals (1/10 s))...
			timer_flag = 0;
			lsm6dsl_read_xyz(&x, &y, &z);  // Read acceleration data

			// Calculate total magnitude of change in movement
			int32_t delta_x = abs(x - prev_x);
			int32_t delta_y = abs(y - prev_y);
			int32_t delta_z = abs(z - prev_z);
			int32_t total_movement = delta_x + delta_y + delta_z;

//			printf("X: %6d  [Change: %5d]   Y: %6d  [Change: %5d]   Z: %6d  [Change: %5d]\n",
//					  x, x - prev_x,
//					  y, y - prev_y,
//					  z, z - prev_z);
//			printf("Total Movement: %d\n",total_movement);

			prev_x = x;
			prev_y = y;
			prev_z = z;

			//If our device's total movement is beyond threshold, update device moved flag.
			if (total_movement > MOVEMENT_THRESHOLD) { device_moved_flag = 1; }
			else { device_moved_flag = 0; }

			//If device DID move.
			if (device_moved_flag) {
				is_lost = 0;
				time_still = 0;
			}
			else {
				if (time_still >= LOST_TIME_THRESHOLD && !is_lost) { // If has been still for THRESHOLD, and isnt already lost...
					is_lost = 1;									 // THEN, device is LOST! (Enter lost mode)
				}
			}

			if (is_lost) { //BLINKING LOGIC

				// calculating minutes lost
				//uint8_t minutes_since_lost = (time_still - LOST_TIME_THRESHOLD) / 60000;
				uint16_t seconds_since_lost = (time_still - LOST_TIME_THRESHOLD) / 1000;


				if ((time_still % 10000) == 0) {
				    unsigned char formatted_str[32];
				    snprintf((char*)formatted_str, sizeof(formatted_str), "%s %u", device_name, seconds_since_lost);

				    // Use strlen to get the actual string length
				    int str_len = strlen((char*)formatted_str);

				    updateCharValue(NORDIC_UART_SERVICE_HANDLE, READ_CHAR_HANDLE, 0, str_len, formatted_str);
				}

				//printf("device is currently lost --> %d --> min: %d\n", time_still, minutes_since_lost);
			}
			else {
				//printf("device is not lost --> time still: %d\n",time_still);
			}

		}
	}

}

void TIM2_IRQHandler()
{
	time_still = time_still + 50;
    TIM2->SR &= ~TIM_SR_UIF;  // Clear interrupt flag
    timer_flag = 1;      	  // Set flag for main loop
}

/**
  * @brief System Clock Configuration
  * @attention This changes the System clock frequency, make sure you reflect that change in your timer
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  if (HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_MSI;
  RCC_OscInitStruct.MSIState = RCC_MSI_ON;
  RCC_OscInitStruct.MSICalibrationValue = 0;
  // This lines changes system clock frequency
  RCC_OscInitStruct.MSIClockRange = RCC_MSIRANGE_7;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_MSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief SPI3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SPI3_Init(void)
{

  /* USER CODE BEGIN SPI3_Init 0 */

  /* USER CODE END SPI3_Init 0 */

  /* USER CODE BEGIN SPI3_Init 1 */

  /* USER CODE END SPI3_Init 1 */
  /* SPI3 parameter configuration*/
  hspi3.Instance = SPI3;
  hspi3.Init.Mode = SPI_MODE_MASTER;
  hspi3.Init.Direction = SPI_DIRECTION_2LINES;
  hspi3.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi3.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi3.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi3.Init.NSS = SPI_NSS_SOFT;
  hspi3.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_2;
  hspi3.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi3.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi3.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi3.Init.CRCPolynomial = 7;
  hspi3.Init.CRCLength = SPI_CRC_LENGTH_DATASIZE;
  hspi3.Init.NSSPMode = SPI_NSS_PULSE_ENABLE;
  if (HAL_SPI_Init(&hspi3) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI3_Init 2 */

  /* USER CODE END SPI3_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
/* USER CODE BEGIN MX_GPIO_Init_1 */
/* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOE_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIO_LED1_GPIO_Port, GPIO_LED1_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(BLE_CS_GPIO_Port, BLE_CS_Pin, GPIO_PIN_SET);


  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(BLE_RESET_GPIO_Port, BLE_RESET_Pin, GPIO_PIN_SET);

  /*Configure GPIO pin : BLE_INT_Pin */
  GPIO_InitStruct.Pin = BLE_INT_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(BLE_INT_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : GPIO_LED1_Pin BLE_RESET_Pin */
  GPIO_InitStruct.Pin = GPIO_LED1_Pin|BLE_RESET_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pin : BLE_CS_Pin */
  GPIO_InitStruct.Pin = BLE_CS_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  HAL_GPIO_Init(BLE_CS_GPIO_Port, &GPIO_InitStruct);

  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(EXTI9_5_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI9_5_IRQn);

/* USER CODE BEGIN MX_GPIO_Init_2 */
/* USER CODE END MX_GPIO_Init_2 */
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

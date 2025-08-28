/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f1xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define IO5_Pin GPIO_PIN_3
#define IO5_GPIO_Port GPIOA
#define IO4_Pin GPIO_PIN_4
#define IO4_GPIO_Port GPIOA
#define IO7_Pin GPIO_PIN_5
#define IO7_GPIO_Port GPIOA
#define IO6_Pin GPIO_PIN_6
#define IO6_GPIO_Port GPIOA
#define IO3_Pin GPIO_PIN_7
#define IO3_GPIO_Port GPIOA
#define IO2_Pin GPIO_PIN_0
#define IO2_GPIO_Port GPIOB
#define IO1_Pin GPIO_PIN_11
#define IO1_GPIO_Port GPIOB
#define KEY3_Pin GPIO_PIN_12
#define KEY3_GPIO_Port GPIOB
#define KEY3_EXTI_IRQn EXTI15_10_IRQn
#define KEY0_Pin GPIO_PIN_14
#define KEY0_GPIO_Port GPIOB
#define KEY0_EXTI_IRQn EXTI15_10_IRQn
#define KEY1_Pin GPIO_PIN_8
#define KEY1_GPIO_Port GPIOA
#define KEY1_EXTI_IRQn EXTI9_5_IRQn
#define KEY2_Pin GPIO_PIN_10
#define KEY2_GPIO_Port GPIOA
#define KEY2_EXTI_IRQn EXTI15_10_IRQn
#define FM225_CTL_Pin GPIO_PIN_5
#define FM225_CTL_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */

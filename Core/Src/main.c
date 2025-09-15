/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.c
 * @brief          : Main program body
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
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "dma.h"
#include "gpio.h"
#include "i2c.h"
#include "rtc.h"
#include "tim.h"
#include "usart.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "fm225.h"
#include "oled.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>

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

/* USER CODE BEGIN PV */
uint8_t KEY0_PRESSED = 0;
uint8_t KEY1_PRESSED = 0;
uint8_t KEY2_PRESSED = 0;
uint8_t KEY3_PRESSED = 0;
uint8_t ENROLL_ID = 1;
uint8_t DELETE_ID = 1;

int menu_main();
int menu_enroll();
int menu_verify();
int menu_delete();
void OLED_ShowTime();
int (*menu)() = &menu_main;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
 * @brief  The application entry point.
 * @retval int
 */
int main(void) {

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick.
   */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_I2C1_Init();
  MX_RTC_Init();
  MX_USART1_UART_Init();
  MX_TIM1_Init();
  MX_TIM2_Init();
  MX_TIM3_Init();
  /* USER CODE BEGIN 2 */
  rtc_init_user();                             // RTC初始化
  __HAL_UART_ENABLE_IT(&huart1, UART_IT_IDLE); // 使能串口IDLE中断
  HAL_UARTEx_ReceiveToIdle_DMA(&huart1, (uint8_t *)RX_BUFFER, RX_BUFF_SIZE);
  HAL_TIM_Base_Start_IT(&htim1); // 按键消抖
  HAL_TIM_Base_Start_IT(&htim2); // 音频引脚下降沿的时长
  HAL_TIM_Base_Start_IT(&htim3); // OLED每秒更新RTC时钟
  OLED_Init();                   // OLED初始
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1) {
    if (KEY1_PRESSED == 1) {
      menu = menu_main;
    }
    if (KEY3_PRESSED == 1) {
      menu = menu_enroll;
    }
    if (KEY2_PRESSED == 1) {
      menu = menu_verify;
    }
    if (KEY0_PRESSED == 1) {
      menu = menu_delete;
    }
    if (menu != NULL) {
      KEY0_PRESSED = 0;
      KEY1_PRESSED = 0;
      KEY2_PRESSED = 0;
      KEY3_PRESSED = 0;
      menu();
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
void SystemClock_Config(void) {
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
   * in the RCC_OscInitTypeDef structure.
   */
  RCC_OscInitStruct.OscillatorType =
      RCC_OSCILLATORTYPE_HSE | RCC_OSCILLATORTYPE_LSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.LSEState = RCC_LSE_ON;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
   */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK |
                                RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK) {
    Error_Handler();
  }
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_RTC;
  PeriphClkInit.RTCClockSelection = RCC_RTCCLKSOURCE_LSE;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK) {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

// 计算 BCC 异或校验码的函数
uint8_t calculateBCC(uint8_t *data, int length) {
  uint8_t bcc = 0;
  for (int i = 0; i < length; i++) {
    // 对每个字节进行异或操作
    bcc ^= data[i];
  }
  return bcc;
}

int menu_main() {
  menu = NULL;
  OLED_ClearRows(2, 7); // 清空2~7行
  OLED_ShowTime();

  OLED_ShowCHinese(32, 2, 0, 0);
  OLED_ShowCHinese(48, 2, 1, 0);
  OLED_ShowCHinese(64, 2, 6, 0);
  OLED_ShowCHinese(80, 2, 7, 0);

  OLED_ShowCHinese(32, 4, 4, 0);
  OLED_ShowCHinese(48, 4, 5, 0);
  OLED_ShowCHinese(64, 4, 6, 0);
  OLED_ShowCHinese(80, 4, 7, 0);

  OLED_ShowCHinese(32, 6, 2, 0);
  OLED_ShowCHinese(48, 6, 3, 0);
  OLED_ShowCHinese(64, 6, 6, 0);
  OLED_ShowCHinese(80, 6, 7, 0);
  return 0;
}
int menu_enroll() {
  OLED_ClearRows(2, 7); // 清空2~7行
  OLED_ShowTime();
  OLED_ShowCHinese(0, 2, 13, 0);  // 再
  OLED_ShowCHinese(16, 2, 14, 0); // 按
  OLED_ShowCHinese(32, 2, 15, 0); // 一
  OLED_ShowCHinese(48, 2, 16, 0); // 次

  OLED_ShowCHinese(64, 2, 0, 0);  // 注
  OLED_ShowCHinese(80, 2, 1, 0);  // 册
  OLED_ShowCHinese(96, 2, 6, 0);  // 人
  OLED_ShowCHinese(112, 2, 7, 0); // 脸

  OLED_ShowCHinese(32, 4, 17, 0); // 序
  OLED_ShowCHinese(48, 4, 18, 0); // 号
  OLED_ShowCHinese(64, 4, 19, 0); // ：
  OLED_ShowNum(80, 4, ENROLL_ID, 2, 16, 0);
  HAL_GPIO_WritePin(FM225_CTL_GPIO_Port, FM225_CTL_Pin, GPIO_PIN_SET);

  while (1) {
    if (KEY3_PRESSED == 1) {
      KEY3_PRESSED = 0;

      OLED_ClearRows(2, 7); // 清空2~7行
      OLED_ShowTime();
      OLED_ShowCHinese(40, 2, 0, 0);  // 注
      OLED_ShowCHinese(56, 2, 1, 0);  // 册
      OLED_ShowCHinese(72, 2, 24, 0); // 中

      OLED_ShowCHinese(24, 4, 8, 0);  // 请
      OLED_ShowCHinese(40, 4, 20, 0); // 靠
      OLED_ShowCHinese(56, 4, 21, 0); // 近
      OLED_ShowCHinese(72, 4, 22, 0); // 设
      OLED_ShowCHinese(88, 4, 23, 0); // 备

      uint8_t values[] = {0xef, 0xaa, 0x1d, 0x00, 0x23, 0x00, 0x00, 0x00, 0x00,
                          0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                          0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                          0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                          0x00, 0x00, 0x00, 0x00, 0x3e};

      // 赋值操作
      for (int i = 0; i < 41; i++) {
        TX_BUFFER[i] = values[i];
      }

      TX_BUFFER[6] = ENROLL_ID;

      // 调用函数计算 BCC 校验码
      TX_BUFFER[40] = calculateBCC(&TX_BUFFER[2], 38);
      HAL_UART_Transmit_DMA(&huart1, (uint8_t *)TX_BUFFER, 41);
      while (1) {
        if ((RX_BUFFER[2] == 0X00) && (RX_BUFFER[5] == 0X1D) &&
            (RX_BUFFER[6] == 0X0A)) {
          // 播放人脸已录入语音
          HAL_GPIO_WritePin(IO3_GPIO_Port, IO3_Pin, GPIO_PIN_RESET);
          __HAL_TIM_SET_COUNTER(&htim2, 0);
          HAL_TIM_Base_Start_IT(&htim2);

          // 关闭FM225的电源
          HAL_GPIO_WritePin(FM225_CTL_GPIO_Port, FM225_CTL_Pin, GPIO_PIN_RESET);
          OLED_ClearRows(2, 7); // 清空2~7行
          OLED_ShowTime();
          OLED_ShowCHinese(24, 2, 6, 0);  // 人
          OLED_ShowCHinese(40, 2, 7, 0);  // 脸
          OLED_ShowCHinese(56, 2, 29, 0); // 已
          OLED_ShowCHinese(72, 2, 30, 0); // 录
          OLED_ShowCHinese(88, 2, 31, 0); // 入

          while (1) {
            if (KEY3_PRESSED == 1) {
              KEY3_PRESSED = 0;
            }
            if (KEY2_PRESSED == 1) {
              KEY2_PRESSED = 0;
            }

            if (KEY0_PRESSED == 1) {
              KEY0_PRESSED = 0;
            }
            if (KEY1_PRESSED == 1) {
              KEY1_PRESSED = 0;
              HAL_GPIO_WritePin(FM225_CTL_GPIO_Port, FM225_CTL_Pin,
                                GPIO_PIN_RESET);
              menu = menu_main;
              return 0;
            }
          }
        } else if ((RX_BUFFER[2] == 0X00) && (RX_BUFFER[5] == 0X1D) &&
                   (RX_BUFFER[6] == 0X00) && (RX_BUFFER[8] != 0X00)) {
          // 播放录入成功语音
          HAL_GPIO_WritePin(IO1_GPIO_Port, IO1_Pin, GPIO_PIN_RESET);
          __HAL_TIM_SET_COUNTER(&htim2, 0);
          HAL_TIM_Base_Start_IT(&htim2);

          // 关闭FM225的电源
          HAL_GPIO_WritePin(FM225_CTL_GPIO_Port, FM225_CTL_Pin, GPIO_PIN_RESET);
          OLED_ClearRows(2, 7); // 清空2~7行
          OLED_ShowCHinese(32, 2, 30, 0);
          OLED_ShowCHinese(48, 2, 31, 0);
          OLED_ShowCHinese(64, 2, 25, 0);
          OLED_ShowCHinese(80, 2, 26, 0);

          while (1) {
            if (KEY3_PRESSED == 1) {
              KEY3_PRESSED = 0;
            }
            if (KEY2_PRESSED == 1) {
              KEY2_PRESSED = 0;
            }

            if (KEY0_PRESSED == 1) {
              KEY0_PRESSED = 0;
            }
            if (KEY1_PRESSED == 1) {
              KEY1_PRESSED = 0;
              HAL_GPIO_WritePin(FM225_CTL_GPIO_Port, FM225_CTL_Pin,
                                GPIO_PIN_RESET);
              menu = menu_main;
              return 0;
            }
          }
        } else if ((RX_BUFFER[2] == 0X00) && (RX_BUFFER[5] == 0X1D) &&
                   (RX_BUFFER[6] == 0X0D)) {
          // 播放录入失败语音
          HAL_GPIO_WritePin(IO2_GPIO_Port, IO2_Pin, GPIO_PIN_RESET);
          __HAL_TIM_SET_COUNTER(&htim2, 0);
          HAL_TIM_Base_Start_IT(&htim2);

          // 关闭FM225的电源
          HAL_GPIO_WritePin(FM225_CTL_GPIO_Port, FM225_CTL_Pin, GPIO_PIN_RESET);
          OLED_ClearRows(2, 7); // 清空2~7行
          OLED_ShowCHinese(32, 2, 30, 0);
          OLED_ShowCHinese(48, 2, 31, 0);
          OLED_ShowCHinese(64, 2, 27, 0);
          OLED_ShowCHinese(80, 2, 28, 0);

          while (1) {
            if (KEY3_PRESSED == 1) {
              KEY3_PRESSED = 0;
            }
            if (KEY2_PRESSED == 1) {
              KEY2_PRESSED = 0;
            }

            if (KEY0_PRESSED == 1) {
              KEY0_PRESSED = 0;
            }
            if (KEY1_PRESSED == 1) {
              KEY1_PRESSED = 0;
              HAL_GPIO_WritePin(FM225_CTL_GPIO_Port, FM225_CTL_Pin,
                                GPIO_PIN_RESET);
              menu = menu_main;
              return 0;
            }
          }
        }
        if (KEY2_PRESSED == 1) {
          KEY2_PRESSED = 0;
        }
        if (KEY3_PRESSED == 1) {
          KEY3_PRESSED = 0;
        }
        if (KEY0_PRESSED == 1) {
          KEY0_PRESSED = 0;
        }
        if (KEY1_PRESSED == 1) {
          KEY1_PRESSED = 0;
          HAL_GPIO_WritePin(FM225_CTL_GPIO_Port, FM225_CTL_Pin, GPIO_PIN_RESET);
          menu = menu_main;
          return 0;
        }
      }
    }
    if (KEY2_PRESSED == 1) {
      KEY2_PRESSED = 0;
      if (ENROLL_ID < 99) {
        ENROLL_ID++;
        OLED_ShowNum(80, 4, ENROLL_ID, 2, 16, 0);
      }
    }
    if (KEY0_PRESSED == 1) {
      KEY0_PRESSED = 0;
      if (ENROLL_ID > 1) {
        ENROLL_ID--;
        OLED_ShowNum(80, 4, ENROLL_ID, 2, 16, 0);
      }
    }
    if (KEY1_PRESSED == 1) {

      KEY1_PRESSED = 0;
      HAL_GPIO_WritePin(FM225_CTL_GPIO_Port, FM225_CTL_Pin, GPIO_PIN_RESET);
      menu = menu_main;
      return 0;
    }
  }
}
int menu_verify() {
  while (1) {
    HAL_GPIO_WritePin(FM225_CTL_GPIO_Port, FM225_CTL_Pin, GPIO_PIN_SET);
    OLED_ClearRows(2, 7);           // 清空2~7行
    OLED_ShowCHinese(24, 2, 8, 0);  // 请
    OLED_ShowCHinese(40, 2, 20, 0); // 靠
    OLED_ShowCHinese(56, 2, 21, 0); // 近
    OLED_ShowCHinese(72, 2, 22, 0); // 设
    OLED_ShowCHinese(88, 2, 23, 0); // 备

    uint8_t ready_msg[7] = {0XEF, 0XAA, 0X01, 0X00, 0X01, 0X00,  0X00};

    // 等待收到开机准备好的消息
    while (strcmp((const char *)user_buffer, (const char *)ready_msg) != 0) {
      continue;
    }

    // 调用验证函数
    face_verify(0x01, 10);

    // 等待收到验证结果
    while (verify_received_data(user_buffer, user_buffer_len) == false) {
      continue;
    }

    while (1) {
      if (user_buffer[2] == 0X00 && user_buffer[5] == 0X12 &&
          user_buffer[6] == 0X00 && user_buffer[8] != 0X00) {

        // 重置 user_buffer 和 user_buffer_len
        user_buffer_len = 0;
        memset(user_buffer, 0x00, sizeof(user_buffer));

        // 播放验证成功语音
        HAL_GPIO_WritePin(IO5_GPIO_Port, IO5_Pin, GPIO_PIN_RESET);
        __HAL_TIM_SET_COUNTER(&htim2, 0);
        HAL_TIM_Base_Start_IT(&htim2);

        uint8_t id = user_buffer[8];
        OLED_ShowNum(72, 2, user_buffer[8], 2, 16, 0);
        HAL_GPIO_WritePin(IO6_GPIO_Port, IO6_Pin, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(FM225_CTL_GPIO_Port, FM225_CTL_Pin, GPIO_PIN_RESET);
        OLED_ClearRows(2, 7);           // 清空2~7行
        OLED_ShowCHinese(32, 2, 4, 0);  // 验
        OLED_ShowCHinese(48, 2, 5, 0);  // 证
        OLED_ShowCHinese(64, 2, 25, 0); // 成
        OLED_ShowCHinese(80, 2, 26, 0); // 功

        OLED_ShowCHinese(24, 4, 39, 0); // 库
        OLED_ShowCHinese(40, 4, 40, 0); // 中
        OLED_ShowCHinese(56, 4, 41, 0); // 第
        OLED_ShowNum(72, 4, id, 2, 16, 0);
        OLED_ShowCHinese(88, 4, 42, 0); // 个
      }
      if (user_buffer[2] == 0X00 && user_buffer[5] == 0X12 &&
          user_buffer[6] == 0X0D) {

        // 重置 user_buffer 和 user_buffer_len
        user_buffer_len = 0;
        memset(user_buffer, 0x00, sizeof(user_buffer));

        // 播放验证失败语音
        HAL_GPIO_WritePin(IO6_GPIO_Port, IO6_Pin, GPIO_PIN_RESET);
        __HAL_TIM_SET_COUNTER(&htim2, 0);
        HAL_TIM_Base_Start_IT(&htim2);

        HAL_GPIO_WritePin(IO7_GPIO_Port, IO7_Pin, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(FM225_CTL_GPIO_Port, FM225_CTL_Pin, GPIO_PIN_RESET);
        OLED_ClearRows(2, 7); // 清空2~7行
        OLED_ShowCHinese(32, 2, 4, 0); // 验
        OLED_ShowCHinese(48, 2, 5, 0); // 证
        OLED_ShowCHinese(64, 2, 27, 0); // 失
        OLED_ShowCHinese(80, 2, 28, 0); // 败
      }
      if (KEY2_PRESSED == 1) {
        KEY2_PRESSED = 0;
      }

      if (KEY0_PRESSED == 1) {
        KEY0_PRESSED = 0;
      }
      if (KEY3_PRESSED == 1) {
        KEY3_PRESSED = 0;
      }
      if (KEY1_PRESSED == 1) {
        KEY1_PRESSED = 0;
        HAL_GPIO_WritePin(FM225_CTL_GPIO_Port, FM225_CTL_Pin, GPIO_PIN_RESET);
        menu = menu_main;
        return 0;
      }
    }
  }
}
int menu_delete() {

  OLED_ClearRows(2, 7); // 清空2~7行
  OLED_ShowCHinese(0, 2, 13, 0); // 再
  OLED_ShowCHinese(16, 2, 14, 0); // 按
  OLED_ShowCHinese(32, 2, 15, 0); // 一
  OLED_ShowCHinese(48, 2, 16, 0); // 次

  OLED_ShowCHinese(64, 2, 2, 0); // 删
  OLED_ShowCHinese(80, 2, 3, 0); // 除
  OLED_ShowCHinese(96, 2, 6, 0); // 人
  OLED_ShowCHinese(112, 2, 7, 0); // 脸

  OLED_ShowCHinese(24, 4, 39, 0); // 库
  OLED_ShowCHinese(40, 4, 40, 0); // 中
  OLED_ShowCHinese(56, 4, 41, 0); // 第
  OLED_ShowNum(72, 4, DELETE_ID, 2, 16, 0);
  OLED_ShowCHinese(88, 4, 42, 0); // 个
  HAL_GPIO_WritePin(FM225_CTL_GPIO_Port, FM225_CTL_Pin, GPIO_PIN_SET);

  while (1) {
    if (KEY0_PRESSED == 1) {
      KEY0_PRESSED = 0;
      // 播放删除成功语音
      HAL_GPIO_WritePin(IO7_GPIO_Port, IO7_Pin, GPIO_PIN_RESET);
      __HAL_TIM_SET_COUNTER(&htim2, 0);
      HAL_TIM_Base_Start_IT(&htim2);

      if (DELETE_ID == 0) {
        OLED_ClearRows(2, 7); // 清空2~7行
        OLED_ShowCHinese(16, 2, 37, 0);
        OLED_ShowCHinese(32, 2, 38, 0);
        OLED_ShowCHinese(48, 2, 2, 0);
        OLED_ShowCHinese(64, 2, 3, 0);
        OLED_ShowCHinese(80, 2, 25, 0);
        OLED_ShowCHinese(96, 2, 26, 0);

        TX_BUFFER[0] = 0XEF;
        TX_BUFFER[1] = 0XAA;
        TX_BUFFER[2] = 0X21;
        TX_BUFFER[3] = 0X00;
        TX_BUFFER[4] = 0X00;
        TX_BUFFER[5] = 0X21;

        HAL_UART_Transmit_DMA(&huart1, TX_BUFFER, 6);
        HAL_UARTEx_ReceiveToIdle_DMA(&huart1, (uint8_t *)RX_BUFFER,
                                     RX_BUFF_SIZE); // 继续接收数据

      } else {
        OLED_ClearRows(2, 7); // 清空2~7行
        OLED_ShowCHinese(32, 2, 2, 0);
        OLED_ShowCHinese(48, 2, 3, 0);
        OLED_ShowCHinese(64, 2, 25, 0);
        OLED_ShowCHinese(80, 2, 26, 0);

        TX_BUFFER[0] = 0XEF;
        TX_BUFFER[1] = 0XAA;
        TX_BUFFER[2] = 0X20;
        TX_BUFFER[3] = 0X00;
        TX_BUFFER[4] = 0X02;
        TX_BUFFER[5] = 0X00;
        TX_BUFFER[6] = DELETE_ID;

        // 调用函数计算 BCC 校验码
        TX_BUFFER[7] = calculateBCC(&TX_BUFFER[2], 5);
        HAL_UART_Transmit_DMA(&huart1, TX_BUFFER, 8);
        HAL_UARTEx_ReceiveToIdle_DMA(&huart1, (uint8_t *)RX_BUFFER,
                                     RX_BUFF_SIZE); // 继续接收数据
      }
      while (1) {

        if (RX_BUFFER[2] == 0X00 && RX_BUFFER[5] == 0X20 &&
            RX_BUFFER[6] == 0X00) {
          HAL_GPIO_WritePin(FM225_CTL_GPIO_Port, FM225_CTL_Pin, GPIO_PIN_RESET);
        }

        if (RX_BUFFER[2] == 0X00 && RX_BUFFER[5] == 0X21 &&
            RX_BUFFER[6] == 0X00) {
          HAL_GPIO_WritePin(FM225_CTL_GPIO_Port, FM225_CTL_Pin, GPIO_PIN_RESET);
        }

        if (KEY3_PRESSED == 1) {
          KEY3_PRESSED = 0;
        }

        if (KEY2_PRESSED == 1) {
          KEY2_PRESSED = 0;
        }

        if (KEY0_PRESSED == 1) {
          KEY0_PRESSED = 0;
        }
        if (KEY1_PRESSED == 1) {
          KEY1_PRESSED = 0;
          HAL_GPIO_WritePin(FM225_CTL_GPIO_Port, FM225_CTL_Pin, GPIO_PIN_RESET);
          menu = menu_main;
          return 0;
        }
      }
    }
    if (KEY3_PRESSED == 1) {
      KEY3_PRESSED = 0;
      if (DELETE_ID < 99) {
        DELETE_ID++;
        OLED_ShowNum(72, 4, DELETE_ID, 2, 16, 0);
      }
    }
    if (KEY2_PRESSED == 1) {
      KEY2_PRESSED = 0;
      if (DELETE_ID > 0) {
        DELETE_ID--;
        OLED_ShowNum(72, 4, DELETE_ID, 2, 16, 0);
      }
    }
    if (KEY1_PRESSED == 1) {
      KEY1_PRESSED = 0;
      HAL_GPIO_WritePin(FM225_CTL_GPIO_Port, FM225_CTL_Pin, GPIO_PIN_RESET);
      menu = menu_main;
      return 0;
    }
  }
}
void OLED_ShowTime(void) {
  char buf[50]; // 存放 "YYYY-MM-DD HH:MM"

  /* 获取 RTC 当前时间和日期 */
  RTC_GetTime();

  /* 格式化日期 + 时分 */
  memset(buf, 0, sizeof(buf));
  sprintf(buf, "%04d-%02d-%02d %02d:%02d", date_info[0], date_info[1],
          date_info[2], date_info[3], date_info[4]);

  /* 显示在 OLED 第一行 */
  OLED_ShowString(0, 0, buf, 16, 0);
}

/* 定时器中断回调函数 */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {
  if (htim->Instance == TIM1) {
    /* 停止定时器 */
    HAL_TIM_Base_Stop_IT(&htim1);

    /* 读取按键状态 */
    if (HAL_GPIO_ReadPin(KEY0_GPIO_Port, KEY0_Pin) == GPIO_PIN_RESET) {
      KEY0_PRESSED = 1;
    }
    if (HAL_GPIO_ReadPin(KEY1_GPIO_Port, KEY1_Pin) == GPIO_PIN_RESET) {
      KEY1_PRESSED = 1;
    }
    if (HAL_GPIO_ReadPin(KEY2_GPIO_Port, KEY2_Pin) == GPIO_PIN_RESET) {
      KEY2_PRESSED = 1;
    }
    if (HAL_GPIO_ReadPin(KEY3_GPIO_Port, KEY3_Pin) == GPIO_PIN_RESET) {
      KEY3_PRESSED = 1;
    }
  }
  if (htim->Instance == TIM2) {
    /* 停止定时器 */
    HAL_TIM_Base_Stop_IT(&htim2);

    /* 复位按键状态：高电平 */
    HAL_GPIO_WritePin(IO1_GPIO_Port, IO1_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(IO2_GPIO_Port, IO2_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(IO3_GPIO_Port, IO3_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(IO4_GPIO_Port, IO4_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(IO5_GPIO_Port, IO5_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(IO6_GPIO_Port, IO6_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(IO7_GPIO_Port, IO7_Pin, GPIO_PIN_SET);
  }
}
// 外部中断回调函数
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
  // 检查是否为有效按键引脚
  if (GPIO_Pin == KEY0_Pin || GPIO_Pin == KEY1_Pin || GPIO_Pin == KEY2_Pin ||
      GPIO_Pin == KEY3_Pin) {
    // 清除定时器计数并启动定时器
    __HAL_TIM_SET_COUNTER(&htim1, 0);
    HAL_TIM_Base_Start_IT(&htim1);
  }
}
// UART接收事件回调函数
void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size) {
  if (huart->Instance == USART1) {

    HAL_UART_DMAStop(&huart1);

    // 保存数据
    memcpy(user_buffer, RX_BUFFER, Size);

    user_buffer_len = Size;

    // 再清空
    memset(RX_BUFFER, 0x00, sizeof(RX_BUFFER));

    HAL_UARTEx_ReceiveToIdle_DMA(&huart1, RX_BUFFER, RX_BUFF_SIZE);
  }
}
// RTC秒中断回调函数
void HAL_RTCEx_RTCEventCallback(RTC_HandleTypeDef *hrtc) { OLED_ShowTime(); }
/* USER CODE END 4 */

/**
 * @brief  This function is executed in case of error occurrence.
 * @retval None
 */
void Error_Handler(void) {
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1) {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
 * @brief  Reports the name of the source file and the source line number
 *         where the assert_param error has occurred.
 * @param  file: pointer to the source file name
 * @param  line: assert_param error line source number
 * @retval None
 */
void assert_failed(uint8_t *file, uint32_t line) {
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line
     number, ex: printf("Wrong parameters value: file %s on line %d\r\n", file,
     line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

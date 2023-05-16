/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2023 STMicroelectronics.
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
#include "stm32f0xx_hal.h"

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
#define ADDR3_Pin GPIO_PIN_13
#define ADDR3_GPIO_Port GPIOC
#define USER_BTN_Pin GPIO_PIN_15
#define USER_BTN_GPIO_Port GPIOC
#define EN_3V3S_Pin GPIO_PIN_0
#define EN_3V3S_GPIO_Port GPIOC
#define EN_5V_Pin GPIO_PIN_1
#define EN_5V_GPIO_Port GPIOC
#define EN_12V_Pin GPIO_PIN_2
#define EN_12V_GPIO_Port GPIOC
#define SPI1_SS_Pin GPIO_PIN_4
#define SPI1_SS_GPIO_Port GPIOA
#define D300_CAL1_Pin GPIO_PIN_0
#define D300_CAL1_GPIO_Port GPIOB
#define D300_CAL2_Pin GPIO_PIN_1
#define D300_CAL2_GPIO_Port GPIOB
#define D300_RESET_Pin GPIO_PIN_2
#define D300_RESET_GPIO_Port GPIOB
#define SBUS_TXE_Pin GPIO_PIN_12
#define SBUS_TXE_GPIO_Port GPIOB
#define HB_LED_Pin GPIO_PIN_13
#define HB_LED_GPIO_Port GPIOB
#define SBUS_CS_Pin GPIO_PIN_15
#define SBUS_CS_GPIO_Port GPIOB
#define PNTW_SET_Pin GPIO_PIN_6
#define PNTW_SET_GPIO_Port GPIOC
#define PNTW_RESET_Pin GPIO_PIN_7
#define PNTW_RESET_GPIO_Port GPIOC
#define VUSB_Pin GPIO_PIN_8
#define VUSB_GPIO_Port GPIOA
#define TX_LED_Pin GPIO_PIN_10
#define TX_LED_GPIO_Port GPIOC
#define RX_LED_Pin GPIO_PIN_11
#define RX_LED_GPIO_Port GPIOC
#define ADDR2_Pin GPIO_PIN_12
#define ADDR2_GPIO_Port GPIOC
#define SPARE1_Pin GPIO_PIN_3
#define SPARE1_GPIO_Port GPIOB
#define ADDR0_Pin GPIO_PIN_8
#define ADDR0_GPIO_Port GPIOB
#define ADDR1_Pin GPIO_PIN_9
#define ADDR1_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */

/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
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

void HAL_TIM_MspPostInit(TIM_HandleTypeDef *htim);

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define ADDR3_Pin GPIO_PIN_13
#define ADDR3_GPIO_Port GPIOC
#define USER_BTN_Pin GPIO_PIN_15
#define USER_BTN_GPIO_Port GPIOC
#define FAN_ALERT_Pin GPIO_PIN_0
#define FAN_ALERT_GPIO_Port GPIOC
#define PWM_VFBK_Pin GPIO_PIN_1
#define PWM_VFBK_GPIO_Port GPIOC
#define PWM_CFBK_Pin GPIO_PIN_2
#define PWM_CFBK_GPIO_Port GPIOC
#define VIN_FBK_Pin GPIO_PIN_3
#define VIN_FBK_GPIO_Port GPIOC
#define PWM14_Pin GPIO_PIN_4
#define PWM14_GPIO_Port GPIOA
#define PWM16_Pin GPIO_PIN_6
#define PWM16_GPIO_Port GPIOA
#define PWM17_P_Pin GPIO_PIN_7
#define PWM17_P_GPIO_Port GPIOA
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
#define K96_PWM_Pin GPIO_PIN_14
#define K96_PWM_GPIO_Port GPIOB
#define SBUS_CS_Pin GPIO_PIN_15
#define SBUS_CS_GPIO_Port GPIOB
#define K96_RDY_Pin GPIO_PIN_6
#define K96_RDY_GPIO_Port GPIOC
#define K96_EN_Pin GPIO_PIN_7
#define K96_EN_GPIO_Port GPIOC
#define VUSB_Pin GPIO_PIN_8
#define VUSB_GPIO_Port GPIOA
#define TX_LED_Pin GPIO_PIN_10
#define TX_LED_GPIO_Port GPIOC
#define RX_LED_Pin GPIO_PIN_11
#define RX_LED_GPIO_Port GPIOC
#define ADDR2_Pin GPIO_PIN_12
#define ADDR2_GPIO_Port GPIOC
#define SPARE_1_Pin GPIO_PIN_3
#define SPARE_1_GPIO_Port GPIOB
#define SPARE_2_Pin GPIO_PIN_6
#define SPARE_2_GPIO_Port GPIOB
#define PWM17_N_Pin GPIO_PIN_7
#define PWM17_N_GPIO_Port GPIOB
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

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

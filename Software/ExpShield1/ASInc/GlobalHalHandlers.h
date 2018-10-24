/* ===========================================================================
 * Copyright 2015 EUROPEAN UNION
 *
 * Licensed under the EUPL, Version 1.1 or subsequent versions of the
 * EUPL (the "License"); You may not use this work except in compliance
 * with the License. You may obtain a copy of the License at
 * http://ec.europa.eu/idabc/eupl
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * Date: 02/04/2015
 * Authors:
 * - Michel Gerboles, michel.gerboles@jrc.ec.europa.eu,
 *   Laurent Spinelle, laurent.spinelle@jrc.ec.europa.eu and
 *   Alexander Kotsev, alexander.kotsev@jrc.ec.europa.eu:
 *			European Commission - Joint Research Centre,
 * - Marco Signorini, marco.signorini@liberaintentio.com
 *
 * ===========================================================================
 */

#ifndef _GLOBALHALHANDLERS_H_
#define _GLOBALHALHANDLERS_H_

#include "stm32f0xx_hal.h"

#ifdef __cplusplus
 extern "C" {
#endif

 extern I2C_HandleTypeDef hi2c1;
 extern I2C_HandleTypeDef hi2c2;

 extern SPI_HandleTypeDef hspi1;

 extern TIM_HandleTypeDef htim17;

 extern UART_HandleTypeDef huart1;
 extern UART_HandleTypeDef huart2;
 extern UART_HandleTypeDef huart3;
 extern UART_HandleTypeDef huart4;


#ifdef __cplusplus
}
#endif


#endif /* _GLOBALHALHANDLERS_H_ */

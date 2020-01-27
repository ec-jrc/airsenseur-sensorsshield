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

#include "GPIOHelper.h"

// Singleton GPIOHelper instance
const GPIOHelper GPIOHelper::instance;
const uint16_t GPIOHelper::pinIDs[LAST_UNUSEDPIN] = { HB_LED_Pin, TX_LED_Pin, RX_LED_Pin, USER_BTN_Pin,
													ADDR0_Pin, ADDR1_Pin, ADDR2_Pin, ADDR3_Pin,
													SBUS_TXE_Pin, SBUS_CS_Pin,
													EN_3V3S_Pin, EN_5V_Pin, EN_12V_Pin,
													D300_CAL1_Pin, D300_CAL2_Pin, D300_RESET_Pin,
													PNTW_SET_Pin, PNTW_RESET_Pin, SPI1_SS_Pin,
													};
const GPIO_TypeDef* const GPIOHelper::pinPorts[LAST_UNUSEDPIN] = { HB_LED_GPIO_Port, TX_LED_GPIO_Port, RX_LED_GPIO_Port, USER_BTN_GPIO_Port,
																	ADDR0_GPIO_Port, ADDR1_GPIO_Port, ADDR2_GPIO_Port, ADDR3_GPIO_Port,
																	SBUS_TXE_GPIO_Port, SBUS_CS_GPIO_Port,
																	EN_3V3S_GPIO_Port, EN_5V_GPIO_Port, EN_12V_GPIO_Port,
																	D300_CAL1_GPIO_Port, D300_CAL2_GPIO_Port, D300_RESET_GPIO_Port,
																	PNTW_SET_GPIO_Port, PNTW_RESET_GPIO_Port, SPI1_SS_GPIO_Port
																};

GPIOHelper::GPIOHelper() {
}

GPIOHelper::~GPIOHelper() {
}

void GPIOHelper::digitalWrite(uint8_t pin, bool value) const {
	if (pin >= LAST_UNUSEDPIN)
		return;

	HAL_GPIO_WritePin((GPIO_TypeDef*)pinPorts[pin], pinIDs[pin], (value)? GPIO_PIN_SET:GPIO_PIN_RESET);
}

uint8_t GPIOHelper::digitalRead(uint8_t pin) const {
	if (pin >= LAST_UNUSEDPIN)
		return 0xFF;

	return (HAL_GPIO_ReadPin((GPIO_TypeDef*)pinPorts[pin], pinIDs[pin]) == GPIO_PIN_SET)? 0x01 : 0x00;
}

uint8_t GPIOHelper::getBoardId() const {

	 return ((HAL_GPIO_ReadPin(ADDR0_GPIO_Port, ADDR0_Pin) == GPIO_PIN_RESET) ? 0x01 : 0x00) +
			 ((HAL_GPIO_ReadPin(ADDR1_GPIO_Port, ADDR1_Pin) == GPIO_PIN_RESET) ? 0x02 : 0x00) +
			 ((HAL_GPIO_ReadPin(ADDR2_GPIO_Port, ADDR2_Pin) == GPIO_PIN_RESET) ? 0x04 : 0x00) +
			 ((HAL_GPIO_ReadPin(ADDR3_GPIO_Port, ADDR3_Pin) == GPIO_PIN_RESET) ? 0x08 : 0x00);
}

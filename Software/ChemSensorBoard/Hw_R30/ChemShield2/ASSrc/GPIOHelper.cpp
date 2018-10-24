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
const uint16_t GPIOHelper::pinIDs[LAST_UNUSEDPIN] = { HB_LED_Pin, TX_LED_Pin, RX_LED_Pin,
													AFE1_MENB_Pin, AFE2_MENB_Pin, AFE3_MENB_Pin, AFE4_MENB_Pin,
													ADC1_CS_Pin, ADC2_CS_Pin, ADC3_CS_Pin, ADC4_CS_Pin,
													DAC1_GAIN_Pin, DAC2_GAIN_Pin, DAC3_GAIN_Pin, DAC4_GAIN_Pin,
													USER_BUTTON_Pin
													};
const GPIO_TypeDef* GPIOHelper::pinPorts[LAST_UNUSEDPIN] = { HB_LED_GPIO_Port, TX_LED_GPIO_Port, RX_LED_GPIO_Port,
															AFE1_MENB_GPIO_Port, AFE2_MENB_GPIO_Port, AFE3_MENB_GPIO_Port, AFE4_MENB_GPIO_Port,
															ADC1_CS_GPIO_Port, ADC2_CS_GPIO_Port, ADC3_CS_GPIO_Port, ADC4_CS_GPIO_Port,
															DAC1_GAIN_GPIO_Port, DAC2_GAIN_GPIO_Port, DAC3_GAIN_GPIO_Port, DAC4_GAIN_GPIO_Port,
															USER_BUTTON_GPIO_Port
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

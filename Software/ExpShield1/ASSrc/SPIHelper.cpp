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

#include <GlobalHalHandlers.h>
#include <SPIHelper.h>
#include "main.h"
#include "stm32f0xx_hal.h"


// Singleton SPIHelper instance
const SPIHelper SPIHelper::instance;

SPIHelper::SPIHelper() {
}

SPIHelper::~SPIHelper() {
}


uint8_t SPIHelper::transfer(uint8_t data) const {

	uint8_t rxData;
	if (HAL_SPI_TransmitReceive(&hspi1, &data, &rxData, 1, 200) == HAL_OK) {
		return rxData;
	}

	return 0x00;
}

uint32_t SPIHelper::transfer(uint32_t data) const {

	uint32_t rxData = 0;

	if (HAL_SPI_TransmitReceive(&hspi1, (uint8_t*)&data, (uint8_t*)&rxData, 2, 200) == HAL_OK) {
		return rxData;
	}

	return 0x00;
}

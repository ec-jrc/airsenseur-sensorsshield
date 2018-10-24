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

#ifndef SPIHELPER_H_
#define SPIHELPER_H_

#include "stm32f0xx_hal.h"

class SPIHelper {
private:
	SPIHelper();

public:
	virtual ~SPIHelper();
	static inline const SPIHelper& getInstance() { return SPIHelper::instance; }

	uint8_t transfer(uint8_t data) const;
	uint32_t transfer(uint32_t data) const;

private:
	static const SPIHelper instance;
};

#define AS_SPI SPIHelper::getInstance()

#endif /* SPIHELPER_H_ */

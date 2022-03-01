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

#ifndef GPIOHELPER_H_
#define GPIOHELPER_H_

#include "Persistence.h"
#include "stm32f0xx_hal.h"

#define LOW	false
#define HIGH true

/* Read/Write GPIOs based on definitions in Persistence.h
 * thus creating a middle-layer between logic and bare hardware
 */
class GPIOHelper {
private:
	GPIOHelper();

public:
	virtual ~GPIOHelper();

	static inline const GPIOHelper& getInstance() { return GPIOHelper::instance; };

	void digitalWrite(uint8_t pin, bool value) const;
	uint8_t digitalRead(uint8_t pin) const;
	uint8_t getBoardId() const;

private:
	static const GPIOHelper instance;
	static const uint16_t pinIDs[LAST_UNUSEDPIN];
	static const GPIO_TypeDef* const pinPorts[LAST_UNUSEDPIN];
};

#define AS_GPIO GPIOHelper::getInstance()

#endif /* GPIOHELPER_H_ */

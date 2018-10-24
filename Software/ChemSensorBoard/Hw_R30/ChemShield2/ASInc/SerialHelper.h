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

#ifndef SERIALHELPER_H_
#define SERIALHELPER_H_

#include "stm32f0xx_hal.h"

class SerialHelper {
protected:
	SerialHelper();

public:
	virtual ~SerialHelper();

public:
	virtual void init() const = 0;
	virtual void onDataRx() = 0;
	virtual uint16_t write(char* buffer) const = 0;
	virtual bool available() const = 0;
	virtual uint8_t read() = 0;
	virtual void onErrorCallback() = 0;

public:
	void putByte(uint8_t data);
	bool dataReady() const;
	uint8_t getByte();

private:
	uint8_t *rxQueueBuffer;
	uint8_t *head;
	uint8_t *tail;
};

#endif /* SERIALHELPER_H_ */

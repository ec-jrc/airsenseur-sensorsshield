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

#ifndef SERIALUSBHELPER_H_
#define SERIALUSBHELPER_H_

#include "SerialHelper.h"

#define SERIALUSBBUFFERSIZE	64

class SerialUSBHelper : public SerialHelper {
public:
	SerialUSBHelper();

private:
	virtual ~SerialUSBHelper();

public:
	static inline SerialHelper* getInstance() { return &instance; }
	virtual void init() const;
	virtual void onDataRx(bool halfBuffer);
	virtual unsigned short write(char* buffer) const;
	virtual bool available() const;
	virtual uint8_t read();
	void onDataRx(unsigned char* buffer, long length);
	virtual void onErrorCallback();

private:
	static SerialUSBHelper instance;
	static uint8_t txBuffer[SERIALUSBBUFFERSIZE];
};

#define SerialUSB (*(SerialUSBHelper::getInstance()))

#endif /* SERIALUSBHELPER_H_ */

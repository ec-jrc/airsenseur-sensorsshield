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

#include <string.h>
#include "GlobalHalHandlers.h"
#include "SerialUSBHelper.h"

extern "C" {
	uint8_t CDC_Transmit_FS(uint8_t* Buf, uint16_t Len);
}

// Singleton SerialAHelper instance
SerialUSBHelper SerialUSBHelper::instance;
uint8_t SerialUSBHelper::txBuffer[SERIALUSBBUFFERSIZE];

SerialUSBHelper::SerialUSBHelper() {
}

SerialUSBHelper::~SerialUSBHelper() {
}

void SerialUSBHelper::init() const {
}

void SerialUSBHelper::onDataRx() {
}

void SerialUSBHelper::onDataRx(unsigned char* buffer, long length) {
	for (long n = 0; n < length; n++) {
		putByte(buffer[n]);
	}
}

unsigned short SerialUSBHelper::write(char* buffer) const {

	uint16_t len = strlen(buffer);
	len = (len > SERIALUSBBUFFERSIZE)? SERIALUSBBUFFERSIZE : len;
	memcpy(txBuffer, buffer, len);
	CDC_Transmit_FS((uint8_t*)txBuffer, len);

	return len;
}

bool SerialUSBHelper::available() const {
	return dataReady();
}

uint8_t SerialUSBHelper::read() {
	return getByte();
}

void SerialUSBHelper::onErrorCallback() {
}


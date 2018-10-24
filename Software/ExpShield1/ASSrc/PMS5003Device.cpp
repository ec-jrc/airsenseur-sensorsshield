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

#include "PMS5003Device.h"
#include "SerialCHelper.h"
#include "GPIOHelper.h"


#define PSM5003_START_FRAME	0x424d

const char* PMS5003Device::channelNames[] = {
		"5301CST", "5325CST", "5310CST", "5301CAT", "5325CAT", "5310CAT",
		"53PT003", "53PT005", "53PT010", "53PT025", "53PT050", "53PT100"
};

PMS5003Device::PMS5003Device() : SensorDevice(PSM5003_NUM_CHANNELS),
					curOffset(0), startCharacter(0), validFrame(false) {

	// Initialize the SerialC line
	SerialC.init();

	// Sleep mode
	AS_GPIO.digitalWrite(PNTW_SET, false);

	// Place in Reset
	AS_GPIO.digitalWrite(PNTW_RESET, false);
}

PMS5003Device::~PMS5003Device() {
}

void PMS5003Device::onStartSampling() {

	SensorDevice::onStartSampling();

	// Release from reset
	AS_GPIO.digitalWrite(PNTW_RESET, true);

	// Normal mode
	AS_GPIO.digitalWrite(PNTW_SET, true);
}

void PMS5003Device::onStopSampling() {

	// Sleep mode
	AS_GPIO.digitalWrite(PNTW_SET, false);

	// Place in Reset (to avoid draining current from this pin)
	AS_GPIO.digitalWrite(PNTW_RESET, false);
}

void PMS5003Device::setLowPowerMode(bool lowPower) {
}

void PMS5003Device::loop() {

    // Handle the serial line C
    if (SerialC.available()) {
    		unsigned char val = SerialC.read();
    		onDataReceived(val);
    }
}

void PMS5003Device::tick() {

}

const char* PMS5003Device::getChannelName(unsigned char channel) const {

	if (channel < sizeof (channelNames) / sizeof (const char *)) {
		return channelNames[channel];
	}

	return "";
}

void PMS5003Device::onDataReceived(unsigned char pivotChar) {

	if (!validFrame) {
		startCharacter = (startCharacter << 8) + pivotChar;
		if (startCharacter == PSM5003_START_FRAME) {
			validFrame = true;
			curOffset = 0;
		}
	} else {

		*(((unsigned char*)&dataStruct)+curOffset) = pivotChar;
		curOffset++;
		if (curOffset == sizeof(datastruct)) {
			evaluateFrame();
			validFrame = false;
		}
	}
}

bool PMS5003Device::checkCRC() {

	unsigned short sum = (PSM5003_START_FRAME & 0xFF) + ((PSM5003_START_FRAME>>8) & 0xFF);
	for (unsigned n = 0; n < sizeof(datastruct)-2; n++) {
		unsigned char* pVar = ((unsigned char*)&dataStruct)+n;
		sum += *pVar;
	}

	// Swap endianess
	sum = (sum>>8) + ((sum&0xFF)<<8);

	return sum == dataStruct.check;
}

void PMS5003Device::swapEndianess() {

	for (unsigned n = 0; n < (sizeof(datastruct)/2)-1; n++) {
		unsigned short* pVar = ((unsigned short*)&dataStruct)+n;
		*pVar = (*pVar>>8) + ((*pVar&0xFF)<<8);
	}
}

void PMS5003Device::evaluateFrame() {

	if (checkCRC()) {
		swapEndianess();

		setSample(0, dataStruct.pm1concSt);
		setSample(1, dataStruct.pm25concSt);
		setSample(2, dataStruct.pm10concSt);
		setSample(3, dataStruct.pm1concAt);
		setSample(4, dataStruct.pm25concAt);
		setSample(5, dataStruct.pm10concAt);
		setSample(6, dataStruct.part03);
		setSample(7, dataStruct.part05);
		setSample(8, dataStruct.part10);
		setSample(9, dataStruct.part25);
		setSample(10, dataStruct.part50);
		setSample(11, dataStruct.part100);
	}
}


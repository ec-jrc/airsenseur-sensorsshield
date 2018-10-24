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

#include "D300Device.h"
#include "GPIOHelper.h"
#include "I2CBHelper.h"

#define D300_I2C_ADDRESS		((0x31)<<1)
#define D300_CMD_DATAREQ		0x52

#define D300_DEFAULT_SAMPLERATE	2		/* Three seconds default sample rate */
#define D300_BLANK_PERIOD		1200		/* (one sampletick = 0.01s) */

const unsigned char D300Device::defaultSampleRate() {
	return D300_DEFAULT_SAMPLERATE;
}

D300Device::D300Device() : SensorDevice(1), go(false), blankTimer(0) {

	// Set in standby mode
	AS_GPIO.digitalWrite(D300_RESET, false);
}

D300Device::~D300Device() {

}

void D300Device::onStartSampling() {

	SensorDevice::onStartSampling();

	// Disable calibration pins
	AS_GPIO.digitalWrite(D300_CAL1, true);
	AS_GPIO.digitalWrite(D300_CAL2, true);

	// Release from standby mode
	AS_GPIO.digitalWrite(D300_RESET, true);

	// Discard all samples until the sensor is ready to run
	blankTimer = 0;
}

void D300Device::onStopSampling() {

	// Set in standby mode
	AS_GPIO.digitalWrite(D300_RESET, false);
}

void D300Device::setLowPowerMode(bool lowPower) {
}

void D300Device::loop() {

	if (go && (blankTimer >= D300_BLANK_PERIOD)) {
		go = false;

		unsigned char data[7];
		bool result = I2CB.read(D300_I2C_ADDRESS, D300_CMD_DATAREQ, 0x01, data, 0x07);
		if (result) {
			unsigned short sample = (data[1]<<8) + data[2];
			setSample(0, sample);
		}
	}
}

void D300Device::tick() {

	if (blankTimer < D300_BLANK_PERIOD) {
		blankTimer++;
	}
}

const char* D300Device::getChannelName(unsigned char channel) const {
	return "D300";
}


void D300Device::triggerSample() {
	SensorDevice::triggerSample();
	go = true;
}

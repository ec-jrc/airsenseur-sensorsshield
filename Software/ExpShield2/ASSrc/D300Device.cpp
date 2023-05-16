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
#define D300_BLANK_PERIOD		1200	/* (one sampletick = 0.01s) */

#define D300_START_CHECKING_AVAILABILITY	D300_BLANK_PERIOD + 1000
#define D300_END_CHECKING_AVAILABILITY		D300_START_CHECKING_AVAILABILITY + 100 /* One second after */

const unsigned char D300Device::defaultSampleRate() {
	return D300_DEFAULT_SAMPLERATE;
}

D300Device::D300Device() : SensorDevice(1), go(false), blankTimer(0), available(false) {

	// Set in standby mode
	AS_GPIO.digitalWrite(D300_RESET, false);

	// Disable calibration pins
	AS_GPIO.digitalWrite(D300_CAL1, true);
	AS_GPIO.digitalWrite(D300_CAL2, true);
}

D300Device::~D300Device() {

}

void D300Device::onStartSampling() {

	SensorDevice::onStartSampling();

	// Power on the device
	powerOn(true);
}

void D300Device::onStopSampling() {

	// Power off the device
	powerOn(false);
}

void D300Device::setLowPowerMode(bool lowPower) {
}

bool D300Device::init() {
	return true;
}

void D300Device::loop() {

	// Check for D300 availability, if required (mainly at startup)
	if (!available && (blankTimer >= D300_END_CHECKING_AVAILABILITY)) {
		blankTimer = 0;

		unsigned char data[7];
		available = I2CB.read(D300_I2C_ADDRESS, D300_CMD_DATAREQ, 0x01, data, 0x07);
	}

	if (go && available && (blankTimer >= D300_BLANK_PERIOD)) {
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

	if (available && (blankTimer < D300_BLANK_PERIOD)) {
		blankTimer++;
	}

	if (!available && (blankTimer >= D300_START_CHECKING_AVAILABILITY) && (blankTimer < D300_END_CHECKING_AVAILABILITY)) {
		blankTimer++;
	}
}

const char* D300Device::getSerial() const  {
	return NULL;
}

bool D300Device::setChannelName(unsigned char channel, const char* name) {
	return false;
}

const char* D300Device::getChannelName(unsigned char channel) const {
	return "D300";
}

const char* D300Device::getMeasurementUnit(unsigned char channel) const {
	return "ppm";
}

float D300Device::evaluateMeasurement(unsigned char channel, float value, bool firstSample) const {
	return value;
}

void D300Device::triggerSample() {
	SensorDevice::triggerSample();
	go = true;
}

void D300Device::powerOn(bool on) {

	// Turn On the device
	if (on) {

		// Disable calibration pins
		AS_GPIO.digitalWrite(D300_CAL1, true);
		AS_GPIO.digitalWrite(D300_CAL2, true);

		// Release from standby mode
		AS_GPIO.digitalWrite(D300_RESET, true);

		// Start checking for availability
		available = false;
		blankTimer = D300_START_CHECKING_AVAILABILITY;

		return;
	}

	// Turn Off the device otherwise
	AS_GPIO.digitalWrite(D300_RESET, false);
}

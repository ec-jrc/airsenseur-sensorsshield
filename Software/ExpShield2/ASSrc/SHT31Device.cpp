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
 *      European Commission - Joint Research Centre,
 * - Marco Signorini, marco.signorini@liberaintentio.com
 *
 * ===========================================================================
 */

#include <stddef.h>
#include "SHT31Device.h"
#include "I2CBHelper.h"
#include "IntChamberTempRef.h"
#include <GlobalHalHandlers.h>

#define SHT31ONBOARDADDRESS             (0x44<<1)
#define SHT32OFFBOARDADDRESS			(0x45<<1)
#define SHT31_CMD_START_HIGHREP         0x2400
#define SHT31_CMD_START_MEDREP          0x240B
#define SHT31_CMD_START_LOWREP          0x2416

#define SHT31_CMD_READ_STATUS           0xF32D
#define SHT31_CMD_CLEAR_STATUS          0x3041

#define SHT31_NUM_OF_CHANNELS			(SHT31_CHANNEL_HUMIDITY + 1)

#define TICKER_WAIT_FOR_SAMPLE			4

const char* const SHT31Device::channelNames[] {
		"SHT31TI", "SHT31HI", "SHT31TE", "SHT31HE"
};

const char* const SHT31Device::channelMeasurementUnits[] = {
		"C", "% RH"
};

SHT31Device::SHT31Device(bool internal) : SensorDevice(SHT31_NUM_OF_CHANNELS), ticker(0) {

	sensorAddress = internal? SHT31ONBOARDADDRESS: SHT32OFFBOARDADDRESS;

	status = UNAVAILABLE;
}

SHT31Device::~SHT31Device() {

}

void SHT31Device::onStartSampling() {
	SensorDevice::onStartSampling();

	status = UNAVAILABLE;
	if (checkPresence()) {
		status = IDLE_READY;
	}
}

void SHT31Device::onStopSampling() {

	if (status != UNAVAILABLE) {
		status = IDLE_STOP;
	}
}

void SHT31Device::setLowPowerMode(bool lowPower) {
}

bool SHT31Device::init() {

	// Check if the device is present on the I2C bus
	// then move it to STOP mode
	status = UNAVAILABLE;
	if (checkPresence()) {
		status = IDLE_STOP;
	}
	return true;
}

void SHT31Device::loop() {

	switch (status) {

		// Nothing to do
		case UNAVAILABLE:
		case WAIT_FOR_SAMPLE:
		case IDLE_READY:
		case IDLE_STOP:
			return;

		// Send a start sampling command
		case START_SAMPLING: {
			if (sendCommand(SHT31_CMD_START_LOWREP)) {
				status = WAIT_FOR_SAMPLE;
			}

		}
			break;

		case READ_SAMPLE: {
			unsigned short temperature, humidity;
			if (readData(&temperature, &humidity)) {
				setSample(SHT31_CHANNEL_TEMPERATURE, temperature);
				setSample(SHT31_CHANNEL_HUMIDITY, humidity);

				// Propagate the internal chamber temperature to the temperature
				// reference control helper
				if (sensorAddress == SHT31ONBOARDADDRESS) {
					double dTemp = evaluateMeasurement(SHT31_CHANNEL_TEMPERATURE, temperature, false) * 100;
					AS_INTCH_TEMPREF.setReadTemperature(IntChamberTempRef::SOURCE_TEMPERATURE_I, (short) dTemp);
				}
			}
			status = IDLE_READY;
		}
			break;
	}


}

void SHT31Device::tick() {

	if (status == WAIT_FOR_SAMPLE) {
		ticker++;
		if (ticker >= TICKER_WAIT_FOR_SAMPLE) {
			status = READ_SAMPLE;
		}
	}
}

const char* SHT31Device::getSerial() const {
	return "NA";
}

const char* SHT31Device::getChannelName(unsigned char channel) const {
	if (channel < SHT31_NUM_OF_CHANNELS) {
		return channelNames[channel + ((sensorAddress == SHT31ONBOARDADDRESS)? 0 : SHT31_NUM_OF_CHANNELS)];
	}

	return "";
}

bool SHT31Device::setChannelName(unsigned char channel, const char* name) {
	return false;
}

const char* SHT31Device::getMeasurementUnit(unsigned char channel) const {
	if (channel < SHT31_NUM_OF_CHANNELS) {
		return channelMeasurementUnits[channel];
	}

	return "";
}

float SHT31Device::evaluateMeasurement(unsigned char channel, float value, bool firstSample) const {

	if (firstSample) {
		return 0.0f;
	}

	if (channel == SHT31_CHANNEL_TEMPERATURE) {
		return ((((double)value)/65535)*175)-45.0;
	} else if (channel == SHT31_CHANNEL_HUMIDITY) {
		return  (((double)value)/65535)*100.0;
	}

	return 0.0f;
}

void SHT31Device::triggerSample() {
	SensorDevice::triggerSample();

	if (status == IDLE_READY) {
		ticker = 0;
		status = START_SAMPLING;
	}
}

char SHT31Device::sendCommand(unsigned short command) const {

	unsigned char buffer[] = { (unsigned char)(command >> 8), (unsigned char)(command & 0xFF) };
	bool result = I2CB.write(sensorAddress, buffer, 2);

  return (result)?1:0;
}

char SHT31Device::readData(unsigned short *temperature, unsigned short *humidity) const {

  unsigned char data[6];
  bool result = I2CB.read(sensorAddress, data, 0x06);

  // convert to unsigned short
  *temperature = ((unsigned short)data[0] << 8) | data[1];
  *humidity = ((unsigned short)data[3] << 8) | data[4];

  return (result)?1:0;
}

bool SHT31Device::checkPresence() {

	unsigned char checkNumber = 0;
	bool available = false;
	while ((checkNumber < 10) && !available) {
		available |= (sendCommand(SHT31_CMD_CLEAR_STATUS) != 0);
		checkNumber++;
	}

	return available;
}



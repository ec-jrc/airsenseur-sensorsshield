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
#include "SPS30Device.h"
#include "GPIOHelper.h"
#include "I2CAHelper.h"

#define SPS30_DEFAULT_SAMPLERATE	4		/* Five seconds. The sensor is able to generate
												a sample each second, but our prescaler is
												running in seconds units */
#define SPS30_BLANK_PERIOD			3000	/* (one sampletick = 0.01s) */
#define SPS30_MAX_CHECK_NUMBER		100		/* Max times to check if data are ready when sampling before discarding the sample */

#define SPS30_I2C_ADDRESS	(0x69 << 1)

#define SPS30_MEASUREMENTS_BUFFERSIZE_FLOAT	(SPS30_NUM_CHANNELS*6)
#define SPS30_MEASUREMENTS_BUFFERSIZE_INT	(SPS30_NUM_CHANNELS*3)

#define SPS30_START_MEASUREMENT		0x0010
#define SPS30_STOP_MEASUREMENT		0x0104
#define SPS30_READ_DATAREADY		0x0202
#define SPS30_READ_MEASUREMENT_VALS	0x0300
#define SPS30_READ_SERIALNUMBER		0xD033
#define SPS30_READ_FIRMWAREVERSION	0xD100

#define SPS30_DISCOVERY_RETRY		3
#define SPS30_READ_SERIALNUMBER_BUFFERSIZE	48
#define SPS30_READ_FIRMWAREVERSION_BUFFERSIZE	3

#define SPS30_PM_MULTIPLIER				128.0f		/* Max 512 ug/m3 with 1/128 resolution */
#define SPS30_BINS_MULTIPLIER			32.0f		/* Max 2048 count/ml with 1/32 resolution */
#define SPS30_TYPSIZE_MULTIPLIER		32768.0f	/* Max 1.9999 typical particle size with 1/32768 resolution */


#define SWAP_ENDIANESS(a) ((a)>>8) + (((a)&0xFF)<<8);

const char* const SPS30Device::channelNames[] = {
		"S30PM01", "S30PM25", "S30PM40", "S30PM10",
		"S30P005", "S30P010", "S30P025", "S30P040", "S30P100", "S30TSZE"
};

const char* const SPS30Device::channelMeasurementUnits[] = {
		"ug/m3", 		"ug/m3", 		"ug/m3", 		"ug/m3",
		"counts/ml", 	"counts/ml", 	"counts/ml", 	"counts/ml", 	"counts/ml", 	"um"
};

float const SPS30Device::multiplierFactors[] = {
		SPS30_PM_MULTIPLIER, SPS30_PM_MULTIPLIER, SPS30_PM_MULTIPLIER, SPS30_PM_MULTIPLIER,
		SPS30_BINS_MULTIPLIER, SPS30_BINS_MULTIPLIER, SPS30_BINS_MULTIPLIER, SPS30_BINS_MULTIPLIER, SPS30_BINS_MULTIPLIER,
		SPS30_TYPSIZE_MULTIPLIER
};

double const SPS30Device::evaluationFactors[] = {
		(1/SPS30_PM_MULTIPLIER), (1/SPS30_PM_MULTIPLIER), (1/SPS30_PM_MULTIPLIER), (1/SPS30_PM_MULTIPLIER),
		(1/SPS30_BINS_MULTIPLIER), (1/SPS30_BINS_MULTIPLIER), (1/SPS30_BINS_MULTIPLIER), (1/SPS30_BINS_MULTIPLIER), (1/SPS30_BINS_MULTIPLIER),
		(1/SPS30_TYPSIZE_MULTIPLIER)
};

const unsigned char SPS30Device::defaultSampleRate() {
	return SPS30_DEFAULT_SAMPLERATE;
}

SPS30Device::SPS30Device() : SensorDevice(SPS30_NUM_CHANNELS), go(false), blankTimer(0), deviceReady(false), maxCheckReady(0) {

	// Select I2C communications
	AS_GPIO.digitalWrite(SPARE1, false);

	memset(serialNumber, 0x00, SPS30_SERIAL_NUMBER_MAXLENGTH);
	strcpy(serialNumber, "NA");

	for (unsigned char retry = 0; retry < SPS30_DISCOVERY_RETRY; retry++) {
		char serial[32];
		if (readSerialNumber(serial)) {
			strncpy(serialNumber, serial+1, SPS30_SERIAL_NUMBER_MAXLENGTH);
			deviceReady = true;
			break;
		}
	}

	// Check for the firmware version (We don't support firmware older than 2.x revisions)
	if (deviceReady) {
		char fwVersion[12];
		deviceReady = readFirmwareVersion(fwVersion);
		if (deviceReady) {

			if (fwVersion[0] < 0x02) {
				deviceReady = false;
			}
		}
	}
}

SPS30Device::~SPS30Device() {
}


void SPS30Device::onStartSampling() {

	SensorDevice::onStartSampling();

	// Release from standby mode
	startMeasurement();

	// Discard all samples until the sensor is ready to run
	blankTimer = 0;
}

void SPS30Device::onStopSampling() {

	// Set in standby mode
	stopMeasurement();
}

void SPS30Device::setLowPowerMode(bool lowPower) {
}

void SPS30Device::loop() {

	if (deviceReady && go && (blankTimer >= SPS30_BLANK_PERIOD)) {

		// Check for sample availability
		if (!readDataReady()) {

			// No sample available. Check for a max of SPS30_MAX_CHECK_NUMBER times, then abandon.
			maxCheckReady++;
			if (maxCheckReady == SPS30_MAX_CHECK_NUMBER) {
				go = false;
			}

			return;
		}

		// Ok. Something to evaluate
		maxCheckReady = 0;
		go = false;

		unsigned short cmd = SWAP_ENDIANESS(SPS30_READ_MEASUREMENT_VALS);
		bool result = I2CA.write(SPS30_I2C_ADDRESS, (unsigned char*)&cmd, 0x02);
		if (result) {

#ifdef SPS30_USE_INTEGERS
			unsigned char data[SPS30_MEASUREMENTS_BUFFERSIZE_INT];
			bool result = I2CA.read(SPS30_I2C_ADDRESS, data, SPS30_MEASUREMENTS_BUFFERSIZE_INT);
			if (result) {
				unsigned short measurements[10];
				if (decodeMeasurements(data, measurements)) {
					for (unsigned char n = 0; n < SPS30_NUM_CHANNELS; n++) {
						unsigned short measurement = measurements[n];
						setSample(n, measurement);
					}
				}
			}
#else
			unsigned char data[SPS30_MEASUREMENTS_BUFFERSIZE_FLOAT];
			bool result = I2CA.read(SPS30_I2C_ADDRESS, data, SPS30_MEASUREMENTS_BUFFERSIZE_FLOAT);
			if (result) {
				float measurements[SPS30_NUM_CHANNELS];
				if (decodeMeasurements(data, measurements)) {
					for (unsigned char n = 0; n < SPS30_NUM_CHANNELS; n++) {
						float sample = measurements[n];
						sample = sample * multiplierFactors[n];
						setSample(n, (unsigned short)sample);
					}
				}
			}
#endif
		}
	}
}

void SPS30Device::tick() {

	if (blankTimer < SPS30_BLANK_PERIOD) {
		blankTimer++;
	}
}

const char* SPS30Device::getSerial() const  {
	return serialNumber;
}

bool SPS30Device::setChannelName(unsigned char channel, const char* name) {
	return false;
}

const char* SPS30Device::getChannelName(unsigned char channel) const {

	if (channel < SPS30_NUM_CHANNELS) {
		return channelNames[channel];
	}

	return "";
}

const char* SPS30Device::getMeasurementUnit(unsigned char channel) const {

	if (channel < SPS30_NUM_CHANNELS) {
		return channelMeasurementUnits[channel];
	}

	return "";
}

float SPS30Device::evaluateMeasurement(unsigned char channel, float value) const {

#ifdef SPS30_USE_INTEGERS
	return value;
#else

	if (channel < SPS30_NUM_CHANNELS) {
		return value* evaluationFactors[channel];
	}

	return 0.0f;
#endif
}

void SPS30Device::triggerSample() {
	SensorDevice::triggerSample();
	go = true;
}

bool SPS30Device::startMeasurement() const {

	unsigned char data[3];

#ifdef SPS30_USE_INTEGERS
	data[0] = 0x05; // Big-endian unsigned 16-bit integer values
#else
	data[0] = 0x03; // Big-endian IEEE754 float values
#endif

	data[1] = 0x00; // dummy byte, insert 0x00
	data[2] = crc(data);

	bool result = I2CA.write(SPS30_I2C_ADDRESS, SPS30_START_MEASUREMENT, 0x02, data, 0x03);

	return result;
}

bool SPS30Device::stopMeasurement() const {

	unsigned short data = SWAP_ENDIANESS(SPS30_STOP_MEASUREMENT);
	bool result = I2CA.write(SPS30_I2C_ADDRESS, (unsigned char*)&data, 0x02);

	return result;
}

bool SPS30Device::readDataReady() const {

	unsigned short cmd = SWAP_ENDIANESS(SPS30_READ_DATAREADY);
	bool result = I2CA.write(SPS30_I2C_ADDRESS, (unsigned char*)&cmd, 0x02);
	if (result) {

		union umsg {
			struct svalues {
				unsigned char unused;
				unsigned char dataReadyFlag;
				unsigned char crc;
			} value;
			unsigned char buffer[3];
		} message;

		result = I2CA.read(SPS30_I2C_ADDRESS, &message.buffer[0], sizeof(message));

		return result && (message.value.crc == crc(&message.buffer[0])) && (message.value.dataReadyFlag == 0x01);
	}

	return false;
}

bool SPS30Device::decodeMeasurements(unsigned char* data, unsigned short* measurements) {

	for (unsigned char n = 0; n < SPS30_MEASUREMENTS_BUFFERSIZE_INT; n+=3) {

		unsigned char crc1 = data[n+2];

		// Check for CRC
		if (crc1 != crc(data+n)) {
			return false;
		}

		*measurements = (((unsigned short)data[n]) << 8) |  ((unsigned short)data[n+1]);
		measurements++;
	}

	return true;
}

bool SPS30Device::decodeMeasurements(unsigned char* data, float* measurements) {

	unsigned char m = 0;
	for (unsigned char n = 0; n < SPS30_MEASUREMENTS_BUFFERSIZE_FLOAT; n+=6) {

		unsigned char crc1 = data[n+2];
		unsigned char crc2 = data[n+5];

		// Check for CRC
		if ((crc1 != crc(data+n)) || (crc2 != crc(data+n+3))) {
			return false;
		}

	    union {
	        unsigned long valuelong;
	        float valuefloat;
	    } measurement;

	    measurement.valuelong = ((unsigned long)data[n] << 24) | ((unsigned long)data[n+1] << 16) | ((unsigned long)data[n+3] << 8) | ((unsigned long)data[n+4]);
		measurements[m] = measurement.valuefloat;
		m++;
	}

	return true;
}

bool SPS30Device::readSerialNumber(char* serialNumber) const {

	unsigned short cmd = SWAP_ENDIANESS(SPS30_READ_SERIALNUMBER);
	bool result = I2CA.write(SPS30_I2C_ADDRESS, (unsigned char*)&cmd, 0x02);
	if (result) {
		char data[SPS30_READ_SERIALNUMBER_BUFFERSIZE];
		result = I2CA.read(SPS30_I2C_ADDRESS, (unsigned char*)data, SPS30_READ_SERIALNUMBER_BUFFERSIZE);

		if (result) {
			for (unsigned char n = 0; n < SPS30_READ_SERIALNUMBER_BUFFERSIZE; n+=3) {

				// Check for CRC
				if (data[n+2] != crc((unsigned char*)data + n)) {
					return false;
				}

				// Copy the received valid characters
				*serialNumber = data[n];
				serialNumber++;
				*serialNumber = data[n+1];
				serialNumber++;
			}
		}
	}

	return result;
}

bool SPS30Device::readFirmwareVersion(char* firmwareVersion) const {

	unsigned short cmd = SWAP_ENDIANESS(SPS30_READ_FIRMWAREVERSION);
	bool result = I2CA.write(SPS30_I2C_ADDRESS, (unsigned char*)&cmd, 0x02);
	if (result) {
		char data[SPS30_READ_FIRMWAREVERSION_BUFFERSIZE];
		result = I2CA.read(SPS30_I2C_ADDRESS, (unsigned char*)data, SPS30_READ_FIRMWAREVERSION_BUFFERSIZE);
		if (result) {

			// Check for CRC
			if (data[2] != crc((unsigned char*)data)) {
				return false;
			}

			// Copy the received valid characters
			firmwareVersion[0] = data[0];
			firmwareVersion[1] = data[1];
			firmwareVersion[2] = 0x00;
		}
	}

	return result;
}

unsigned char SPS30Device::crc(unsigned char data[2]) const {

	unsigned char crc = 0xFF;
	for(int i = 0; i < 2; i++) {
		crc ^= data[i];
		for(unsigned char bit = 8; bit > 0; --bit) {
			if(crc & 0x80) {
				crc = (crc << 1) ^ 0x31u;
			} else {
				crc = (crc << 1);
			}
		}
	}

	return crc;
}

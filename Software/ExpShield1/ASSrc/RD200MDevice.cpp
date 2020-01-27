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

#include "RD200MDevice.h"
#include "SerialDHelper.h"

#define DEFAULT_SAMPLING_PERIOD_TIME 10	/* Then minutes */

/* Command List */
#define CMD_RD200M_RESULT_QUERY  	0x01
#define CMD_RD200M_RESET 			0xA0
#define CMD_RD200M_SEND_PERIOD_SET	0xA1
#define CMD_RD200M_RESULT_RETURN		0x10

#define STX_CHAR						0x02
#define DEFAULT_RESULT_LEN			0x04

/* Statuses */
#define RD200_STATUS_POWERON			0x00
#define RD200_STATUS_1HWAITING		0x01
#define RD200_STATUS_NORMAL			0x02
#define RD200_STATUS_1HWAITWARNING	0x10
#define RD200_STATUS_VIBRATIONS		0xE0

#define RD200_SAMPLE_VAL_IN_WARNING	0xFFFF

#define RD200_DEFAULT_SAMPLERATE			24		// 25 seconds each sample
#define RD200_DEFAULT_DECIMATIONVALUE	39		// 40 samples -> 10 minutes each result

const unsigned char RD200MDevice::defaultSampleRate() {
	return RD200_DEFAULT_SAMPLERATE;
}

const unsigned char RD200MDevice::defaultDecimationValue() {
	return RD200_DEFAULT_DECIMATIONVALUE;
}


RD200MDevice::RD200MDevice() : SensorDevice(1), curStatus(RX_IDLE), curRxOffset(0), needsReset(true) {

	// Initialize the SerialD line
	SerialD.init();
}

RD200MDevice::~RD200MDevice() {
}

unsigned char RD200MDevice::checkSum() {
	unsigned char* data = (unsigned char*)&dataStruct;
	unsigned char sum = 0;
	for (unsigned char i = 0; i < sizeof(datastruct); i++) {
		sum += data[i];
	}

	return 0xFF-sum;
}

void RD200MDevice::onStartSampling() {

	SensorDevice::onStartSampling();

	// Set period
	char buffer[5] = {0x02, CMD_RD200M_SEND_PERIOD_SET, 0x01, DEFAULT_SAMPLING_PERIOD_TIME, (char) (0xFF - (CMD_RD200M_SEND_PERIOD_SET + 0x01 + DEFAULT_SAMPLING_PERIOD_TIME))};
	SerialD.write(buffer, 5);
}

void RD200MDevice::onStopSampling() {
}

void RD200MDevice::setLowPowerMode(bool lowPower) {
}

void RD200MDevice::loop() {

    // Handle the serial line D
    if (SerialD.available()) {
    		unsigned char val = SerialD.read();
    		onDataReceived(val);
    }
}


void RD200MDevice::tick() {
}

const char* RD200MDevice::getSerial() const {
	return NULL;
}

bool RD200MDevice::setChannelName(unsigned char channel, const char* name) {
	return false;
}

const char* RD200MDevice::getChannelName(unsigned char channel) const {
	return "RD200M";
}

const char* RD200MDevice::getMeasurementUnit(unsigned char channel) const {
	return "pCi/L";
}

float RD200MDevice::evaluateMeasurement(unsigned char channel, float value) const {
	return value/100.0;
}

void RD200MDevice::triggerSample() {
	SensorDevice::triggerSample();

	if (!evaluateReset()) {
		requestData(CMD_RD200M_RESULT_QUERY);
	}
}

void RD200MDevice::requestData(unsigned char command) {

	char buffer[4] = {0x02, (char)command, 0x00, (char)(0xFF - command)};
	SerialD.write(buffer, 4);
}

void RD200MDevice::onDataReceived(unsigned char pivotChar) {

	switch (curStatus) {

		case RX_IDLE: {
			if (pivotChar == STX_CHAR) {
				curStatus = RX_STARTFOUND;
			}
		}
		break;

		case RX_STARTFOUND: {
			if (pivotChar == CMD_RD200M_RESULT_RETURN) {
				dataStruct.command = pivotChar;
				curStatus = RX_COMMANDFOUND;
			} else  {
				curStatus = RX_IDLE;
			}
		}
		break;

		case RX_COMMANDFOUND: {
			if (pivotChar == DEFAULT_RESULT_LEN) {
				dataStruct.dataSize = pivotChar;
				curStatus = RX_WAITING_PARAMETERS;
				curRxOffset = 0;
			} else  {
				curStatus = RX_IDLE;
			}
		}
		break;

		case RX_WAITING_PARAMETERS: {
			dataStruct.data.buffer[curRxOffset] = pivotChar;
			curRxOffset++;
			if (curRxOffset == dataStruct.dataSize) {
				curStatus = RX_WAITING_CRC;
			}
		}
		break;

		case RX_WAITING_CRC: {
			unsigned char calcChecksum = checkSum();
			if (calcChecksum == pivotChar) {
				evaluateRxBuffer();
			}
			curStatus = RX_IDLE;
		}
		break;

		default:
			curStatus = RX_IDLE;
			break;
	}
}

void RD200MDevice::evaluateRxBuffer() {

	if (dataStruct.data.decoded.status == RD200_STATUS_NORMAL) {

		unsigned short sample = (dataStruct.data.decoded.unit * 100) + dataStruct.data.decoded.cents;
		setSample(0, sample);

	} else if (dataStruct.data.decoded.status == RD200_STATUS_1HWAITWARNING) {
		setSample(0, RD200_SAMPLE_VAL_IN_WARNING);

	} else if (dataStruct.data.decoded.status == RD200_STATUS_VIBRATIONS) {

		// Issue a RESET (to be verified)
		needsReset = true;
	}
}

bool RD200MDevice::evaluateReset() {
	if (needsReset) {
		needsReset = false;

		requestData(CMD_RD200M_RESET);

		return true;
	}

	return false;
}

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


#include "NextPMDevice.h"
#include "SerialAHelper.h"

#define NEXTPM_DEFAULT_ADDRESS	0x81

#define READ_CONV_AVG10_1S		0x11
#define READ_CONC_AVG60_10S		0x12
#define READ_CONC_AVG900_60S	0x13
#define READ_TEMP_HUMIDITY		0x14
#define REQ_POWER_ON_SLEEP		0x15
#define READ_SENSOR_STATE 		0x16

#define STATE_BIT_LASER_ERROR	0x07
#define STATE_BIT_MEMORY_ERROR	0x06
#define STATE_BIT_FAN_ERROR		0x05
#define STATE_BIT_TRH_ERROR		0x04
#define STATE_BIT_HEAT_ERROR	0x03
#define STATE_BIT_NOT_READY		0x02
#define STATE_BIT_DEGRADED		0x01
#define STATE_BIT_SLEEP			0x00

// Macro for calculate CRC at compile time when sending commands
#define NEXTPM_CRC_SEED			0x100
#define CALC_CRC_FOR_CMD(cmd) (((NEXTPM_CRC_SEED - NEXTPM_DEFAULT_ADDRESS - (cmd))) & 0xFF)
#define CALC_CRC_FOR_CMD_WITH_PAR(cmd,par) (((NEXTPM_CRC_SEED - NEXTPM_DEFAULT_ADDRESS - (cmd) - (par))) & 0xFF)

#define SWAP_ENDIANESS(a) ((a)>>8) + (((a)&0xFF)<<8)

#define NEXTPM_DEFAULT_SAMPLERATE			0		// 1 seconds each sample
#define NEXTPM_DEFAULT_DECIMATIONVALUE		0		// 1 seconds each sample

const char* const NextPMDevice::channelNames[] = {
		"NPMPC01", "NPMPC25", "NPMPC10",
		"NPMPM01", "NPMPM25", "NPMPM10",
		"NPMTemp", "NPMHum", "NPMSta"
};

const char* const NextPMDevice::channelMeasurementUnits[] = {
		"pcs/L", 		"pcs/L", 		"pcs/L",
		"ug/m3", 		"ug/m3", 		"ug/m3",
		"C", 			"%", 			"bit",
};

double const NextPMDevice::evaluationFactors[] = {
		1.0, 	1.0, 	1.0,
		0.1,	0.1,	0.1,
		0.01, 	0.01
};


const unsigned char NextPMDevice::defaultSampleRate() {
	return NEXTPM_DEFAULT_SAMPLERATE;
}

const unsigned char NextPMDevice::defaultDecimationValue() {
	return NEXTPM_DEFAULT_DECIMATIONVALUE;
}


NextPMDevice::NextPMDevice() : SensorDevice(NEXTPM_NUM_CHANNELS),
		deviceFound(false), samplingOn(false), go(false), curStatus(RX_IDLE), expectedDataSize(0), curRxOffset(0) {

	// Initialize the serial communication peripheral
	SerialA.init();

	// Entering in sleep mode by triggering a read status command
	char data[3] = { NEXTPM_DEFAULT_ADDRESS, READ_SENSOR_STATE, CALC_CRC_FOR_CMD(READ_SENSOR_STATE) };
	SerialA.write(data, 3);
}

NextPMDevice::~NextPMDevice() {
}

void NextPMDevice::onStartSampling(void) {
	SensorDevice::onStartSampling();
	samplingOn = true;

	// Exit from sleep by triggering a read status command
	char data[3] = { NEXTPM_DEFAULT_ADDRESS, READ_SENSOR_STATE, CALC_CRC_FOR_CMD(READ_SENSOR_STATE) };
	SerialA.write(data, 3);
}

void NextPMDevice::onStopSampling(void) {
	samplingOn = false;

	// Entering in sleep mode by triggering a read status command
	char data[3] = { NEXTPM_DEFAULT_ADDRESS, READ_SENSOR_STATE, CALC_CRC_FOR_CMD(READ_SENSOR_STATE) };
	SerialA.write(data, 3);
}

void NextPMDevice::setLowPowerMode(bool lowPower) {
}

void NextPMDevice::loop() {

	// Start triggering samples if required
	// (writing to serial is allowed only in the loop and
	// called functions. This avoids overlap of requests through the serial line)
	if (go) {
		go = false;

		// Ask for PM. Temperature and humidity will follow
		// as soon as the answer of this command has been received
		requestData(READ_CONV_AVG10_1S);
	}

    // Handle the serial line A
    if (SerialA.available()) {
    		unsigned char val = SerialA.read();
    		onDataReceived(val);
    }
}

void NextPMDevice::tick() {
}

const char* NextPMDevice::getSerial() const {
	return NULL;
}

bool NextPMDevice::setChannelName(unsigned char channel, const char* name) {
	return false;
}


const char* NextPMDevice::getChannelName(unsigned char channel) const {
	if (channel < NEXTPM_NUM_CHANNELS) {
		return channelNames[channel];
	}

	return "";
}

const char* NextPMDevice::getMeasurementUnit(unsigned char channel) const {
	if (channel < NEXTPM_NUM_CHANNELS) {
		return channelMeasurementUnits[channel];
	}

	return "";
}

float NextPMDevice::evaluateMeasurement(unsigned char channel, float value) const {
	if (channel < NEXTPM_NUM_CHANNELS) {
		return value* evaluationFactors[channel];
	}

	return 0;
}

void NextPMDevice::triggerSample() {
	SensorDevice::triggerSample();

	if (deviceFound && samplingOn) {
		go = true;
	}
}

void NextPMDevice::requestData(unsigned char command) {

	char buffer[3] = { NEXTPM_DEFAULT_ADDRESS, (char)command, CALC_CRC_FOR_CMD(command) };
	SerialA.write(buffer, 3);
}


void NextPMDevice::onDataReceived(unsigned char pivotChar) {

	switch (curStatus) {

		case RX_IDLE: {
			if (pivotChar == NEXTPM_DEFAULT_ADDRESS) {
				curStatus = RX_STARTFOUND;
			}
		}
		break;

		case RX_STARTFOUND: {
			if ((pivotChar >= READ_CONV_AVG10_1S) &&
					(pivotChar <= READ_SENSOR_STATE)) {

			if ((pivotChar == READ_CONV_AVG10_1S) ||
				(pivotChar == READ_CONC_AVG60_10S) ||
				(pivotChar == READ_CONC_AVG900_60S) ) {
					expectedDataSize = NEXTPM_DATABUFFERSZ;
				} else if (pivotChar == READ_TEMP_HUMIDITY) {
					expectedDataSize = NEXTPM_DATABUFFERSZ_TEMPHUMID;
				} else {
					expectedDataSize = 0;
				}

			dataStruct.command = pivotChar;
			curStatus = RX_COMMANDFOUND;

			} else  {
				curStatus = RX_IDLE;
			}
		}
		break;

		case RX_COMMANDFOUND: {

			dataStruct.state = pivotChar;
			curStatus = (expectedDataSize == 0)? RX_WAITING_CRC : RX_WAITING_PARAMETERS;
			curRxOffset = 0;
		}
		break;

		case RX_WAITING_PARAMETERS: {
			dataStruct.data.buffer[curRxOffset] = pivotChar;
			curRxOffset++;
			if (curRxOffset == expectedDataSize) {
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

unsigned char NextPMDevice::checkSum() {

	unsigned char crc = (NEXTPM_CRC_SEED - NEXTPM_DEFAULT_ADDRESS);

	crc -= dataStruct.command;
	crc -= dataStruct.state;
	for (unsigned char n = 0; n < expectedDataSize; n++) {
		crc -= dataStruct.data.buffer[n];
	}

	return crc;
}

void NextPMDevice::evaluateRxBuffer() {

	switch (dataStruct.command) {

		case READ_CONV_AVG10_1S:
		case READ_CONC_AVG60_10S:
		case READ_CONC_AVG900_60S: {

			if (evaluateState(dataStruct.state)) {

				setSample(NEXTPM_PM1PCS, SWAP_ENDIANESS(dataStruct.data.decodedpm.pm1pcs));
				setSample(NEXTPM_PM25PCS, SWAP_ENDIANESS(dataStruct.data.decodedpm.pm25pcs));
				setSample(NEXTPM_PM10PCS, SWAP_ENDIANESS(dataStruct.data.decodedpm.pm10pcs));
				setSample(NEXTPM_PM1CONC, SWAP_ENDIANESS(dataStruct.data.decodedpm.pm1ug));
				setSample(NEXTPM_PM25CONC, SWAP_ENDIANESS(dataStruct.data.decodedpm.pm25ug));
				setSample(NEXTPM_PM10CONC, SWAP_ENDIANESS(dataStruct.data.decodedpm.pm10ug));
				setSample(NEXTPM_STATUS, dataStruct.state);

				// It's time to ask for temperature and humidity
				requestData(READ_TEMP_HUMIDITY);
			}
			break;
		}

		case READ_TEMP_HUMIDITY: {

			setSample(NEXTPM_TEMPERATURE, SWAP_ENDIANESS(dataStruct.data.decodedth.temperature));
			setSample(NEXTPM_HUMIDITY, SWAP_ENDIANESS(dataStruct.data.decodedth.humidity));
		}
		break;

		case READ_SENSOR_STATE: {
			evaluateState(dataStruct.state);
		}

		default:
			break;
	}

}


// Return true if the data read is valid
// Send the PowerOn or Sleep mode command when required (i.e. start/stop sampling)
bool NextPMDevice::evaluateState(unsigned char state) {

	// The bit 0 is set to 1 when the sensor is set to sleep state
	if ( ((samplingOn) && (state & (1<<STATE_BIT_SLEEP))) ||
		 ((!samplingOn) &&  !(state & (1<<STATE_BIT_SLEEP)) )) {
		requestData(REQ_POWER_ON_SLEEP);
	}

	// Flag we found a device communicating with the proper protocol
	deviceFound = true;

	return true;
}


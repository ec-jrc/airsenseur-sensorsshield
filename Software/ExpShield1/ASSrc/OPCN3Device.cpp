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

#include "OPCN3Device.h"
#include "GPIOHelper.h"
#include "SPIHelper.h"

#define OPCN3_WAIT_INFOSTRING_TIME		29	/* 30ms */
#define OPCN3_WAIT_SERIALSTRING_TIME	29 /* 30ms */
#define OPCN3_WAIT_FAN_TIME				249	/* 2.5 seconds */
#define OPCN3_WAIT_LASER_TIME			149 /* 1.5 seconds */
#define OPC_BINS_MULTIPLIER				32.0f	/* Max 2048 count/ml with 1/32 resolution */
#define OPC_PM_MULTIPLIER				32.0f	/* Max 2048 ug/m3 with 1/32 resolution */
#define OPC_VOLUME_MULTIPLIER			64.0f	/* Max 1024 ml with 1/64 resolution */
#define OPC_TEMP_MULTIPLIER				1000.0f
#define OPC_HUM_MULTIPLIER				1000.0f

#define LASER_CURRENT_THRESHOLD	300	/* We experienced a working laser is associated to the surrounding of 600 */

const char* const OPCN3Device::channelNames[] = {
    "OPCN3Bin0", "OPCN3Bin1", "OPCN3Bin2", "OPCN3Bin3", "OPCN3Bin4", "OPCN3Bin5",
	"OPCN3Bin6", "OPCN3Bin7", "OPCN3Bin8", "OPCN3Bin9", "OPCN3Bin10", "OPCN3Bin11",
	"OPCN3Bin12", "OPCN3Bin13", "OPCN3Bin14", "OPCN3Bin15", "OPCN3Bin16", "OPCN3Bin17",
	"OPCN3Bin18", "OPCN3Bin19", "OPCN3Bin20", "OPCN3Bin21", "OPCN3Bin22", "OPCN3Bin23",
	"OPCN3PM1", "OPCN3PM25", "OPCN3PM10",
	"OPCN3Temp", "OPCN3Hum", "OPCN3Vol",
	"OPCN3TSam", "OPCN3FRt", "OPCN3Lsr"
};

const char* const OPCN3Device::channelMeasurementUnits[] = {
	"counts/ml", "counts/ml", "counts/ml", "counts/ml", "counts/ml", "counts/ml",
	"counts/ml", "counts/ml", "counts/ml", "counts/ml", "counts/ml", "counts/ml",
	"counts/ml", "counts/ml", "counts/ml", "counts/ml", "counts/ml", "counts/ml",
	"counts/ml", "counts/ml", "counts/ml", "counts/ml", "counts/ml", "counts/ml",
	"ug/m3", "ug/m3", "ug/m3",
	"C", "% RH", "ml",
	"ms", "ml/s", "ticks",
};

double const OPCN3Device::evaluationFactors[] = {
	(1/OPC_BINS_MULTIPLIER), (1/OPC_BINS_MULTIPLIER), (1/OPC_BINS_MULTIPLIER), (1/OPC_BINS_MULTIPLIER), (1/OPC_BINS_MULTIPLIER), (1/OPC_BINS_MULTIPLIER),
	(1/OPC_BINS_MULTIPLIER), (1/OPC_BINS_MULTIPLIER), (1/OPC_BINS_MULTIPLIER), (1/OPC_BINS_MULTIPLIER), (1/OPC_BINS_MULTIPLIER), (1/OPC_BINS_MULTIPLIER),
	(1/OPC_BINS_MULTIPLIER), (1/OPC_BINS_MULTIPLIER), (1/OPC_BINS_MULTIPLIER), (1/OPC_BINS_MULTIPLIER), (1/OPC_BINS_MULTIPLIER), (1/OPC_BINS_MULTIPLIER),
	(1/OPC_BINS_MULTIPLIER), (1/OPC_BINS_MULTIPLIER), (1/OPC_BINS_MULTIPLIER), (1/OPC_BINS_MULTIPLIER), (1/OPC_BINS_MULTIPLIER), (1/OPC_BINS_MULTIPLIER),
	(1/OPC_PM_MULTIPLIER), (1/OPC_PM_MULTIPLIER), (1/OPC_PM_MULTIPLIER),
	(1/(1000.0f)), (1/(1000.0f)), (1/OPC_VOLUME_MULTIPLIER),
	1.0f, 1.0f, 1.0f
};

OPCN3Device::OPCN3Device() : SensorDevice(OPCN3_CHAN_NUMBER),
				lowPowerMode(false), samplingEnabled(false), curStatus(REQ_INFOSTRING), isOPCN2Unit(false), timer(0) {
	memset(serialNumber, 0x00, OPCN3_SERIAL_NUMBER_MAXLENGTH);
	strcpy(serialNumber, "NA");
 }

OPCN3Device::~OPCN3Device() {
}

void OPCN3Device::onStartSampling() {
	SensorDevice::onStartSampling();

	if (!lowPowerMode) {
		curStatus = SET_LASER_ON;
	}
	samplingEnabled = true;
}

void OPCN3Device::onStopSampling() {

	curStatus = SET_LASER_OFF;
	samplingEnabled = false;
}

void OPCN3Device::setLowPowerMode(bool lowPower) {
	lowPowerMode = lowPower;
}

void OPCN3Device::loop() {

	opcComm.loop();

	switch (curStatus) {

		case REQ_INFOSTRING: {
			if (opcComm.ready()) {
				if (opcComm.triggerCommand(OPCN3Comm::cmdOffset::READ_INFOSTRING)) {
					curStatus = WAIT_INFOSTRING;
					timer = 0;
				}
			}
		}
		break;

		case WAIT_INFOSTRING: {
			if (timer > OPCN3_WAIT_INFOSTRING_TIME) {

				OPCN3Comm::infostring* infostring;
				infostring = opcComm.getInfoString();
				if (infostring != NULL) {
					evaluateInfoString(infostring);
					curStatus = REQ_SERIALSTRING;
				}
			}
		}
		break;

		case REQ_SERIALSTRING: {
			if (opcComm.ready()) {
				if (opcComm.triggerCommand(OPCN3Comm::cmdOffset::READ_SERIAL)) {
					curStatus = WAIT_SERIALSTRING;
					timer = 0;
				}
			}
		}
		break;

		case WAIT_SERIALSTRING: {
			if (timer > OPCN3_WAIT_SERIALSTRING_TIME) {
				OPCN3Comm::serialstring* serialstring;
				serialstring = opcComm.getSerialString();
				if (serialstring != NULL) {
					evaluateSerialString(serialstring);
					curStatus = IDLE;
				}
			}
		}
		break;

		case SET_LASER_ON: {
			if (opcComm.ready()) {
				if (opcComm.triggerCommand(OPCN3Comm::cmdOffset::SET_LASER_SWITCH_ON)) {
					curStatus = WAIT_LASER_ON;
					timer = 0;
				}
			}
		}
		break;

		case WAIT_LASER_ON: {
			if (timer > OPCN3_WAIT_LASER_TIME) {
				curStatus = SET_FAN_ON;
			}
		}
		break;

		case SET_FAN_ON: {
			if (opcComm.ready()) {
				if (opcComm.triggerCommand(OPCN3Comm::cmdOffset::SET_FAN_DIGPOT_ON)) {
					curStatus = WAIT_FAN_ON;
					timer = 0;
				}
			}
		}
		break;

		case WAIT_FAN_ON: {
			if (timer > OPCN3_WAIT_FAN_TIME) {
				curStatus = SAMPLING_READY;
				timer = 0;
			}
		}
		break;

		case SAMPLING_START: {
			if (opcComm.ready()) {
				if (opcComm.triggerCommand(OPCN3Comm::cmdOffset::READ_HISTOGRAM)) {
					curStatus = SAMPLING_WAITING;
				}
			}
		}
		break;

		case SAMPLING_WAITING: {
			OPCN3Comm::histogram* histogram;
			histogram = opcComm.getLastHistogram();
			if (histogram) {

				bool laserFail = evaluateHistogram(histogram);

				// Seems the laser is still turned off. Try to restart-it again
				// before the next sampling period
				if (laserFail) {
					curStatus = SET_LASER_ON;
				} else {
					curStatus = (lowPowerMode)? SET_LASER_OFF : SAMPLING_READY;
				}
			}
		}
		break;

		case SET_LASER_OFF: {
			if (opcComm.ready()) {
				if (opcComm.triggerCommand(OPCN3Comm::cmdOffset::SET_LASER_SWITCH_OFF)) {
					curStatus = SET_FAN_OFF;
				}
			}
		}
		break;

		case SET_FAN_OFF: {
			if (opcComm.ready()) {
				if (opcComm.triggerCommand(OPCN3Comm::cmdOffset::SET_FAN_DIGPOT_OFF)) {
					curStatus = IDLE;
				}
			}
		}
		break;

		case IDLE:
		case SAMPLING_READY:
		default:
			break;
	}
}

const char* OPCN3Device::getSerial() const {

	return serialNumber;
}

bool OPCN3Device::evaluateInfoString(OPCN3Comm::infostring* infostring) {

	if (infostring == NULL) {
		return false;
	}

	if (infostring->infoString[5] == '2') {
		isOPCN2Unit = true;
	}

	return true;
}

bool OPCN3Device::evaluateSerialString(OPCN3Comm::serialstring* serialstring) {

	if (serialstring == NULL) {
		return false;
	}

	strncpy(serialNumber, serialstring->serialString+7, OPCN3_SERIAL_NUMBER_MAXLENGTH);
	serialNumber[OPCN3_SERIAL_NUMBER_MAXLENGTH-1] = 0x00;

	// We don't want to have spaces after the numbers. Replace the 1st found space with string termination
	for (unsigned char n = 0; n < strlen(serialNumber); n++) {
		if (serialNumber[n] == ' ') {
			serialNumber[n] = 0x00;
			break;
		}
	}

	return true;
}

bool OPCN3Device::evaluateHistogram(OPCN3Comm::histogram* histogram) {

	// Evaluate histogram
	float airVolume = ((float)(histogram->samplingFlowRate * histogram->samplingPeriod)) / 10000.0f; /* Volume in ml */
	for (unsigned char channel = 0; channel < OPCN3_BINS_NUMBER; channel++) {
		float fParticles = ((float)(histogram->bins[channel])) / airVolume;	/* Counts / ml */
		fParticles *= OPC_BINS_MULTIPLIER;
		unsigned short particles = (unsigned short) fParticles;
		setSample(channel, particles);
	}

	for (unsigned char channel = 0; channel < OPCN3_PMS_NUMBER; channel++) {
		unsigned short pmVal = (unsigned short)(opcComm.toPmValue(histogram->pmVal + channel) * OPC_PM_MULTIPLIER);
		setSample(channel+OPCN3_BINS_NUMBER, pmVal);
	}

	double evalTemp = (((((double)histogram->temperature)/65535)*175)-45.0)*OPC_TEMP_MULTIPLIER;
	setSample(OPCN3_TEMP, (unsigned short)evalTemp);
	double evalHum = (((double)histogram->relHumidity)/65535)*100.0*OPC_HUM_MULTIPLIER;
	setSample(OPCN3_HUM, (unsigned short)evalHum);
	setSample(OPCN3_VOL, (unsigned short) (airVolume * OPC_VOLUME_MULTIPLIER));

	/* These are reported for debug purposes only */
	setSample(OPCN3_TSA, histogram->samplingPeriod);
	setSample(OPCN3_FRT, histogram->samplingFlowRate);
	setSample(OPCN3_LSRST, histogram->laserStatus);

	return (histogram->laserStatus < LASER_CURRENT_THRESHOLD);
}

void OPCN3Device::tick() {

	opcComm.tick();
	timer++;
}

bool OPCN3Device::setChannelName(unsigned char channel, const char* name) {
	return false;
}

const char* OPCN3Device::getChannelName(unsigned char channel) const {

	if (channel < OPCN3_CHAN_NUMBER) {
		return channelNames[channel];
	}

	return "";
}

const char* OPCN3Device::getMeasurementUnit(unsigned char channel) const {

	if (channel < OPCN3_CHAN_NUMBER) {
		return channelMeasurementUnits[channel];
	}

	return "";
}

float OPCN3Device::evaluateMeasurement(unsigned char channel, float value) const {

	if (channel < OPCN3_CHAN_NUMBER) {
		return value* evaluationFactors[channel];
	}

	return 0.0f;
}

void OPCN3Device::triggerSample() {
	SensorDevice::triggerSample();

	// Skip if sampling not enabled
	if (!samplingEnabled) {
		return;
	}

	// Skip if not still ready to sample
	if ((curStatus == IDLE) || (curStatus == SAMPLING_READY)) {
		curStatus = (lowPowerMode)? SET_LASER_ON : SAMPLING_START;
	}
}

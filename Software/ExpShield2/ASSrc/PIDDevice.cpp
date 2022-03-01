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

#include "PIDDevice.h"
#include "TControlEngine.h"
#include "CRC32Helper.h"
#include "EEPROMHelper.h"
#include "Persistence.h"

// Defines the simulated registers useful to read/write PID coefficients
// and store in the EEPROM to be persisted
#define PID_REG_P_COEFF			0x0000
#define PID_REG_I_COEFF			0x0001
#define PID_REG_D_COEFF			0x0002
#define PID_REG_MULTIPLIER		0x0003
#define PID_REG_DEADZONE_HEAT	0x0004
#define PID_REG_DEADZONE_COOL	0x0005
#define PID_REG_STORE_COEFFS	0x0006
#define PID_VAL_STORE_COEFFS	0xAA

#define PIDDEV_DEFAULT_SAMPLE_RATE	9		/* 10 seconds default sample rate */

const char* const PIDDevice::channelNames[] {
		"PIDH", "PIDC"
};

const char* const PIDDevice::channelMeasurementUnits[] = {
		"%", "%"
};

PIDDevice* const PIDDevice::instance = new PIDDevice();


PIDDevice* const PIDDevice::getInstance() {
	return PIDDevice::instance;
}

const unsigned char PIDDevice::defaultSampleRate() {
	return PIDDEV_DEFAULT_SAMPLE_RATE;
}

PIDDevice::PIDDevice() : SensorDevice(PIDDEV_NUM_OF_CHANNELS),
						go(false) {
}

PIDDevice::~PIDDevice() {

}

void PIDDevice::onStopSampling() {
}

void PIDDevice::setLowPowerMode(bool lowPower) {
}

// Read from flash the PID parameters.
// PIDs are protected by a CRC32.
bool PIDDevice::init() {

	// Read PID coefficients from EEPROM
	if (!EEPROM.read(PID_COEFFICIENTS, (unsigned char*)&pidData, sizeof(pidcoeffs))) {
		return false;
	}

	// Check for CRC
	long crc = CRC32.getCRC32((long*)&pidData, sizeof(pidcoeffs)-4);
	if (crc != pidData.crc) {
		return false;
	}

	// Store and apply
	applyPIDCoefficients();

	return true;
}

void PIDDevice::loop() {

	if (go) {
		go = false;

		// Update the reporting values with current temperature control PID driver status
		short int currentDrive = AS_TCONTROL.getCurrentDrive();

		setSample(PIDDEV_CHANNEL_PID_HEATER, (currentDrive >= 0)? currentDrive : 0);
		setSample(PIDDEV_CHANNEL_PID_COOLER, (currentDrive <= 0)? -currentDrive : 0);
	}
}

void PIDDevice::tick() {
}

const char* PIDDevice::getSerial() const {
	return "NA";
}

const char* PIDDevice::getChannelName(unsigned char channel) const {
	if (channel < PIDDEV_NUM_OF_CHANNELS) {
		return channelNames[channel];
	}

	return "";
}

bool PIDDevice::setChannelName(unsigned char channel, const char* name) {
	return false;
}

const char* PIDDevice::getMeasurementUnit(unsigned char channel) const {
	if (channel < PIDDEV_NUM_OF_CHANNELS) {
		return channelMeasurementUnits[channel];
	}

	return "";
}

float PIDDevice::evaluateMeasurement(unsigned char channel, float value) const {

	return value / 100;
}

void PIDDevice::triggerSample() {
	SensorDevice::triggerSample();
	go = true;
}

#define NIBBLEBINTOHEX(a) ((a)>0x09)?(((a)-0x0A)+'A'):((a)+'0');

bool PIDDevice::writeGenericRegister(unsigned int address, unsigned int value, unsigned char* buffer, unsigned char buffSize){

	// Start with an error result
	buffer[0] = 'K';
	buffer[1] = 'O';
	buffer[2] = '\0';

	// Return an error for unhandled coefficients
	if(address > PID_REG_STORE_COEFFS) {
		return true;
	}

	// Store coefficients to the EEPROM
	if (address == PID_REG_STORE_COEFFS) {

		if (value == PID_VAL_STORE_COEFFS) {

			// Apply then write to EEPROM
			applyPIDCoefficients();
			writePIDCoefficientsToEEPROM();

			// Return an OK result
			buffer[0] = 'O';
			buffer[1] = 'K';
			return true;
		}

		return true;
	}

	// Temporary store the PID and multiplier coefficients
	pidData.data.raw[address] = value;

	// Return an OK result
	buffer[0] = 'O';
	buffer[1] = 'K';

	return true;
}

bool PIDDevice::readGenericRegister(unsigned int address, unsigned char* buffer, unsigned char buffSize) {

	// Return an error for unhandled coefficients
	if(address >= PID_REG_STORE_COEFFS) {

		buffer[0] = 'K';
		buffer[1] = 'O';
		buffer[2] = '\0';

		return true;
	}

	// We opt for reading the local copy of the coefficients. They are synchronized
	// with the EEPROM values when the user writes PID_VAL_STORE_COEFFS to PID_REG_STORE_COEFFS
	// (and after startup)
	unsigned short value =  pidData.data.raw[address];

	buffer[0] = NIBBLEBINTOHEX((value>>12)&0x0F);
	buffer[1] = NIBBLEBINTOHEX((value>>8)&0x0F);
	buffer[2] = NIBBLEBINTOHEX((value>>4)&0x0F);
	buffer[3] = NIBBLEBINTOHEX(value&0x0F);
	buffer[4] = '\0';

	return true;
}


// Apply local PID coefficients to the PID engine
void PIDDevice::applyPIDCoefficients() {

	// Handle exceptions
	if (pidData.data.coeff.mult == 0) {
		return;
	}

	// Apply PID coefficients
	float multiplier = pidData.data.coeff.mult;
	float P = (float)pidData.data.coeff.P / multiplier;
	float I = (float)pidData.data.coeff.I / multiplier;
	float D = (float)pidData.data.coeff.D / multiplier;

	AS_TCONTROL.setCoefficients(P, I, D);
	AS_TCONTROL.setOutMinMax(-10000, 10000);

	// Apply min threshold for dead-zone cooling
	if (pidData.data.coeff.dzCool != 0xFFFF) {
		AS_TCONTROL.setCoolingStartPercentage(pidData.data.coeff.dzCool);
	}

	// Apply min threshold for dead-zone heating
	if (pidData.data.coeff.dzHeat != 0xFFFF) {
		AS_TCONTROL.setHeatingStartPercentage(pidData.data.coeff.dzHeat);
	}
}

// Write local PID coefficients to EEPROM
void PIDDevice::writePIDCoefficientsToEEPROM() {

	// Handle exceptions
	if (pidData.data.coeff.mult == 0) {
		return;
	}

	// Calculate the CRC
	pidData.crc = CRC32.getCRC32((long*)&pidData, sizeof(pidcoeffs)-4);

	// Store to EEPROM
	EEPROM.write(PID_COEFFICIENTS, (unsigned char*)&pidData, sizeof(pidcoeffs));
}

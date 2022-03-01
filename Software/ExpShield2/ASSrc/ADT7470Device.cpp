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

#include "ADT7470Device.h"
#include "I2CBHelper.h"

#define ADT7470_I2C_ADDRESS 			(0x2C<<1)

#define ADT7470_REG_BASE_TEMPERATURE	0x20
#define ADT7470_REG_BASE_FANTACH_R		0x2A
#define ADT7470_REG_FANPWM_BASE			0x32
#define ADT7470_REG_CONFIGURATION1		0x40
#define ADT7470_REG_CONFIGURATION2		0x74

#define ADT7470_REG_DEVICEID			0x3D
#define ADT7470_REG_COMPANYID			0x3E
#define ADT7470_REG_REVISIONNUMBER		0x3F

// Configuragion1 bit flags
#define ADT7470_STRT_FLAG				0x01
#define ADT7470_TODIS_FLAG				0x08
#define ADT7470_LOCK_FLAG				0x10
#define ADT7470_HF_LF					0x40
#define ADT7470_T05_STB					0x80

// Configuration2 bit flags
#define ADT7470_SHUTDOWN_FLAG			0x01
#define ADT7470_22KHZ_FLAG				0x10
#define ADT7470_44_1HZ_FLAG				0x50
#define ADT7470_88_2HZ_FLAG				0x70

// Minimum PWM values for fans. Fans are turned off if PWM is below this threshold
#define MIN_F_EXT_HEATSINK_PERC			0x0F
#define MIN_F_INT_HEATSINK_PERC			0x10
#define MIN_F_AIR_CIR_PERC				0x10

// Min and max temperature setpoint values in C degrees
#define MIN_TEMPERATURE_SETPOINT		15
#define MAX_TEMPERATURE_SETPOINT		40

const char* const ADT7470Device::channelNames[] {
		"TICHMBR", "TEHTSNK", "TIHTSNK", "FEHTSNK", "FIHTSNK", "FACIRC"
};

const char* const ADT7470Device::channelMeasurementUnits[] = {
		"C", "C", "C", "rpm", "rpm", "rpm"
};

ADT7470Device* const ADT7470Device::instance = new ADT7470Device();


ADT7470Device* const ADT7470Device::getInstance() {
	return ADT7470Device::instance;
}

const unsigned char ADT7470Device::defaultSampleRate() {
	return ADT7470_SAMPLING_PERIOD;
}

ADT7470Device::ADT7470Device() : SensorDevice(ADT7470_NUM_CHANNELS),
				go(false), error(false), communicationTimer(ADT7470_COMMUNICATION_PERIOD*3/4) {
}

ADT7470Device::~ADT7470Device() {
}

void ADT7470Device::onStopSampling() {
}

void ADT7470Device::setLowPowerMode(bool lowPower) {
}

// Device's loop function is always called, even when sampling is not running
// This device always communicates to the ADT7470 IC, independently
// by the sampling procedure.
// This is required because fan control and temperature readings
// are needed by the external temperature control engine.
void ADT7470Device::loop() {

	if (communicationTimer > ((error)? ADT7470_COMMUNICATION_PERIOD_ON_ERR :
									   ADT7470_COMMUNICATION_PERIOD)) {
		communicationTimer = 0;

		// Communicate here with the device. Update the error status
		readTemperatures();
		readFanSpeeds();
	}

	if (!error && go) {
		go = false;

		// Report read data to the higher reporting layers
		for (unsigned char n = 0; n < ADT7470_NUM_TEMPERATURE_CHANNELS; n++) {

			setSample(n, temperatures[n]);	// Temperatures are stored in 1/100 C units
			setSample(n+ADT7470_NUM_TEMPERATURE_CHANNELS, fansSpeed[n]);
		}
	}
}

// This device always communicates to the ADT7470 IC,
// independently by the sampling procedure.
// This is required because fan control and temperature readings
// are needed by the external temperature control engine.
// Device's tick function is called 10 times a second.
// We plan to communicate with the ADT7470 IC each second
void ADT7470Device::tick() {
	communicationTimer++;
}

const char* ADT7470Device::getSerial() const {
	return "NA";
}

const char* ADT7470Device::getChannelName(unsigned char channel) const {
	if (channel < ADT7470_NUM_CHANNELS) {
		return channelNames[channel];
	}

	return "";
}

bool ADT7470Device::setChannelName(unsigned char channel, const char* name) {
	return false;
}

const char* ADT7470Device::getMeasurementUnit(unsigned char channel) const {
	if (channel < ADT7470_NUM_CHANNELS) {
		return channelMeasurementUnits[channel];
	}

	return "";
}

float ADT7470Device::evaluateMeasurement(unsigned char channel, float value) const {

	if (channel < ADT7470_NUM_TEMPERATURE_CHANNELS) {

		// Temperatures are stored in 1/100 C units
		return value / 100;
	} else if ((channel > ADT7470_CHANNEL_T_INT_HEATSINK) && (channel <= ADT7470_CHANNEL_F_AIR_CIR)) {

		// Exception: Rotation below the readable minimum
		if ((unsigned int)value >= 0xFFFF) {
			return 0.0f;
		}

		// Exception: Rotation over the readable maximum
		if ((unsigned short)value == 0x00) {
			return 65535.0;
		}

		// Real case: RPM = (90000 x 60)/fanSpeed (see datasheet page 24)
		return (90000*60)/((unsigned short)value);
	}

	return value;
}

void ADT7470Device::triggerSample() {
	SensorDevice::triggerSample();
	go = true;
}


bool ADT7470Device::setSetpointForChannel(unsigned char channel, unsigned char setpoint) {

	if (channel < ADT7470_NUM_CHANNELS) {

		// Check for setpoint validity.
		// Setpoints are in 1/100 units
		if (channel < ADT7470_NUM_TEMPERATURE_CHANNELS) {

			// Avoid temperature setpoint to invalid values
			if (setpoint < MIN_TEMPERATURE_SETPOINT) {
				setpoint = MIN_TEMPERATURE_SETPOINT;
			} else if (setpoint > MAX_TEMPERATURE_SETPOINT) {
				setpoint = MAX_TEMPERATURE_SETPOINT;
			}

		} else {

			// Clamp to 100% for fan speed
			if (setpoint > 100) {
				setpoint = 100;
			}
 		}

		setpoints[channel] = setpoint * 100;

		return true;
	}

	return false;
}


bool ADT7470Device::getSetpointForChannel(unsigned char channel, unsigned char& setpoint) {

	if (channel < ADT7470_NUM_CHANNELS) {
		setpoint = setpoints[channel] / 100;

		return true;
	}

	return false;
}

// Initialize the external IC registers
// Returns true on success; false on error;
bool ADT7470Device::init() {

	// Check for device presence
	unsigned char ucRegisterVal = 0x00;
	error = !I2CB.read(ADT7470_I2C_ADDRESS, ADT7470_REG_DEVICEID, 1, &ucRegisterVal, 1);
	if (error || (ucRegisterVal != 0x70)) {
		return false;
	}
	error = !I2CB.read(ADT7470_I2C_ADDRESS, ADT7470_REG_COMPANYID, 1, &ucRegisterVal, 1);
	if (error || (ucRegisterVal != 0x41)) {
		return false;
	}

	unsigned char config1 = ADT7470_STRT_FLAG | ADT7470_T05_STB;	// See datasheet, Table 31
	error = !I2CB.write(ADT7470_I2C_ADDRESS, ADT7470_REG_CONFIGURATION1, 1, &config1, 1);
	if (error) {
		return false;
	}

	unsigned char config2 = ADT7470_22KHZ_FLAG; 	// See datasheet, Table 44
	error = !I2CB.write(ADT7470_I2C_ADDRESS, ADT7470_REG_CONFIGURATION2, 1, &config2, 1);
	if (error) {
		return false;
	}

	// Turn off the fans
	setCirculationFanSpeed(0);
	setExternalFanSpeed(0);
	setInternalFanSpeed(0);

	return !error;
}

// Temperatures are returned in 1/100 C units
void ADT7470Device::getChamberSetpointAndTemperature(short& setpoint, short& temperature) {

	setpoint = setpoints[ADT7470_CHANNEL_T_INT_CHAMBER];
	temperature = temperatures[ADT7470_CHANNEL_T_INT_CHAMBER];
}

// Read temperatures registers and update the error status
void ADT7470Device::readTemperatures() {

	// Shutdow temperature measurement for a while
	resetRegisterFlag(ADT7470_REG_CONFIGURATION1, ADT7470_T05_STB);
	if (error) {
		return;
	}

	// Read temperature registers
	for (unsigned char n = 0; n < ADT7470_NUM_TEMPERATURE_CHANNELS; n++) {
		unsigned char temperature;
		error = !I2CB.read(ADT7470_I2C_ADDRESS, ADT7470_REG_BASE_TEMPERATURE+n, 1, &temperature, 1);
		if (!error) {

			// Temperatures are stored in 1/100 units
			temperatures[n] = ((char)temperature) * 100;
		}
	}

	// Restart temperature measurement
	setRegisterFlag(ADT7470_REG_CONFIGURATION1, ADT7470_T05_STB);
}

// Read fan speed registers and update the error status
void ADT7470Device::readFanSpeeds() {

	for (unsigned char n = 0; n < ADT7470_NUM_FAN_CHANNELS; n++) {

		unsigned char speedL, speedH;
		error = !I2CB.read(ADT7470_I2C_ADDRESS, ADT7470_REG_BASE_FANTACH_R + (n<<1), 1, &speedL, 1);
		if (!error) {
			error = !I2CB.read(ADT7470_I2C_ADDRESS, ADT7470_REG_BASE_FANTACH_R + (n<<1) + 1, 1, &speedH, 1);
			if (!error) {
				fansSpeed[n] = (((unsigned short)speedH)<<8) + speedL;
			}
		}
	}
}

// Set air circulation fan speed from 0 to 100 %
void ADT7470Device::setCirculationFanSpeed(unsigned char percentage) {
	unsigned char effectivePercentage = (percentage == 0) ? 0 : ((unsigned char)(((percentage - 1)) * (((float)(100-MIN_F_AIR_CIR_PERC)) / 99) + MIN_F_AIR_CIR_PERC));
	setFanSpeed(ADT7470_CHANNEL_F_AIR_CIR, effectivePercentage);
}

// Set air circulation fan speed at setpoint
void ADT7470Device::setCirculationFanSpeedAtSetpoint() {
	setCirculationFanSpeed(setpoints[ADT7470_CHANNEL_F_AIR_CIR]/100);
}

// Set external fan speed from 0 to 100 %
void ADT7470Device::setExternalFanSpeed(unsigned char percentage) {
	unsigned char effectivePercentage = (percentage == 0) ? 0 : ((unsigned char)(((percentage - 1)) * (((float)(100-MIN_F_EXT_HEATSINK_PERC)) / 99) + MIN_F_EXT_HEATSINK_PERC));
	setFanSpeed(ADT7470_CHANNEL_F_EXT_HEATSINK, effectivePercentage);
}

// Set external fan speed at setpoint
void ADT7470Device::setExternalFanSpeedAtSetpoint() {
	setExternalFanSpeed(setpoints[ADT7470_CHANNEL_F_EXT_HEATSINK]/100);
}

// Set internal fan speed from 0 to 100 %
void ADT7470Device::setInternalFanSpeed(unsigned char percentage) {
	unsigned char effectivePercentage = (percentage == 0) ? 0 : ((unsigned char)(((percentage - 1)) * (((float)(100-MIN_F_INT_HEATSINK_PERC)) / 99) + MIN_F_INT_HEATSINK_PERC));
	setFanSpeed(ADT7470_CHANNEL_F_INT_HEATSINK, effectivePercentage);
}

// Set internal fan speed at setpoint
void ADT7470Device::setInternalFanSpeedAtSetpoint() {
	setInternalFanSpeed(setpoints[ADT7470_CHANNEL_F_INT_HEATSINK]/100);
}

// Set a specific flag in a remote register
void ADT7470Device::setRegisterFlag(unsigned char address, unsigned char hexFlag) {

	unsigned char regVal;
	error = !I2CB.read(ADT7470_I2C_ADDRESS, address, 1, &regVal, 1);
	if (error) {
		return;
	}

	regVal |= hexFlag;

	error = !I2CB.write(ADT7470_I2C_ADDRESS, address, 1, &regVal, 1);
}

// Reset a specific flag in a remote register
void ADT7470Device::resetRegisterFlag(unsigned char address, unsigned char hexFlag) {

	unsigned char regVal;
	error = !I2CB.read(ADT7470_I2C_ADDRESS, address, 1, &regVal, 1);
	if (error) {
		return;
	}

	regVal &= (0xFF ^ hexFlag);

	error = !I2CB.write(ADT7470_I2C_ADDRESS, address, 1, &regVal, 1);
}

// Set a specific FAN speed in percentage (0 to 100)
void ADT7470Device::setFanSpeed(unsigned char fanID, unsigned char percentage) {

	fanID -= ADT7470_CHANNEL_F_EXT_HEATSINK;
	if (fanID > ADT7470_NUM_FAN_CHANNELS) {
		return;
	}

	unsigned char pwmValue = (percentage == 100)? 255 : (percentage * 2.57);

	error = !I2CB.write(ADT7470_I2C_ADDRESS, ADT7470_REG_FANPWM_BASE + fanID, 1, &pwmValue, 1);
}

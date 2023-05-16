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

#include "SensorsArray.h"
#include "Sampler.h"
#include "FixedRateSampler.h"
#include "SamplesAverager.h"
#include "K96SamplesAverager.h"
#include "SensorDevice.h"
#include "D300Device.h"
#include "IntADDevice.h"
#include "SHT31Device.h"
#include "PIDDevice.h"
#include "ADT7470Device.h"
#include "K96Device.h"
#include "Persistence.h"
#include "EEPROMHelper.h"
#include "GPIOHelper.h"
#include <string.h>


#define BOARD_TYPE_EXP2_SENSORSHIELD		0x05

#define GLOBAL_SAMPLE_PRESCALER_RATIO		100		/* 1 second */

DitherTool SensorsArray::ditherTool = DitherTool();

const SensorsArray::channeltosamplersubchannel SensorsArray::chToSamplerSubChannel[NUM_OF_TOTAL_CHANNELS] = {
		{ SENSOR_SHT31_I, SHT31_CHANNEL_TEMPERATURE, true },
		{ SENSOR_SHT31_I, SHT31_CHANNEL_HUMIDITY, false },
		{ SENSOR_SHT31_E, SHT31_CHANNEL_TEMPERATURE, true },
		{ SENSOR_SHT31_E, SHT31_CHANNEL_HUMIDITY, false },
		{ SENSOR_INTAD, INTAD_CHANNEL_PLT_VFBK, true },
		{ SENSOR_INTAD, INTAD_CHANNEL_PLT_CFBK, false },
		{ SENSOR_INTAD, INTAD_CHANNEL_VIN_FBK, false },
		{ SENSOR_INTAD, INTAD_CHANNEL_TEMPERATURE, false },
		{ SENSOR_PID, PIDDEV_CHANNEL_PID_HEATER, true },
		{ SENSOR_PID, PIDDEV_CHANNEL_PID_COOLER, false },
		{ SENSOR_ADT7470, ADT7470_CHANNEL_T_INT_CHAMBER, true },
		{ SENSOR_ADT7470, ADT7470_CHANNEL_T_EXT_HEATSINK, false },
		{ SENSOR_ADT7470, ADT7470_CHANNEL_T_INT_HEATSINK, false },
		{ SENSOR_ADT7470, ADT7470_CHANNEL_F_EXT_HEATSINK, false },
		{ SENSOR_ADT7470, ADT7470_CHANNEL_F_INT_HEATSINK, false },
		{ SENSOR_ADT7470, ADT7470_CHANNEL_F_AIR_CIR, false },
		{ SENSOR_D300, 0 , true },
		{ SENSOR_K96, K96_CHANNEL_LPL_PC_FLT, true },
		{ SENSOR_K96, K96_CHANNEL_SPL_PC_FLT, false },
		{ SENSOR_K96, K96_CHANNEL_MPL_PC_FLT, false },
		{ SENSOR_K96, K96_CHANNEL_PRESS0, false },
		{ SENSOR_K96, K96_CHANNEL_TEMP_NTC0, false },
		{ SENSOR_K96, K96_CHANNEL_TEMP_NTC1, false },
		{ SENSOR_K96, K96_CHANNEL_TEMP_UCDIE, false },
		{ SENSOR_K96, K96_CHANNEL_RH0, false },
		{ SENSOR_K96, K96_CHANNEL_T_RH0, false },
		{ SENSOR_K96, K96_CHANNEL_LPL_UFLT_IR, false },
		{ SENSOR_K96, K96_CHANNEL_SPL_UFLT_IR, false },
		{ SENSOR_K96, K96_CHANNEL_MPL_UFLT_IR, false },
		{ SENSOR_K96, K96_CHANNEL_ERRORSTATUS, false },
		{ SENSOR_K96, K96_CHANNEL_LPL_UFLT_ERR, false },
		{ SENSOR_K96, K96_CHANNEL_SPL_UFLT_ERR, false },
		{ SENSOR_K96, K96_CHANNEL_MPL_UFLT_ERR, false },
};

SensorsArray::SensorsArray() :
		sensors{ new SHT31Device(true),
				 new SHT31Device(false),
				 IntADDevice::getInstance(),
				 PIDDevice::getInstance(),
				 ADT7470Device::getInstance(),
				 new D300Device(),
				 new K96Device()
		},
		samplers{ new Sampler(sensors[SENSOR_SHT31_I]),
				  new Sampler(sensors[SENSOR_SHT31_E]),
				  new FixedRateSampler(sensors[SENSOR_INTAD], IntADDevice::defaultSampleRate()),
				  new FixedRateSampler(sensors[SENSOR_PID], PIDDevice::defaultSampleRate()),
				  new FixedRateSampler(sensors[SENSOR_ADT7470], ADT7470Device::defaultSampleRate()),
				  new FixedRateSampler(sensors[SENSOR_D300], D300Device::defaultSampleRate()),
				  new Sampler(sensors[SENSOR_K96])
		},
		averagers{
				new SamplesAverager(sensors[SENSOR_SHT31_I]->getNumChannels()),
				new SamplesAverager(sensors[SENSOR_SHT31_E]->getNumChannels()),
				new SamplesAverager(sensors[SENSOR_INTAD]->getNumChannels()),
				new SamplesAverager(sensors[SENSOR_PID]->getNumChannels()),
				new SamplesAverager(sensors[SENSOR_ADT7470]->getNumChannels()),
				new SamplesAverager(sensors[SENSOR_D300]->getNumChannels()),
				new K96SamplesAverager(sensors[SENSOR_K96]->getNumChannels())
		},
		samplingEnabled(false), timestamp(0), globalPrescaler(0)
	{

    // Set the dithering tool. See the above comment.
    averagers[SENSOR_SHT31_I]->setDitherTool(&ditherTool);
    
    // Initialize by reading the preset stored into the EEPROM
    for (unsigned char n = 0; n < NUM_OF_TOTAL_CHANNELS; n++) {
        loadPreset(n);
    }
}


SensorsArray::~SensorsArray() {
    for (unsigned char n = 0; n < NUM_OF_TOTAL_SAMPLERS; n++) {
        if (samplers[n]) {
            delete samplers[n];
        }
    }

    for (unsigned char n = 0; n < NUM_OF_TOTAL_SENSORS; n++) {
		if (sensors[n]) {
			delete sensors[n];
		}
		if (averagers[n]) {
			delete averagers[n];
		}
    }
}

bool SensorsArray::init() {

	bool result = true;

	// Propagate to all devices so that they may be initialized
	// with full access to working hardware resources
    for (unsigned char n = 0; n < NUM_OF_TOTAL_SENSORS; n++) {
		if (sensors[n] != 0) {
			result &= sensors[n]->init();
		}
    }

    return result;
}

bool SensorsArray::timerTick() {
    
    bool result = false;

    // Propagate to all devices requiring fast rate (one sampletick = 0.01s)
    for (unsigned char n = 0; n < NUM_OF_TOTAL_SENSORS; n++) {
    		if (sensors[n] != 0) {
    			sensors[n]->tick();
    		}
    }

    // Increase the internal timestamp (in 0.01s)
    timestamp++;

    // If sampling is disabled, this function has nothing more to do
    if (!samplingEnabled) {
        return false;
    }

    // This shield does not requires fast sampling rates. A global prescaler
    // is used to reduce the sampling rate to a reasonable value (one sampletick = 1s)
    globalPrescaler++;
    if (globalPrescaler >= GLOBAL_SAMPLE_PRESCALER_RATIO) {
    		globalPrescaler = 0;

		// Loop on each sensor samplers
		for (unsigned char n = 0; n < NUM_OF_TOTAL_SAMPLERS; n++) {
			if (samplers[n] != 0) {
				result |= samplers[n]->sampleTick();
			}
		}
    }
    
    return result;
}

bool SensorsArray::loop() {

	// Call all devices loop
	for (unsigned char n = 0; n < NUM_OF_TOTAL_SENSORS; n++) {
		if (sensors[n]) {
			sensors[n]->loop();
		}
	}

    // If sampling is disabled, this function has nothing more to do
    if (!samplingEnabled) {
        return false;
    }
    
    long currentTimestamp = timestamp;
    bool result = false;
    for (unsigned char n = 0; n < NUM_OF_TOTAL_SAMPLERS; n++) {
    		if (samplers[n] && samplers[n]->sampleLoop()) {
    			for (unsigned char subChannel = 0; subChannel < samplers[n]->getNumChannels(); subChannel++) {
    				// A new set of samples are ready to be averaged
    				result |= averagers[n]->collectSample(subChannel, samplers[n]->getLastSample(subChannel), currentTimestamp);
    			}
    		}
    }

    return result;
}

unsigned char SensorsArray::setSamplePrescaler(unsigned char channel, unsigned char prescaler) {
    
    if (channel >= NUM_OF_TOTAL_CHANNELS)
        return 0;
    
    samplers[chToSamplerSubChannel[channel].sampler]->setPreScaler(prescaler);
    
    return 1;
}

bool SensorsArray::getSamplePrescaler(unsigned char channel, unsigned char* prescaler) {
    
    if (channel >= NUM_OF_TOTAL_CHANNELS)
        return false;
    
    *prescaler = samplers[chToSamplerSubChannel[channel].sampler]->getPrescaler();

    return true;
}

unsigned char SensorsArray::setSamplePostscaler(unsigned char channel, unsigned char postscaler) {
    
    if (channel >= NUM_OF_TOTAL_CHANNELS)
        return 0;
    
    return (averagers[chToSamplerSubChannel[channel].sampler]->init(postscaler) != 0) ;
}

bool SensorsArray::getSamplePostscaler(unsigned char channel, unsigned char* postscaler) {
    
    if (channel >= NUM_OF_TOTAL_CHANNELS) {
        return false;
    }
    
    *postscaler = averagers[chToSamplerSubChannel[channel].sampler]->getBufferSize();

    return true;    
}

unsigned char SensorsArray::setSampleDecimation(unsigned char channel, unsigned char decimation) {

    if (channel >= NUM_OF_TOTAL_CHANNELS) {
        return 0;
    }

	samplers[chToSamplerSubChannel[channel].sampler]->setDecimation(decimation);

    return 1;
}

bool SensorsArray::getSampleDecimation(unsigned char channel, unsigned char* decimation) {
    if (channel >= NUM_OF_TOTAL_CHANNELS)
        return false;

    *decimation = samplers[chToSamplerSubChannel[channel].sampler]->getDecimation();
    
    return true;
}

bool SensorsArray::enableSampling(bool enable) {

	// Initialize samplers when start sampling
	if (enable && !samplingEnabled) {
		globalPrescaler = 0;
		for (unsigned char n = 0; n < NUM_OF_TOTAL_SAMPLERS; n++) {
			samplers[n]->onStartSampling();
		}
	}

	// Shutdown samplers at sampling termination
	if (!enable) {
		for (unsigned char n = 0; n < NUM_OF_TOTAL_SAMPLERS; n++) {
			samplers[n]->onStopSampling();
		}
	}

	// Enable/Disable sampling
    samplingEnabled = enable;
    
    return true;
}

void SensorsArray::inquirySensor(unsigned char channel, unsigned char* buffer, unsigned char bufSize) {

    memset(buffer, 0, bufSize);
    
    if (channel >= NUM_OF_TOTAL_CHANNELS)
        return;

    const char* name = sensors[chToSamplerSubChannel[channel].sampler]->getChannelName(chToSamplerSubChannel[channel].subchannel);
    bufSize = (bufSize < strlen(name))? bufSize:strlen(name);

    strncpy((char*)buffer, name, bufSize);
}


bool SensorsArray::savePreset(unsigned char channel, unsigned char* presetName, unsigned char bufSize) {

    if (channel >= NUM_OF_TOTAL_CHANNELS) {
        return false;
    }
    
    bool result = samplers[chToSamplerSubChannel[channel].sampler]->savePreset(chToSamplerSubChannel[channel].sampler);

	// Save averager preset
	result &= averagers[chToSamplerSubChannel[channel].sampler]->savePreset(chToSamplerSubChannel[channel].sampler);
    
    return result;
}

bool SensorsArray::loadPreset(unsigned char channel) {
    
    if (channel >= NUM_OF_TOTAL_CHANNELS) {
        return false;
    }

    bool result = true;
    
    // Read sampler preset
    result &= samplers[chToSamplerSubChannel[channel].sampler]->loadPreset(chToSamplerSubChannel[channel].sampler);
    
    // Read averager preset
    result &= averagers[chToSamplerSubChannel[channel].sampler]->loadPreset(chToSamplerSubChannel[channel].sampler);
    
    return result;
}

bool SensorsArray::getLastSample(unsigned char channel, unsigned short& lastSample, unsigned long& timestamp) {

    lastSample = 0;
    timestamp = 0;
    
    if (channel < NUM_OF_TOTAL_CHANNELS) {
    	unsigned char enabled = false;
    	samplers[chToSamplerSubChannel[channel].sampler]->getChannelIsEnabled(chToSamplerSubChannel[channel].subchannel, &enabled);

    	if (enabled != 0) {
			lastSample = averagers[chToSamplerSubChannel[channel].sampler]->lastAveragedValue(chToSamplerSubChannel[channel].subchannel);
			timestamp = averagers[chToSamplerSubChannel[channel].sampler]->lastTimeStamp();
    	}
		return true;
    } 
    
    return false;
}

bool SensorsArray::getLastSample(unsigned char channel, float &lastSample, unsigned long &timestamp) {

    lastSample = 0.0f;
    timestamp = 0;

    if (channel < NUM_OF_TOTAL_CHANNELS) {

    	unsigned char enabled = false;
    	samplers[chToSamplerSubChannel[channel].sampler]->getChannelIsEnabled(chToSamplerSubChannel[channel].subchannel, &enabled);

    	if (enabled != 0) {
			// Retrieve the timestamp
			timestamp = averagers[chToSamplerSubChannel[channel].sampler]->lastTimeStamp();

			// Retrieve the averaged value...
			lastSample = averagers[chToSamplerSubChannel[channel].sampler]->lastAveragedFloatValue(chToSamplerSubChannel[channel].subchannel);

			// Evaluate it. Evaluation is done by sensor devices
			lastSample = sensors[chToSamplerSubChannel[channel].sampler]->evaluateMeasurement(chToSamplerSubChannel[channel].subchannel, lastSample, timestamp == 0);
    	}

		return true;
    }

    return false;
}


bool SensorsArray::setSetpoint(unsigned char channel, unsigned short setPointVal) {

	return ((channel < NUM_OF_TOTAL_CHANNELS) &&
			samplers[chToSamplerSubChannel[channel].sampler]->setSetpointForChannel(chToSamplerSubChannel[channel].subchannel, setPointVal));
}


bool SensorsArray::getSetpoint(unsigned char channel, unsigned short& setPointVal) {

	return ((channel < NUM_OF_TOTAL_CHANNELS) &&
			samplers[chToSamplerSubChannel[channel].sampler]->getSetpointForChannel(chToSamplerSubChannel[channel].subchannel, setPointVal));
}


bool SensorsArray::saveSensorSerialNumber(unsigned char channel, unsigned char* buffer, unsigned char buffSize) {

	if (channel >= NUM_OF_TOTAL_CHANNELS) {
		return false;
	}

	// Write the serial number only if the sensor does not support serial number natively
	if (sensors[chToSamplerSubChannel[channel].sampler]->getSerial() == NULL) {
		unsigned short address = SENSOR_SERIAL_NUMBER(chToSamplerSubChannel[channel].sampler);

		// Safety check of incoming data
		unsigned char maxSize = (buffSize < SERIAL_NUMBER_MAXLENGTH)? buffSize : SERIAL_NUMBER_MAXLENGTH;
		buffer[SERIAL_NUMBER_MAXLENGTH-1] = 0;

		return EEPROM.write(address, buffer, maxSize);
	}

	return true;
}

bool SensorsArray::readSensorSerialNumber(unsigned char channel, unsigned char* buffer, unsigned char buffSize) {

	*buffer = 0x00;
	if (channel >= NUM_OF_TOTAL_CHANNELS) {
		return false;
	}

	unsigned char maxSize = (buffSize < SERIAL_NUMBER_MAXLENGTH)? buffSize : SERIAL_NUMBER_MAXLENGTH;

	// Read the serial number from local EEPROM only if the sensor does not support serial number natively
	if (sensors[chToSamplerSubChannel[channel].sampler]->getSerial() == NULL) {
		unsigned short address = SENSOR_SERIAL_NUMBER(chToSamplerSubChannel[channel].sampler);
		for (unsigned char n = 0; n < maxSize; n++) {
			unsigned char ucRead = EEPROM.read(address);
			if (ucRead == 0xFF) {
				ucRead = 0;
			}
			buffer[n] = ucRead;
			address++;
		}
		buffer[maxSize-1] = 0;
	} else {
		const char* sensorSerial = sensors[chToSamplerSubChannel[channel].sampler]->getSerial();
		strncpy((char*)buffer, sensorSerial, maxSize);
		buffer[maxSize-1] = 0x00;
	}

    return true;
}

bool SensorsArray::saveBoardSerialNumber(unsigned char* buffer, unsigned char buffSize) {

	unsigned short address = BOARD_SERIAL_NUMBER;
	return EEPROM.write(address, buffer, buffSize);
}

bool SensorsArray::readBoardSerialNumber(unsigned char* buffer, unsigned char buffSize) {

    unsigned short address = BOARD_SERIAL_NUMBER;
    unsigned char maxSize = (buffSize < SERIAL_NUMBER_MAXLENGTH)? buffSize : SERIAL_NUMBER_MAXLENGTH;
    for (unsigned char n = 0; n < maxSize; n++) {
        unsigned char ucRead = EEPROM.read(address);
        if (ucRead == 0xFF) {
            ucRead = 0;
        }
        buffer[n] = ucRead;
        address++;
    }
    buffer[maxSize-1] = 0;

    return true;
}


bool SensorsArray::readChannelSamplePeriod(unsigned char channel, unsigned long *samplePeriod) {

	if ((channel >= NUM_OF_TOTAL_CHANNELS) || (samplePeriod == 0)) {
		return false;
	}

    unsigned char samplerPrescaler = samplers[chToSamplerSubChannel[channel].sampler]->getPrescaler() + 1;
    unsigned char samplerDecimation = samplers[chToSamplerSubChannel[channel].sampler]->getDecimation() + 1;
    unsigned char averagerBufferSize = averagers[chToSamplerSubChannel[channel].sampler]->getBufferSize() + 1;

	*samplePeriod = samplerPrescaler * samplerDecimation * averagerBufferSize * 1000;

	return true;
}

bool SensorsArray::getUnitForChannel(unsigned char channel, unsigned char* buffer, unsigned char buffSize) {

	if (channel >= NUM_OF_TOTAL_CHANNELS) {
		return false;
	}

	// Units for each channel is known by sensor devices
	const char* units = sensors[chToSamplerSubChannel[channel].sampler]->getMeasurementUnit(chToSamplerSubChannel[channel].subchannel);
    strcpy((char*)buffer, units);

	return true;
}

bool SensorsArray::setEnableChannel(unsigned char channel, unsigned char enabled) {

	if (channel >= NUM_OF_TOTAL_CHANNELS) {
		return false;
	}

	// Enabled status is known by it's sampler
	return samplers[chToSamplerSubChannel[channel].sampler]->setEnableChannel(chToSamplerSubChannel[channel].subchannel, enabled);
}

bool SensorsArray::getChannelIsEnabled(unsigned char channel, unsigned char *enabled) {

	if (channel >= NUM_OF_TOTAL_CHANNELS) {
		return false;
	}

	// Enabled status is known by it's sampler
	return samplers[chToSamplerSubChannel[channel].sampler]->getChannelIsEnabled(chToSamplerSubChannel[channel].subchannel, enabled);
}

bool SensorsArray::writeGenericRegisterChannel(unsigned char channel, unsigned int address, unsigned int value) {

	if (channel >= NUM_OF_TOTAL_CHANNELS) {
		return false;
	}

	return sensors[chToSamplerSubChannel[channel].sampler]->writeGenericRegister(address, value);
}

bool SensorsArray::readGenericRegisterChannel(unsigned char channel, unsigned int address, unsigned int& value) {

	if (channel >= NUM_OF_TOTAL_CHANNELS) {
		return false;
	}

	return sensors[chToSamplerSubChannel[channel].sampler]->readGenericRegister(address, value);
}


unsigned short SensorsArray::getBoardType() {
	return BOARD_TYPE_EXP2_SENSORSHIELD;
}

unsigned short SensorsArray::getBoardNumChannels() {
	return NUM_OF_TOTAL_CHANNELS;
}

bool SensorsArray::getIsFlyboardReady() {
	return ((SHT31Device*)sensors[SENSOR_SHT31_E])->isAvailable();
}

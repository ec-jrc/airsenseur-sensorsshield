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
#include "SamplesAveragerOPCN3.h"
#include "SensorDevice.h"
#include "RD200MDevice.h"
#include "PMS5003Device.h"
#include "D300Device.h"
#include "OPCN3Device.h"
#include "Persistence.h"
#include "EEPROMHelper.h"
#include "GPIOHelper.h"
#include <string.h>

#define GLOBAL_SAMPLE_PRESCALER_RATIO	99

DitherTool SensorsArray::ditherTool = DitherTool();

const SensorsArray::channeltosamplersubchannel SensorsArray::chToSamplerSubChannel[NUM_OF_TOTAL_CHANNELS] = {
		{ SENSOR_RD200M, 0, true },
		{ SENSOR_D300, 0 , true },
		{ SENSOR_PMS5300, PMS5300_PM1CONC_ST, true },{ SENSOR_PMS5300, PMS5300_PM25CONC_ST, false },{ SENSOR_PMS5300, PMS5300_PM10CONC_ST, false },
		{ SENSOR_PMS5300, PMS5300_PM1CONC_AT, false },{ SENSOR_PMS5300, PMS5300_PM25CONC_AT, false },{ SENSOR_PMS5300, PMS5300_PM10CONC_AT, false },
		{ SENSOR_PMS5300, PMS5300_PART03, false },{ SENSOR_PMS5300, PMS5300_PART05, false },{ SENSOR_PMS5300, PMS5300_PART10, false },
		{ SENSOR_PMS5300, PMS5300_PART25, false },{ SENSOR_PMS5300, PMS5300_PART50, false },{ SENSOR_PMS5300, PMS5300_PART100, false },
		{ SENSOR_OPCN3, OPCN3_BIN0, true },{ SENSOR_OPCN3, OPCN3_BIN1 },{ SENSOR_OPCN3, OPCN3_BIN2, false },{ SENSOR_OPCN3, OPCN3_BIN3, false },
		{ SENSOR_OPCN3, OPCN3_BIN4, false },{ SENSOR_OPCN3, OPCN3_BIN5, false },{ SENSOR_OPCN3, OPCN3_BIN6, false },{ SENSOR_OPCN3, OPCN3_BIN7, false },
		{ SENSOR_OPCN3, OPCN3_BIN8, false },{ SENSOR_OPCN3, OPCN3_BIN9, false },{ SENSOR_OPCN3, OPCN3_BIN10, false },{ SENSOR_OPCN3, OPCN3_BIN11, false },
		{ SENSOR_OPCN3, OPCN3_BIN12, false },{ SENSOR_OPCN3, OPCN3_BIN13, false },{ SENSOR_OPCN3, OPCN3_BIN14, false },{ SENSOR_OPCN3, OPCN3_BIN15, false },
		{ SENSOR_OPCN3, OPCN3_BIN16, false },{ SENSOR_OPCN3, OPCN3_BIN17, false },{ SENSOR_OPCN3, OPCN3_BIN18, false },{ SENSOR_OPCN3, OPCN3_BIN19, false },
		{ SENSOR_OPCN3, OPCN3_BIN20, false },{ SENSOR_OPCN3, OPCN3_BIN21, false },{ SENSOR_OPCN3, OPCN3_BIN22, false },{ SENSOR_OPCN3, OPCN3_BIN23, false },
		{ SENSOR_OPCN3, OPCN3_PM01, false },{ SENSOR_OPCN3, OPCN3_PM25, false },{ SENSOR_OPCN3, OPCN3_PM10, false },
		{ SENSOR_OPCN3, OPCN3_TEMP, false },{ SENSOR_OPCN3, OPCN3_HUM, false },{ SENSOR_OPCN3, OPCN3_VOL, false },
		{ SENSOR_OPCN3, OPCN3_TSA, false },{ SENSOR_OPCN3, OPCN3_FRT, false },{ SENSOR_OPCN3, OPCN3_LSRST, false },
};

SensorsArray::SensorsArray() : samplingEnabled(false), timestamp(0), globalPrescaler(0) {

	// Turn on power supply for external sensors
	powerUp5V(true);
	powerUp12V(true);
	powerUp3V3(true);

    // Initialize the samplers and averagers array (yes, I know, this could be skipped but it's done for safety reasons)
    memset(samplers, 0, sizeof(samplers));
    memset(averagers, 0, sizeof(averagers));
    
    // Initialize the sensor drivers
    sensors[SENSOR_RD200M] = new RD200MDevice();
    sensors[SENSOR_D300] = new D300Device();
    sensors[SENSOR_PMS5300] = new PMS5003Device();
    sensors[SENSOR_OPCN3] = new OPCN3Device();

    // Initialize the sampler units
    samplers[SENSOR_RD200M] = new FixedRateSampler(1, sensors[SENSOR_RD200M], RD200MDevice::defaultSampleRate(), RD200MDevice::defaultDecimationValue());
    samplers[SENSOR_D300] = new FixedRateSampler(1, sensors[SENSOR_D300], D300Device::defaultSampleRate());
    samplers[SENSOR_PMS5300] = new FixedRateSampler(PSM5003_NUM_CHANNELS, sensors[SENSOR_PMS5300], FixedRateSampler::DeviceDrivenSampleRate());
    samplers[SENSOR_OPCN3] = new Sampler(OPCN3_CHAN_NUMBER, sensors[SENSOR_OPCN3]);

    // Create the averagers
    averagers[SENSOR_RD200M] = new SamplesAverager(1);
    averagers[SENSOR_D300] = new SamplesAverager(1);
    averagers[SENSOR_PMS5300] = new SamplesAverager(PSM5003_NUM_CHANNELS);
    averagers[SENSOR_OPCN3] = new SamplesAveragerOPCN3();
    
    // Set the dithering tool. See the above comment.
    averagers[CHANNEL_RD200M]->setDitherTool(&ditherTool);
    
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
    }
}


bool SensorsArray::timerTick() {
    
    bool result = false;

    // Propagate to all devices with fast rate (one sampletick = 0.01s)
    for (unsigned char n = 0; n < NUM_OF_TOTAL_SENSORS; n++) {
    		if (sensors[n] != 0) {
    			sensors[n]->tick();
    		}
    }

    // This shield does not requires fast sampling rates. A global prescaler
    // is used to reduce the sampling rate to a reasonable value (one sampletick = 1s)
    globalPrescaler++;
    if (globalPrescaler == GLOBAL_SAMPLE_PRESCALER_RATIO) {
    		globalPrescaler = 0;

		// Loop on each sensor samplers
		for (unsigned char n = 0; n < NUM_OF_TOTAL_SAMPLERS; n++) {
			if (samplers[n] != 0) {
				result |= samplers[n]->sampleTick();
			}
		}
    }
    

    // Increase the internal timestamp
    timestamp++;
    
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

void SensorsArray::powerUp5V(bool enable) {
	AS_GPIO.digitalWrite(EN_5V, true);
	HAL_Delay(1000);
}

void SensorsArray::powerUp3V3(bool enable) {
	AS_GPIO.digitalWrite(EN_3V3S, true);
}

void SensorsArray::powerUp12V(bool enable) {
	AS_GPIO.digitalWrite(EN_12V, true);
	HAL_Delay(1000);
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
    
    return (averagers[chToSamplerSubChannel[channel].sampler]->init(postscaler) != 0);
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
    
    if (channel < NUM_OF_TOTAL_CHANNELS) {
    		lastSample = averagers[chToSamplerSubChannel[channel].sampler]->lastAveragedValue(chToSamplerSubChannel[channel].subchannel);
        timestamp = averagers[chToSamplerSubChannel[channel].sampler]->lastTimeStamp();
        return true;
    } 
    
    lastSample = 0;
    timestamp = 0;
    return false;
}


bool SensorsArray::saveSensorSerialNumber(unsigned char channel, unsigned char* buffer, unsigned char buffSize) {

	if (channel >= NUM_OF_TOTAL_CHANNELS) {
		return false;
	}

	unsigned short address = SENSOR_SERIAL_NUMBER(channel);
	return EEPROM.write(address, buffer, buffSize);
}

bool SensorsArray::readSensorSerialNumber(unsigned char channel, unsigned char* buffer, unsigned char buffSize) {

	*buffer = 0x00;
	if (channel >= NUM_OF_TOTAL_CHANNELS) {
		return false;
	}

    unsigned short address = SENSOR_SERIAL_NUMBER(channel);
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

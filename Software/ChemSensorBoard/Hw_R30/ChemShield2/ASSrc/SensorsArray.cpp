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
#include "LMP91000Eval.h"
#include "AD5694REval.h"
#include "Sampler.h"
#include "ChemSensorSampler.h"
#include "TempSensorSampler.h"
#include "HumSensorSampler.h"
#include "PressSensorSampler.h"
#include "Persistence.h"
#include "EEPROMHelper.h"
#include <string.h>


#define BOARD_TYPE_CHEMICALSENSORSHIELD		0x01

const ADC16S626 SensorsArray::ADCList[NUM_OF_CHEM_SENSORS] = { ADC16S626(ADC_1_CSPIN), ADC16S626(ADC_2_CSPIN), ADC16S626(ADC_3_CSPIN), ADC16S626(ADC_4_CSPIN) };
SHT31 SensorsArray::sht31i = SHT31(true);
SHT31 SensorsArray::sht31e = SHT31(false);
BMP280 SensorsArray::bmp280 = BMP280();
DitherTool SensorsArray::ditherTool = DitherTool();

SensorsArray::SensorsArray() : samplingEnabled(false), timestamp(0) {
    
    // Initialize the internal coefficients for the pressure sensor
    bmp280.begin();

    // Initialize the humidity sensors
    sht31i.begin();
    sht31e.begin();
    
    // Initialize the samplers and averagers array (yes, I know, this could be skipped but it's done for safety reasons)
    memset(AFEList, 0, sizeof(AFEList));
    memset(DACList, 0, sizeof(DACList));
    memset(samplers, 0, sizeof(samplers));
    memset(averagers, 0, sizeof(averagers));
    
    // Initialize the AFEList
    AFEList[CHEMSENSOR_1] = new LMP91000Eval(AFE_1_ENPIN);
    AFEList[CHEMSENSOR_2] = new LMP91000Eval(AFE_2_ENPIN);
    AFEList[CHEMSENSOR_3] = new LMP91000Eval(AFE_3_ENPIN);
    AFEList[CHEMSENSOR_4] = new LMP91000Eval(AFE_4_ENPIN);

    // Initialize the DACList
    DACList[CHEMSENSOR_1] = new AD5694REval(DAC_1_GAINPIN, AD5694_SLAVE_1);
    DACList[CHEMSENSOR_2] = new AD5694REval(DAC_2_GAINPIN, AD5694_SLAVE_2);
    DACList[CHEMSENSOR_3] = new AD5694REval(DAC_3_GAINPIN, AD5694_SLAVE_3);
    DACList[CHEMSENSOR_4] = new AD5694REval(DAC_4_GAINPIN, AD5694_SLAVE_4);

    // Initialize the sampler units
    samplers[CHEMSENSOR_1] = new ChemSensorSampler(ADCList[CHEMSENSOR_1]);
    samplers[CHEMSENSOR_2] = new ChemSensorSampler(ADCList[CHEMSENSOR_2]);
    samplers[CHEMSENSOR_3] = new ChemSensorSampler(ADCList[CHEMSENSOR_3]);
    samplers[CHEMSENSOR_4] = new ChemSensorSampler(ADCList[CHEMSENSOR_4]);
    samplers[PRESSENSOR_1] = new PressSensorSampler(bmp280);
    samplers[TEMPSENSOR_1] = new TempSensorSampler(sht31e);
    samplers[HUMSENSOR_1] = new HumSensorSampler((TempSensorSampler*)samplers[TEMPSENSOR_1]);
    samplers[TEMPSENSOR_2] = new TempSensorSampler(sht31i);
    samplers[HUMSENSOR_2] = new HumSensorSampler((TempSensorSampler*)samplers[TEMPSENSOR_2]);
    
    // Set the dithering tool. Being a static variable, all object share it so 
    // is possible to initialize only one sampler object.
    samplers[CHEMSENSOR_1]->setDitherTool(&ditherTool);
    
    // Create the averagers
    for (unsigned char n = 0; n < NUM_OF_TOTAL_SENSORS; n++) {
        averagers[n] = new SamplesAverager();
    }
    
    // Set the dithering tool. See the above comment.
    averagers[CHEMSENSOR_1]->setDitherTool(&ditherTool);
    
    // Initialize DACs
    for (unsigned char n = 0; n < NUM_OF_CHEM_SENSORS; n++) {
        DACList[n]->init();
    }
    
    // Initialize by reading the preset stored into the EEPROM
    for (unsigned char n = 0; n < NUM_OF_TOTAL_SENSORS; n++) {
        loadPreset(n);
    }
}

SensorsArray::~SensorsArray() {

	for (unsigned char n = 0; n < NUM_OF_CHEM_SENSORS; n++) {
		if (AFEList[n]) {
			delete AFEList[n];
		}
		if (DACList[n]) {
			delete DACList[n];
		}
	}
    for (unsigned char n = 0; n < NUM_OF_TOTAL_SENSORS; n++) {
        if (samplers[n]) {
            delete samplers[n];
        }
        if (averagers[n]) {
        	delete averagers[n];
        }
    }
}


bool SensorsArray::timerTick() {
    
    bool result = false;

    // Loop on each sensor samplers
    for (unsigned char n = 0; n < NUM_OF_TOTAL_SENSORS; n++) {
        if (samplers[n] != 0) {
            result |= samplers[n]->sampleTick();
        }
    }
    
    // Increase the internal timestamp
    timestamp++;
    
    return result;
}

bool SensorsArray::loop() {

    // If sampling is disabled, skip this function
    if (!samplingEnabled) {
        return false;
    }
    
    // Otherwise loop on each sensor sampler and averager
    bool result = false;
    for (unsigned char n = 0; n < NUM_OF_TOTAL_SENSORS; n++) {
        if (samplers[n] != 0) {
            if (samplers[n]->sampleLoop()) {
                
                // A new sample is ready to be averaged
                result |= averagers[n]->collectSample(samplers[n]->getLastSample(), timestamp);
            };
        }
    }
    
    return result;
}

bool SensorsArray::getIsFlyboardReady() {
	return sht31e.isAvailable();
}

unsigned char SensorsArray::setSamplePrescaler(unsigned char channel, unsigned char prescaler) {
    
    if ((channel >= NUM_OF_TOTAL_SENSORS) || (samplers[channel] == 0))
        return 0;
    
    samplers[channel]->setPreScaler(prescaler);
    
    return 1;
}

bool SensorsArray::getSamplePrescaler(unsigned char channel, unsigned char* prescaler) {
    
    if ((channel >= NUM_OF_TOTAL_SENSORS) || (samplers[channel] == 0))
        return false;
    
    *prescaler = samplers[channel]->getPrescaler();

    return true;
}

unsigned char SensorsArray::setSamplePostscaler(unsigned char channel, unsigned char postscaler) {
    
    if ((channel >= NUM_OF_TOTAL_SENSORS) || (averagers[channel] == 0))
        return 0;
    
    return averagers[channel]->init(postscaler);
}

bool SensorsArray::getSamplePostscaler(unsigned char channel, unsigned char* postscaler) {
    
    if ((channel >= NUM_OF_TOTAL_SENSORS) || (averagers[channel] == 0))
        return false;
    
    *postscaler = averagers[channel]->getBufferSize();

    return true;    
}

unsigned char SensorsArray::setSampleDecimation(unsigned char channel, unsigned char decimation) {
    if ((channel >= NUM_OF_TOTAL_SENSORS) || (samplers[channel] == 0))
        return 0;

    samplers[channel]->setDecimation(decimation);
    return 1;
}

bool SensorsArray::getSampleDecimation(unsigned char channel, unsigned char* decimation) {
    if ((channel >= NUM_OF_TOTAL_SENSORS) || (samplers[channel] == 0))
        return false;

    *decimation = samplers[channel]->getDecimation();
    
    return true;
}

bool SensorsArray::getSampleIIRDenominators(unsigned char channel, unsigned char* iirDen1, unsigned char* iirDen2) {
    if ((channel >= NUM_OF_TOTAL_SENSORS) || (samplers[channel] == 0))
        return false;

    *iirDen1 = samplers[channel]->getIIRDenom(IIR1);
    *iirDen2 = samplers[channel]->getIIRDenom(IIR2);
    
    return true;
}

bool SensorsArray::setSampleIIRDenominators(unsigned char channel, unsigned char iirDen1, unsigned char iirDen2) {
    if ((channel >= NUM_OF_TOTAL_SENSORS) || (samplers[channel] == 0))
        return false;

    samplers[channel]->setIIRDenom(IIR1, iirDen1);
    samplers[channel]->setIIRDenom(IIR2, iirDen2);
    
    return true;
}


bool SensorsArray::enableSampling(bool enable) {

	// Reset all samplers if sampling not already enabled
	if (!samplingEnabled) {
		for (unsigned char n = 0; n < NUM_OF_TOTAL_SENSORS; n++) {
			if (samplers[n] != 0) {
				samplers[n]->onStartSampling();
			}
			if (averagers[n] != 0) {
				averagers[n]->onStartSampling();
			}
		}
	}

	// Enable sampling
    samplingEnabled = enable;
    
    return true;
}

void SensorsArray::inquirySensor(unsigned char channel, unsigned char* buffer, unsigned char bufSize) {

    memset(buffer, 0, bufSize);
    
    if (channel >= NUM_OF_TOTAL_SENSORS)
        return;
    
    samplers[channel]->getChannelName(channel, buffer, bufSize);
}

bool SensorsArray::writeAFERegisters(unsigned char channel, unsigned char tia, unsigned char ref, unsigned char mode) {
    bool result = true;
    
    if (channel > CHEMSENSOR_4)
        return false;

    result &= AFEList[channel]->unLock();
    result &= AFEList[channel]->writeRegisters(tia, ref, mode);
    result &= AFEList[channel]->lock();
    
    return result;
}

bool SensorsArray::readAFERegisters(unsigned char channel, unsigned char* tia, unsigned char* ref, unsigned char* mode) {

    bool result = true;
    
    if (channel > CHEMSENSOR_4)
        return false;

    result = AFEList[channel]->readRegisters(tia, ref, mode);
    
    return result;
}

bool SensorsArray::writeDACRegisters(unsigned char channel, unsigned char subchannel, unsigned short value, bool gain) {
    
    bool result = true;
    
    if (channel > CHEMSENSOR_4)
        return false;

    result = DACList[channel]->writeRegisters(subchannel, value, gain);
    
    return result;
}

bool SensorsArray::readDACRegisters(unsigned char channel, unsigned char subchannel, unsigned short* value, bool* gain) {
    
    bool result = true;
    
    if (channel > CHEMSENSOR_4)
        return false;
    
    result = DACList[channel]->readRegisters(subchannel, value, gain);

    return result;
}

bool SensorsArray::savePreset(unsigned char channel, unsigned char* presetName, unsigned char bufSize) {

    if ((channel >= NUM_OF_TOTAL_SENSORS) || (samplers[channel] == 0) || (averagers[channel] == 0))
        return false;
    
    // Save preset for the AFE and DAC (only for chemical sensors)
    bool result = true;
    if (channel <= CHEMSENSOR_4) {
        result &= DACList[channel]->storePreset();
        result &= AFEList[channel]->storePreset();
    }
    
    // Save channel name
    result &= samplers[channel]->saveChannelName(channel, presetName);

    // Save sampler preset
    result &= samplers[channel]->savePreset(channel);
    
    // Save averager preset
    result &= averagers[channel]->savePreset(channel);
    
    return result;
}

bool SensorsArray::loadPreset(unsigned char channel) {
    
    if ((channel >= NUM_OF_TOTAL_SENSORS) || (samplers[channel] == 0) || (averagers[channel] == 0))
        return false;

    // Read AFE and DAC preset (only for chemical sensors)
    bool result = true;
    if (channel <= CHEMSENSOR_4) {
        result &= DACList[channel]->loadPreset();
        result &= AFEList[channel]->loadPreset();
    }
    
    // Read sampler preset
    result &= samplers[channel]->loadPreset(channel);
    
    // Read averager preset
    result &= averagers[channel]->loadPreset(channel);
    
    return result;
}

bool SensorsArray::getLastSample(unsigned char channel, unsigned short& lastSample, unsigned long& timestamp) {

    lastSample = 0;
    timestamp = 0;

    if ((channel < NUM_OF_TOTAL_SENSORS) && (averagers[channel])) {

    	unsigned char enabled = false;
    	samplers[channel]->getChannelIsEnabled(&enabled);

    	if (enabled != 0) {
			lastSample = averagers[channel]->lastAveragedValue();
			timestamp = averagers[channel]->lastTimeStamp();
    	}

		// Only for chemical sensors, convert back to two complement binary format
		if (channel <= CHEMSENSOR_4) {
			lastSample = twoComplement(lastSample);
		}

		return true;

    } 
    
    return false;
}

bool SensorsArray::getLastSample(unsigned char channel, float &lastSample, unsigned long &timestamp) {

    lastSample = 0.0f;
    timestamp = 0;

    if ((channel < NUM_OF_TOTAL_SENSORS) && (averagers[channel])) {

    	unsigned char enabled = false;
    	samplers[channel]->getChannelIsEnabled(&enabled);

    	if (enabled != 0) {

			// Only for chemical sensors, convert to nA
			if (channel <= CHEMSENSOR_4) {

				lastSample = averagers[channel]->lastAveragedValue();

				double vRefm = DACList[channel]->getChannelVoltage(CHANNEL_DAC_REFM);
				double vRefAD = DACList[channel]->getChannelVoltage(CHANNEL_DAC_REFAD);
				bool afeRifInternal = AFEList[channel]->getIntSource();
				double vRefAFE = (afeRifInternal)? 5.0 : DACList[channel]->getChannelVoltage(CHANNEL_DAC_REFAFE);
				double intZeroVoltage = vRefAFE*(((double)AFEList[channel]->getIntZero())/100.0);
				double absSampleVoltage = ADCList[channel].getVoltage(lastSample, vRefm, vRefAD);
				double gain = AFEList[channel]->getGain();

				// Get sample Voltage relative to the AFE internal zero
				double sampleVoltage = absSampleVoltage - intZeroVoltage;

				// Convert to current
				double sampleCurrent = (sampleVoltage / gain);

				// Take nA to be sent over the wire
				lastSample = (float) sampleCurrent * 1e9;
			} else {

				lastSample = (float) samplers[channel]->evaluateMeasurement(averagers[channel]->lastAveragedValue());
			}

			timestamp = averagers[channel]->lastTimeStamp();
    	}
		return true;
    }

    return false;
}

bool SensorsArray::saveSensorSerialNumber(unsigned char channel, unsigned char* buffer, unsigned char buffSize) {

	// Serial is available only for chemical sensors
	if (channel > CHEMSENSOR_4) {
		return false;
	}

	// Safety check of incoming data
	unsigned char maxSize = (buffSize < SERIAL_NUMBER_MAXLENGTH)? buffSize : SERIAL_NUMBER_MAXLENGTH;
	buffer[SERIAL_NUMBER_MAXLENGTH-1] = 0;

	unsigned short address = SENSOR_SERIAL_NUMBER(channel);
	return EEPROM.write(address, buffer, maxSize);
}

bool SensorsArray::readSensorSerialNumber(unsigned char channel, unsigned char* buffer, unsigned char buffSize) {

	*buffer = 0x00;
	if (channel >= NUM_OF_TOTAL_SENSORS) {
		return false;
	}

	// Serial is available only for chemical sensors
	if (channel > CHEMSENSOR_4) {
		return true;
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

	// Safety check of incoming data
	unsigned char maxSize = (buffSize < SERIAL_NUMBER_MAXLENGTH)? buffSize : SERIAL_NUMBER_MAXLENGTH;
	buffer[SERIAL_NUMBER_MAXLENGTH-1] = 0;

	unsigned short address = BOARD_SERIAL_NUMBER;
	return EEPROM.write(address, buffer, maxSize);
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

    if ((channel >= NUM_OF_TOTAL_SENSORS) || (samplers[channel] == 0) || (averagers[channel] == 0))
        return false;

    if (samplePeriod == 0) {
    	return false;
    }

	*samplePeriod = (averagers[channel]->getBufferSize() + 1) *
					(samplers[channel]->getDecimation() + 1) *
					(samplers[channel]->getPrescaler() + 1) * 10;

	return true;
}

bool SensorsArray::getUnitForChannel(unsigned char channel, unsigned char* buffer, unsigned char buffSize) {

    if ((channel >= NUM_OF_TOTAL_SENSORS) || (samplers[channel] == 0))
        return false;

    strcpy((char*)buffer, samplers[channel]->getMeasurementUnit());

	return true;
}

bool SensorsArray::setEnableChannel(unsigned char channel, unsigned char enabled) {

	if (channel >= NUM_OF_TOTAL_SENSORS) {
		return false;
	}

	// Enabled status is known by it's sampler
	return samplers[channel]->setEnableChannel(enabled);
}

bool SensorsArray::getChannelIsEnabled(unsigned char channel, unsigned char *enabled) {

	if (channel >= NUM_OF_TOTAL_SENSORS) {
		return false;
	}

	// Enabled status is known by it's sampler
	return samplers[channel]->getChannelIsEnabled(enabled);
}


unsigned short SensorsArray::getBoardType() {
	return BOARD_TYPE_CHEMICALSENSORSHIELD;
}

unsigned short SensorsArray::getBoardNumChannels() {
	return NUM_OF_TOTAL_SENSORS;
}

unsigned short SensorsArray::twoComplement(unsigned short sample) {

    // in       ->      out
    // 0        ->      32768
    // 32767    ->      65535
    // 32768    ->      0
    // 65535    ->      32767

    // 2 complement conversion
    if (sample > 32767) {
            sample = sample - 32768;
    } else {
            sample = sample + 32768;
    }
    return sample;
}


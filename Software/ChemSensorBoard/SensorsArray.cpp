/* ========================================================================
 * Copyright 2015 EUROPEAN UNION
 *
 * Licensed under the EUPL, Version 1.1 or - as soon they will be approved by 
 * the European Commission - subsequent versions of the EUPL (the "Licence"); 
 * You may not use this work except in compliance with the Licence. 
 * You may obtain a copy of the Licence at: http://ec.europa.eu/idabc/eupl
 * Unless required by applicable law or agreed to in writing, software distributed 
 * under the Licence is distributed on an "AS IS" basis, WITHOUT WARRANTIES OR 
 * CONDITIONS OF ANY KIND, either express or implied. See the Licence for the 
 * specific language governing permissions and limitations under the Licence.
 * Date: 02/04/2015
 * Authors
 * - Michel Gerboles  - michel.gerboles@jrc.ec.europa.eu,  
 *                     European Commission - Joint Research Centre, 
 * - Laurent Spinelle - laurent.spinelle@jrc.ec.europa.eu,
 *                     European Commission - Joint Research Centre, 
 * - Marco Signorini  - marco.signorini@liberaintentio.com
 * 
 * ======================================================================== 
 */

#include <Arduino.h>
#include "SensorsArray.h"
#include "Sampler.h"
#include "ChemSensorSampler.h"
#include "TempSensorSampler.h"
#include "HumSensorSampler.h"
#include "PressSensorSampler.h"
#include "Persistence.h"

const LMP91000 SensorsArray::AFEList[NUM_OF_CHEM_SENSORS] = { LMP91000(AFE_1_ENPIN), LMP91000(AFE_2_ENPIN), LMP91000(AFE_3_ENPIN), LMP91000(AFE_4_ENPIN) };
const ADC16S626 SensorsArray::ADCList[NUM_OF_CHEM_SENSORS] = { ADC16S626(ADC_1_CSPIN), ADC16S626(ADC_2_CSPIN), ADC16S626(ADC_3_CSPIN), ADC16S626(ADC_4_CSPIN) };
const AD5694R SensorsArray::DACList[NUM_OF_CHEM_SENSORS] = { AD5694R(DAC_1_GAINPIN, AD5694_SLAVE_1), AD5694R(DAC_2_GAINPIN, AD5694_SLAVE_2), AD5694R(DAC_3_GAINPIN, AD5694_SLAVE_3), AD5694R(DAC_4_GAINPIN, AD5694_SLAVE_4) };
UR100CD SensorsArray::ur100cd = UR100CD();
SFE_BMP180 SensorsArray::bmp180 = SFE_BMP180();
DitherTool SensorsArray::ditherTool = DitherTool();

SensorsArray::SensorsArray() : samplingEnabled(false), timestamp(0) {
    
    // Initialize the internal coefficients for the pressure sensor
    bmp180.begin();
    
    // Initialize the samplers and averagers array (yes, I know, this could be skipped but it's done for safety reasons)
    memset(samplers, 0, sizeof(samplers));
    memset(averagers, 0, sizeof(averagers));
    
    // Initialize the sampler units
    samplers[CHEMSENSOR_1] = new ChemSensorSampler(ADCList[CHEMSENSOR_1]);
    samplers[CHEMSENSOR_2] = new ChemSensorSampler(ADCList[CHEMSENSOR_2]);
    samplers[CHEMSENSOR_3] = new ChemSensorSampler(ADCList[CHEMSENSOR_3]);
    samplers[CHEMSENSOR_4] = new ChemSensorSampler(ADCList[CHEMSENSOR_4]);
    samplers[TEMPSENSOR_1] = new TempSensorSampler(ur100cd);
    samplers[HUMSENSOR_1] = new HumSensorSampler((TempSensorSampler*)samplers[TEMPSENSOR_1]);
    samplers[PRESSENSOR_1] = new PressSensorSampler(bmp180);
    
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
        DACList[n].init();
    }
    
    // Initialize by reading the preset stored into the EEPROM
    for (unsigned char n = 0; n < NUM_OF_TOTAL_SENSORS; n++) {
        loadPreset(n);
    }
}

SensorsArray::~SensorsArray() {
    for (unsigned char n = 0; n < NUM_OF_TOTAL_SENSORS; n++) {
        if (samplers[n]) {
            delete samplers[n];
        }
    }
}


const LMP91000* SensorsArray::getAFE(unsigned char channel) {
    return AFEList + channel;
}

const ADC16S626* SensorsArray::getADC(unsigned char channel) {
    return ADCList + channel;
}

const AD5694R* SensorsArray::getDAC(unsigned char channel) {
    return DACList + channel;
}

UR100CD* SensorsArray::getUR100(unsigned char channel) {
    return &ur100cd;
}

SFE_BMP180* SensorsArray::getBMP180(unsigned char channel) {
    return &bmp180;
}

bool SensorsArray::timeTick() {
    
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
    samplingEnabled = enable;
    
    return true;
}

void SensorsArray::inquirySensor(unsigned char channel, unsigned char* buffer, unsigned char bufSize) {

    memset(buffer, 0, bufSize);
    
    if (channel >= NUM_OF_TOTAL_SENSORS)
        return;
    
    if (channel <= CHEMSENSOR_4) {
        AFEList[channel].getPresetName(buffer);
    } else if (channel == PRESSENSOR_1) {
        strcpy((char*)buffer, "BMP180");
    } else if ((channel == TEMPSENSOR_1) || (channel == HUMSENSOR_1)) {
        strcpy((char*)buffer, "UR100CD");
    }
}

bool SensorsArray::writeAFERegisters(unsigned char channel, unsigned char tia, unsigned char ref, unsigned char mode) {
    bool result = true;
    
    if (channel > CHEMSENSOR_4)
        return false;

    result &= AFEList[channel].unLock();
    result &= AFEList[channel].writeRegisters(tia, ref, mode);
    result &= AFEList[channel].lock();
    
    return result;
}

bool SensorsArray::readAFERegisters(unsigned char channel, unsigned char* tia, unsigned char* ref, unsigned char* mode) {

    bool result = true;
    
    if (channel > CHEMSENSOR_4)
        return false;

    result = AFEList[channel].readRegisters(tia, ref, mode);
    
    return result;
}

bool SensorsArray::writeDACRegisters(unsigned char channel, unsigned char subchannel, unsigned short value, bool gain) {
    
    bool result = true;
    
    if (channel > CHEMSENSOR_4)
        return false;

    result = DACList[channel].writeRegisters(subchannel, value, gain);
    
    return result;
}

bool SensorsArray::readDACRegisters(unsigned char channel, unsigned char subchannel, unsigned short* value, bool* gain) {
    
    bool result = true;
    
    if (channel > CHEMSENSOR_4)
        return false;
    
    result = DACList[channel].readRegisters(subchannel, value, gain);

    return result;
}

bool SensorsArray::savePreset(unsigned char channel, unsigned char* presetName, unsigned char bufSize) {

    if ((channel >= NUM_OF_TOTAL_SENSORS) || (samplers[channel] == 0) || (averagers[channel] == 0))
        return false;
    
    // Save preset for the AFE and DAC (only for chemical sensors)
    bool result = true;
    if (channel <= CHEMSENSOR_4) {
        result &= DACList[channel].storePreset();
        result &= AFEList[channel].storePreset(presetName);        
    }
    
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
        result &= DACList[channel].loadPreset();
        result &= AFEList[channel].loadPreset();        
    }
    
    // Read sampler preset
    result &= samplers[channel]->loadPreset(channel);
    
    // Read averager preset
    result &= averagers[channel]->loadPreset(channel);
    
    return result;
}

bool SensorsArray::getLastSample(unsigned char channel, unsigned short& lastSample, unsigned long& timestamp) {
    
    if ((channel < NUM_OF_TOTAL_SENSORS) && (averagers[channel])) {
        lastSample = averagers[channel]->lastAveragedValue();
        
        // Only for chemical sensors, convert back to two complement binary format
        if (channel <= CHEMSENSOR_4) {
            lastSample = twoComplement(lastSample);
        }
        
        timestamp = averagers[channel]->lastTimeStamp();
        return true;
    } 
    
    lastSample = 0;
    timestamp = 0;
    return false;
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

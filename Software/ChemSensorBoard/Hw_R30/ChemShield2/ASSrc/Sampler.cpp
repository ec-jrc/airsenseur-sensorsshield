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


#include "DitherTool.h"
#include "Sampler.h"
#include "Persistence.h"
#include "EEPROMHelper.h"

#define DEFAULT_BLANK_TIMER_PERIOD 5

DitherTool* Sampler::ditherTool = (DitherTool*)0x00;

Sampler::Sampler() {
    go = false; 
    prescaler = 0;
    timer = 0;
    enabled = true;
    decimation = 0; 
    decimationTimer = 0;
    blankTimer = DEFAULT_BLANK_TIMER_PERIOD;
    workSample = 0; 
    lastSample = 0;
    iIRDenum[0] = 0;
    iIRDenum[1] = 0;
    iIRAccumulator[0] = 0;
    iIRAccumulator[1] = 0;
}

void Sampler::onStartSampling() {
	timer = 0;
	decimationTimer = 0;
	blankTimer = DEFAULT_BLANK_TIMER_PERIOD;
    workSample = 0;
    lastSample = 0;
    iIRAccumulator[0] = 0;
    iIRAccumulator[1] = 0;
}

void Sampler::setPreScaler(unsigned char value) {
    prescaler = value;
    timer = 0;
}

unsigned char Sampler::getPrescaler() {
    return prescaler;
}

void Sampler::setDecimation(unsigned char value) {
    decimation = value; 
    decimationTimer = 0;
    blankTimer = DEFAULT_BLANK_TIMER_PERIOD;
}

unsigned char Sampler::getDecimation() {
    return decimation;
}

unsigned short Sampler::getLastSample() {
    return lastSample;
}

bool Sampler::applyDecimationFilter() {
    
    if (decimationTimer == decimation) {
        decimationTimer = 0;

        // Update the lastSample only after
        // elapsed blank period
        if (blankTimer != 0) {
          blankTimer--;
        } else {
          lastSample = workSample;
        }
    } else {
      decimationTimer++;
    }

    return ((decimationTimer == 0) || (blankTimer != 0));
}

unsigned char Sampler::getIIRDenom(unsigned char iIRID) {

    if (iIRID > 1)
        return 0;
    
    return iIRDenum[iIRID];
}

void Sampler::setIIRDenom(unsigned char iIRID, unsigned char value) {

    if (iIRID > 1)
        return;
    
    if (value == 0xFF)
        value = 0;
    
    iIRDenum[iIRID] = value;
    iIRAccumulator[iIRID] = 0.0f;
}

void Sampler::setDitherTool(DitherTool* tool) {
    ditherTool = tool;
}

// Apply the IIR filter. Input and Output are from/to workSample
void Sampler::applyIIRFilter(unsigned char iiRID) {

    if (iiRID > 1)
        return;
    
    unsigned char *denom = iIRDenum + iiRID;
    
    // The filter is disabled if denominator == 0
    if (*denom == 0) {
        return;
    }
    
    double* S = iIRAccumulator + iiRID;
    
    // S(n) = S(n-1) + 1/den * (I(n) - S(n-1))
    *S = *S + ((double)workSample - *S)/(*denom);
    
    workSample = (unsigned short)(ditherTool->applyDithering(*S));
}

void Sampler::onReadSample(unsigned short newSample) {
    lastSample = newSample;
    workSample = newSample;
}

bool Sampler::loadPreset(unsigned char myID) {

    // Load channel persisted parameters
    unsigned char prescVal = EEPROM.read(SAMPLER_PRESET_PRESCALER(myID));
    unsigned char decimVal = EEPROM.read(SAMPLER_PRESET_DECIMATION(myID));
    unsigned char iir1Denom = EEPROM.read(SAMPLER_PRESET_IIR1DENOM(myID));
    unsigned char iiR2Denom = EEPROM.read(SAMPLER_PRESET_IIR2DENOM(myID));
    unsigned char enabled = EEPROM.read(SAMPLER_PRESET_CHENABLED(myID));

    // Apply
    setPreScaler(prescVal);
    setDecimation(decimVal);
    setIIRDenom(0, iir1Denom);
    setIIRDenom(1, iiR2Denom);
    setEnableChannel(enabled);
    
    return true;
}

bool Sampler::savePreset(unsigned char myID) {

    // Store channel configuration values into EEPROM
    EEPROM.write(SAMPLER_PRESET_PRESCALER(myID), getPrescaler());
    EEPROM.write(SAMPLER_PRESET_DECIMATION(myID), getDecimation());
    EEPROM.write(SAMPLER_PRESET_IIR1DENOM(myID), getIIRDenom(0));
    EEPROM.write(SAMPLER_PRESET_IIR2DENOM(myID), getIIRDenom(1));
    EEPROM.write(SAMPLER_PRESET_CHENABLED(myID), (enabled)?1:0);
    
    return true;
}

bool Sampler::saveChannelName(unsigned char myID, unsigned char* name) {

	// Always mark the end of the buffer with 0 to prevent overflow when reading
	name[SENSOR_NAME_LENGTH-1] = 0;

    // Store the preset name
    unsigned short address = SENSOR_NAME(myID);
    EEPROM.write(address, name, SENSOR_NAME_LENGTH);

    return true;
}

bool Sampler::getChannelName(unsigned char myID, unsigned char* buffer, unsigned char buffSize) const {

    unsigned short address = SENSOR_NAME(myID);
    unsigned char maxSize = (buffSize < SENSOR_NAME_LENGTH)? buffSize : SENSOR_NAME_LENGTH;
    for (unsigned char n = 0; n < maxSize; n++) {
        unsigned char ucRead = EEPROM.read(address);
        if (ucRead == 0xFF) {
            ucRead = 0;
        }
        buffer[n] = ucRead;
        address++;
        if (ucRead == 0) {
        	break;
        }
    }

    // Always mark the end of the buffer to prevent overflow
    buffer[maxSize] = 0;

    return true;
}

bool Sampler::setEnableChannel(unsigned char _enabled) {
	enabled = (_enabled != 0x00);

	return true;
}

bool Sampler::getChannelIsEnabled(unsigned char* _enabled) {

	*_enabled = (enabled)? 1:0;

	return true;
}

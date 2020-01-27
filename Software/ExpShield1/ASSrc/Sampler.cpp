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


#include "Sampler.h"
#include "Persistence.h"
#include "EEPROMHelper.h"
#include "SensorDevice.h"
#include <string.h>

Sampler::Sampler(unsigned char channels, SensorDevice* _sensor)
		: go(false), prescaler(0), timer(0), decimation(0), decimationTimer(0), numChannels(channels), sensor(_sensor) {

	lastSample = new unsigned short [channels];
	memset(lastSample, 0, channels*sizeof(unsigned short));
	enabled = new bool[channels];
	memset(enabled, 0xff, channels*sizeof(bool));
}

const unsigned char Sampler::getNumChannels() const {
	return numChannels;
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
}

unsigned char Sampler::getDecimation() {
    return decimation;
}

unsigned short Sampler::getLastSample(unsigned char channel) {

	if (channel >= numChannels)
		return 0;

    return lastSample[channel];
}

bool Sampler::applyDecimationFilter() {
    
    if (decimationTimer == decimation) {
        decimationTimer = 0;

    } else {
      decimationTimer++;
    }

    return (decimationTimer == 0);
}

void Sampler::onReadSample(unsigned char channel, unsigned short newSample) {

	if (channel < numChannels) {
		lastSample[channel] = newSample;
	}
}

bool Sampler::loadPreset(unsigned char myID) {

    // Load prescaler and decimation
    unsigned char prescVal = EEPROM.read(SAMPLER_PRESET_PRESCALER(myID));
    unsigned char decimVal = EEPROM.read(SAMPLER_PRESET_DECIMATION(myID));

    // Apply
    setPreScaler(prescVal);
    setDecimation(decimVal);

    // Load channel enable status
    for (int channel = 0; channel < numChannels; channel++) {
    	unsigned char read = EEPROM.read(SAMPLER_CHANNEL_ENABLED_PRESET(myID, channel));

    	// Apply
    	setEnableChannel(channel, read);
    }

    return true;
}

bool Sampler::savePreset(unsigned char myID) {

    // Get values and store them
    EEPROM.write(SAMPLER_PRESET_PRESCALER(myID), getPrescaler());
    EEPROM.write(SAMPLER_PRESET_DECIMATION(myID), getDecimation());
    
    // Save channel enable status
    EEPROM.write(SAMPLER_CHANNEL_ENABLED_PRESET(myID, 0), (unsigned char*)enabled, numChannels);

    return true;
}

void Sampler::onStartSampling() {

	if (atLeastOneChannelEnabled()) {
		sensor->onStartSampling();
	}
}

void Sampler::onStopSampling() {
	sensor->onStopSampling();
}

bool Sampler::sampleTick() {

    if (timer == prescaler) {

        // It's time for a new sample
    	if (atLeastOneChannelEnabled()) {
			sensor->triggerSample();

			timer = 0;
			go = true;
			return true;
    	}
    }
    timer++;

    return false;
}

bool Sampler::sampleLoop() {

    // Take the new samples
    if (go && sensor->sampleAvailable()) {
    		go = false;

    		for (unsigned char n = 0; n < numChannels; n++) {
    			onReadSample(n, sensor->getSample(n));
    		}

        // Apply the decimation filter
        return applyDecimationFilter();
    }

    return false;
}

void Sampler::setLowPowerMode(bool lowPower) {
}

bool Sampler::setEnableChannel(unsigned char channel, unsigned char _enabled) {
	if (channel >= numChannels)
		return false;

	enabled[channel] = (_enabled != 0);

	return true;
}

bool Sampler::getChannelIsEnabled(unsigned char channel, unsigned char* _enabled) {
	if (channel >= numChannels)
		return false;

	*_enabled = (enabled[channel])? 1:0;

	return true;
}

SensorDevice* Sampler::getSensor() {
	return sensor;
}

bool Sampler::atLeastOneChannelEnabled() {

	bool result = false;
	for (unsigned char channel = 0; (channel < numChannels) & !result; channel++) {
		result |= enabled[channel];
	}

	return result;
}

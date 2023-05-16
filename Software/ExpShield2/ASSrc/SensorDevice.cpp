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

#include "SensorDevice.h"
#include <string.h>

SensorDevice::SensorDevice(const unsigned char reqChannels) : sampleReady(false), numChannels(reqChannels) {

	lastSamples = new unsigned short[reqChannels];
}

SensorDevice::~SensorDevice() {

	delete[] lastSamples;
}

void SensorDevice::onStartSampling() {

	memset(lastSamples, 0x00, numChannels*sizeof(unsigned short));
	sampleReady = false;
}

const unsigned char SensorDevice::getNumChannels() const {
	return numChannels;
}

// To be overridden only by devices allowing for setpoint
bool SensorDevice::setSetpointForChannel(unsigned char channel, unsigned short setpoint) {
	return false;
}

// To be overridden only by devices allowing for setpoint
bool SensorDevice::getSetpointForChannel(unsigned char channel, unsigned short& setpoint) {
	return false;
}

// To be overridden only by devices allowing for writing a generic register/address
bool SensorDevice::writeGenericRegister(unsigned int address, unsigned int value) {
	return false;
}

// To be overridden only by devices allowing for reading a generic register/address
bool SensorDevice::readGenericRegister(unsigned int address, unsigned int& value) {
	return false;
}

void SensorDevice::triggerSample() {
	sampleReady = false;
}

bool SensorDevice::sampleAvailable() {
	return sampleReady;
}

unsigned short SensorDevice::getSample(unsigned char channel) {

	if (channel < numChannels) {

		// As soon as the latest channel has been read, mark all samples as
		// not available anymore
		if (channel == (numChannels-1)) {
			sampleReady = false;
		}

		return lastSamples[channel];
	}

	return 0x0000;
}

void SensorDevice::setSample(unsigned char channel, unsigned short sample) {

	if (channel <= numChannels) {
		lastSamples[channel] = sample;
		sampleReady = true;
	}
}

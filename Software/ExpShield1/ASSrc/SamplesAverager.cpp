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
 
#include "SamplesAverager.h"
#include "Persistence.h"
#include "DitherTool.h"
#include "EEPROMHelper.h"
#include <string.h>

DitherTool* SamplesAverager::ditherTool = (DitherTool*)0x00;

SamplesAverager::SamplesAverager(const unsigned char _channels) : channels(_channels) {

	accumulators = new unsigned long[channels];
	sampleOffsets = new unsigned char[channels];
	lastAverageSamples = new unsigned short[channels];

    reset();
}

SamplesAverager::~SamplesAverager() {
    if (dataBuffer) {
        delete[] dataBuffer;
    }
    if (accumulators) {
    		delete[] accumulators;
    }
    if (sampleOffsets) {
    		delete[] sampleOffsets;
    }
    if (lastAverageSamples) {
    		delete[] lastAverageSamples;
    }
}

// We expect accumulators and channels already allocated/valid
void SamplesAverager::reset() {

    timestamp = 0;    
    bufferSize = 0;
    dataBuffer = 0;
    consolidated = false;

    memset(sampleOffsets, 0, channels*sizeof(unsigned char));
    memset(lastAverageSamples, 0, channels*sizeof(unsigned short));
    memset(accumulators, 0, channels*sizeof(unsigned long));
}

unsigned char SamplesAverager::init(unsigned char size) {

    if (dataBuffer) {
        delete[] dataBuffer;
    }

    if (size != 0) {
		unsigned short overallBufferSize = size * channels;

		reset();
		dataBuffer = new unsigned short [overallBufferSize];
		if (dataBuffer) {
			bufferSize = size;
			memset(dataBuffer, 0, overallBufferSize*sizeof(unsigned short));
		}
    } else {
    		bufferSize = 0;
    }
    
    // Return buffersize...
    if (bufferSize) {
    	return bufferSize;
    }

    // or at least 1 (when average is disabled)
    return 1;
}

// The moving average is calculated each sample but is latched at each buffer 
// completion. This is for speed optimization (but requires an unsigned long accumulator for each channel)
bool SamplesAverager::collectSample(unsigned char channel, unsigned short sample, unsigned long _timestamp) {
    
    if (channel >= channels)
        return false;

    unsigned char* sampleOffset = sampleOffsets+channel;
    unsigned long* accumulator = accumulators+channel;
    unsigned short* lastAverageSample = lastAverageSamples+channel;

    if (bufferSize == 0) {
		*lastAverageSample = sample;
		*accumulator = sample;
		timestamp = _timestamp;
		return true;
    }

    if (!dataBuffer) {
    		return false;
    }

    // Remove the old sample from the accumulator
    *accumulator = *accumulator - dataBuffer[(channel*bufferSize) + *sampleOffset];
      
    // Add the new sample to the accumulator
    *accumulator = *accumulator + sample;
    
    // Store the new sample into the buffer
    dataBuffer[(channel*bufferSize) + *sampleOffset] = sample;
    (*sampleOffset)++;
    
    if (*sampleOffset == bufferSize) {
        *sampleOffset = 0;
		timestamp = _timestamp;
		consolidated = true;

        *lastAverageSample = (unsigned short)ditherTool->applyDithering(((double)(*accumulator))/bufferSize);
        return true;
    }

    // Call for a dithering reseed. This will be done
    // only once even if called several times
    ditherTool->reSeed(sample);

    // We prefer to send back a set of unfiltered samples instead of zero
    // for the very beginning of the filter's life. 
    if (!consolidated) {
      *lastAverageSample = sample;
      timestamp = _timestamp;
      return true;
    }
    
    return false;
}

unsigned char SamplesAverager::getBufferSize() {
	return bufferSize;
}

void SamplesAverager::setDitherTool(DitherTool* tool) {
    ditherTool = tool;
}

unsigned short SamplesAverager::lastAveragedValue(unsigned char channel) {
    return lastAverageSamples[channel];
}

unsigned long SamplesAverager::lastTimeStamp() {
    return timestamp;
}

unsigned long* SamplesAverager::getAccumulators() const {
	return accumulators;
}

bool SamplesAverager::loadPreset(unsigned char myID) {

    // Read the buffer size
    unsigned char bufSize = EEPROM.read(AVERAGER_PRESET_BUFSIZE(myID));

    // Apply (only if the EEPROM contains a valid value)
    if (bufSize != 0xFF) {
        return (init(bufSize) != 0);
    }

    return true;
}

bool SamplesAverager::savePreset(unsigned char myID) {

    // Store the buffer size
    EEPROM.write(AVERAGER_PRESET_BUFSIZE(myID), getBufferSize());
    
    return true;
}

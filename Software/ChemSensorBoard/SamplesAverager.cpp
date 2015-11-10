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
#include <EEPROM.h>
#include "SamplesAverager.h"
#include "Persistence.h"
#include "DitherTool.h"

DitherTool* SamplesAverager::ditherTool = (DitherTool*)0x00;

SamplesAverager::SamplesAverager() : timestamp(0) {
    reset();
}

SamplesAverager::~SamplesAverager() {
    if (dataBuffer) {
        free(dataBuffer);
    }
}

void SamplesAverager::reset() {
        
    bufferSize = 0;
    sampleOffset = 0;
    dataBuffer = 0;
    accumulator = 0;
    lastAverageSample = 0;
}

unsigned char SamplesAverager::init(unsigned char size) {

    if (dataBuffer) {
        free(dataBuffer);
    }
    
    // The buffer should be 1 byte more than what's requested.
    // We need to store at least one sample even if we don't
    // need any average
    size = size+1;
    
    reset();
    dataBuffer = (unsigned short*)malloc(size*sizeof(unsigned short));
    if (dataBuffer) {
        bufferSize = size;
        memset(dataBuffer, 0, size*sizeof(unsigned short));
    }
    
    return bufferSize;
}

// The moving average is calculated each sample but is latched at each buffer 
// completion. This is for speed optimization (but requires an unsigned long accumulator)
bool SamplesAverager::collectSample(unsigned short sample, unsigned long _timestamp) {
    
    if (!dataBuffer)
        return false;
                    
    // Remove the old sample from the accumulator
    accumulator = accumulator - dataBuffer[sampleOffset];
      
    // Add the new sample to the accumulator
    accumulator = accumulator + sample;
    
    // Store the new sample into the buffer
    dataBuffer[sampleOffset] = sample;
    sampleOffset++;
    
    if (sampleOffset == bufferSize) {
        sampleOffset = 0;
        timestamp = _timestamp;
        
        lastAverageSample = (unsigned short)ditherTool->applyDithering(((double)accumulator)/bufferSize);
        return true;
    }
    
    // Call for a dithering reseed. This will be done
    // only once even if called several times
    ditherTool->reSeed(sample);
    
    return false;
}

unsigned char SamplesAverager::getBufferSize() {
    if (bufferSize == 0)
        return bufferSize;
    
    return bufferSize - 1;
}

void SamplesAverager::setDitherTool(DitherTool* tool) {
    ditherTool = tool;
}

unsigned short SamplesAverager::lastAveragedValue() {
    return lastAverageSample;
}

unsigned long SamplesAverager::lastTimeStamp() {
    return timestamp;
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

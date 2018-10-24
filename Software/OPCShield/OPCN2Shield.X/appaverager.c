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

#include "mcc_generated_files/mcc.h"
#include "appaverager.h"

// Internal function prototypes
static void reset(app_averagerdata* averagerData);

void APP_Avg_Init(app_averagerdata* averagerData, float* databuffer, uint8_t bufferSize) {
    
    averagerData->maxBufferSize = bufferSize;
    averagerData->dataBuffer = databuffer;
    
    reset(averagerData);
};

bool APP_Avg_SetBufferDeep(app_averagerdata* averagerData, uint8_t bufferDeep) {
    
    if (!averagerData || !averagerData->dataBuffer || (bufferDeep >= averagerData->maxBufferSize)) {
        return false;
    }
    
    reset(averagerData);
    averagerData->currentBufferSize = bufferDeep;
    
    return true;
}

// The moving average is calculated each sample but is latched at each buffer 
// completion. This is for speed optimization 
bool APP_Avg_CollectSample(app_averagerdata* averagerData, float sample, unsigned long _timestamp) {
    
    if (!averagerData || !averagerData->dataBuffer) {
        return false;
    }
    
    // Flat averagers
    if (averagerData->currentBufferSize == 0) {
        averagerData->timestamp = _timestamp;
        averagerData->lastAverageSample = sample;
        averagerData->accumulator = sample;
        return true;
    }
                    
    // Remove the old sample from the accumulator
    averagerData->accumulator = averagerData->accumulator - averagerData->dataBuffer[averagerData->sampleOffset];
      
    // Add the new sample to the accumulator
    averagerData->accumulator = averagerData->accumulator + sample;
    
    // Store the new sample into the buffer
    averagerData->dataBuffer[averagerData->sampleOffset] = sample;
    averagerData->sampleOffset++;
    
    if (averagerData->sampleOffset == averagerData->currentBufferSize) {
        averagerData->sampleOffset = 0;
        averagerData->timestamp = _timestamp;
        averagerData->consolidated = true;
        
        averagerData->lastAverageSample = averagerData->accumulator/averagerData->currentBufferSize;
        return true;
    }

    // We prefer to send back a set of unfiltered samples instead of zero
    // for the very beginning of the filter's life. 
    if (!averagerData->consolidated) {
      averagerData->lastAverageSample = sample;
      averagerData->timestamp = _timestamp;
      return true;
    }
    
    return false;    
}


uint8_t APP_Avg_GetBufferDeep(app_averagerdata* averagerData) {
    if (!averagerData || !averagerData->dataBuffer) {
        return 0;
    }
    
    if (averagerData->currentBufferSize == 0)
        return averagerData->currentBufferSize;
    
    return averagerData->currentBufferSize;
    
}
float APP_Avg_LastAveragedValue(app_averagerdata* averagerData) {
    if (!averagerData || !averagerData->dataBuffer) {
        return 0;
    }
    return averagerData->lastAverageSample;
}

float APP_Avg_LastCumulatedValue(app_averagerdata* averagerData) {
    if (!averagerData || !averagerData->dataBuffer) {
        return 0;
    }

    return averagerData->accumulator;
}

uint32_t APP_Avg_LastTimeStamp(app_averagerdata* averagerData) {
    if (!averagerData || !averagerData->dataBuffer) {
        return 0;
    }
    return averagerData->timestamp;
}

static void reset(app_averagerdata* averagerData) {
    
    if (!averagerData || !averagerData->dataBuffer) {
        return;
    }
    
    averagerData->currentBufferSize = 0;    
    averagerData->accumulator = 0;
    averagerData->consolidated = false;
    averagerData->lastAverageSample = 0;
    averagerData->sampleOffset = 0;
    averagerData->timestamp = 0;
    
    for (uint8_t n = 0; n < averagerData->maxBufferSize; n++) {
        averagerData->dataBuffer[n] = 0.0f;
    }
}

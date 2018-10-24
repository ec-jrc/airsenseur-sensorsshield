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

#include "HumSensorSampler.h"
#include "TempSensorSampler.h"

HumSensorSampler::HumSensorSampler(TempSensorSampler* temperatureSampler) 
                : tempSampler(temperatureSampler) {
}

HumSensorSampler::~HumSensorSampler() {
}

bool HumSensorSampler::sampleTick() {
    return false;
}

bool HumSensorSampler::sampleLoop() {
    if (tempSampler && tempSampler->getHumiditySampleReady()) {
        
        onReadSample(tempSampler->getLastHumiditySample());
            
        // Filter with two cascade single pole IIRs
        applyIIRFilter(IIR1);
        applyIIRFilter(IIR2);
        
        // Apply the decimation filter
        return applyDecimationFilter();
    }
    
    return false;
}

// We don't have a specific prescaler because we share the prescaler value
// with the Temperature sensor
void HumSensorSampler::setPreScaler(unsigned char value) {
}

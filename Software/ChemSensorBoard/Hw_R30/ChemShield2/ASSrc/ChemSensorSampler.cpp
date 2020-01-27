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

#include "ChemSensorSampler.h"

ChemSensorSampler::ChemSensorSampler(const ADC16S626& adc) : sensor(adc) {
}

ChemSensorSampler::~ChemSensorSampler() {
}

bool ChemSensorSampler::sampleTick() {

    if (timer == prescaler) {
        
        // It's time for a new sample
        timer = 0;
        go = true;
        return true;
    }
    timer++;    
    
    return false;
}

bool ChemSensorSampler::sampleLoop() {

    // Take the new sample if channel is enabled
    if (enabled && go) {
        onReadSample(sensor.getSample());
        go = false;
        
        // Filter with two cascade single pole IIRs
        applyIIRFilter(IIR1);
        applyIIRFilter(IIR2);
        
        // Apply the decimation filter
        return applyDecimationFilter();
    }
    
    return false;
}

const char* ChemSensorSampler::getMeasurementUnit() const {
	return "nA";
}

double ChemSensorSampler::evaluateMeasurement(unsigned short lastSample) const {
	return 0.0;
}

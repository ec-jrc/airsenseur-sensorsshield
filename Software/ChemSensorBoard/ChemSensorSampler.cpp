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

    // Take the new sample
    if (go) {
        lastSample = sensor.getSample();
        go = false;
        
        // Filter with two cascade single pole IIRs
        applyIIRFilter(IIR1);
        applyIIRFilter(IIR2);
        
        // Apply the decimation filter
        return applyDecimationFilter();
    }
    
    return false;
}

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

#include "PressSensorSampler.h"

PressSensorSampler::PressSensorSampler(SFE_BMP180& press) : sensor(press) {
    mStatus = STATUS_START_TEMP;
}

PressSensorSampler::~PressSensorSampler() {
}

void PressSensorSampler::setPreScaler(unsigned char value) {
    
    if (value < 24) {
        value = 24;
    }
    
    value = value + 1;
    prescaler = value >> 2;    
    timer = 0;
    mStatus = STATUS_START_TEMP;
}

unsigned char PressSensorSampler::getPrescaler() {
    
    return (prescaler << 2);
}

bool PressSensorSampler::sampleTick() {
        
    if (timer == (prescaler-1)) {
        
        timer = 0;
        go = true;
        
        return (mStatus == STATUS_SAMPLE);
    }
    timer++;    
    
    return false;
}

bool PressSensorSampler::sampleLoop() {

    if (!go) {
        return false;
    }
    
    go = false;
    bool result = false;
    switch (mStatus) {
        
        case STATUS_START_TEMP: {
            
            sensor.startTemperature();
            mStatus = STATUS_GET_TEMP;
        }
            break;
            
        case STATUS_GET_TEMP: {
            
            temperature = 0;
            sensor.getTemperature(temperature);
            mStatus = STATUS_START_PRESS;
        }
            break;
                        
        case STATUS_START_PRESS: {
            sensor.startPressure(0);
            mStatus = STATUS_SAMPLE;
        }
            break;
            
        case STATUS_SAMPLE: {
            
            double pressure = 0;
            
            sensor.getPressure(pressure, temperature);
            
            lastSample = (unsigned short) (pressure * 48);
            
            // Filter with two cascade single pole IIRs
            applyIIRFilter(IIR1);
            applyIIRFilter(IIR2);

            // Apply the decimation filter
            result = applyDecimationFilter();
            
            mStatus = STATUS_START_TEMP;
        }
            break;
            
        default:
            mStatus = STATUS_START_TEMP;
            break;
    }
    
    return result;
}

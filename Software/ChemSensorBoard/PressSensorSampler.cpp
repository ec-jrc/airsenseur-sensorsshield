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

#include "PressSensorSampler.h"

PressSensorSampler::PressSensorSampler(BMP280& press) : sensor(press) {
}

PressSensorSampler::~PressSensorSampler() {
}

void PressSensorSampler::setPreScaler(unsigned char value) {

    // We expect a maximum rate of 26.32Hz when the BMP280
    // operates in normal mode, 16x oversampling
    if (value < 4) {
        value = 4;
    }
    
    prescaler = value;    
    timer = 0;
}

bool PressSensorSampler::sampleTick() {

    if (timer == prescaler) {
        
        // It's time for a new sample
        timer = 0;
        go = true;
        return true;
    }
    timer++;    
    
    return false;
}

bool PressSensorSampler::sampleLoop() {

    // Take the new sample
    if (go) {
      double temperature = 0;
      double pressure = 0;
  
      sensor.getTemperature(temperature);
      sensor.getPressure(pressure);
      
      onReadSample((unsigned short) (pressure * 48));
      go = false;            
      
      // Filter with two cascade single pole IIRs
      applyIIRFilter(IIR1);
      applyIIRFilter(IIR2);
  
      // Apply the decimation filter
      return applyDecimationFilter();
    }
    
    return false;
}

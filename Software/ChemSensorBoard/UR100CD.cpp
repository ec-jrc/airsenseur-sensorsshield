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
#include "UR100CD.h"
#include <Wire.h>

#define UR100CD_ADDRESS         (0x50 >> 1)

UR100CD::UR100CD() {
    Wire.begin();
}

UR100CD::~UR100CD() {
}

bool UR100CD::startConvertion() const {
    
    Wire.beginTransmission(UR100CD_ADDRESS);
    bool result = (Wire.endTransmission() == 0);
    
    return result;
}

bool UR100CD::getSamples(unsigned short *temperature, unsigned short *humidity) const {
    
    unsigned char sample = 0;
    
    Wire.requestFrom(UR100CD_ADDRESS, 4);
    sample = Wire.available();
    if (sample != 4)
        return false;
    
    // Read humidity
    byte lowVal, highVal;
    highVal = Wire.read();
    lowVal = Wire.read();
    (*humidity) = (((unsigned short) (highVal & 0x3F)) << 8) | lowVal;
    
    // Read temperature
    highVal = Wire.read();
    lowVal = Wire.read();
    (*temperature) = (((unsigned short) highVal) << 6) | ((lowVal >> 2) & 0x3F);
    
    return true;
}

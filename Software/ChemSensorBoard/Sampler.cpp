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


#include <EEPROM.h>
#include "DitherTool.h"
#include "Sampler.h"
#include "Persistence.h"

DitherTool* Sampler::ditherTool = (DitherTool*)0x00;

Sampler::Sampler() {
    go = false; 
    prescaler = 0;
    timer = 0;
    decimation = 0; 
    decimationTimer = 0; 
    lastSample = 0;
    iIRDenum[0] = 0;
    iIRDenum[1] = 0;
    iIRAccumulator[0] = 0;
    iIRAccumulator[1] = 0;
}

void Sampler::setPreScaler(unsigned char value) {
    prescaler = value;
    timer = 0;
}

unsigned char Sampler::getPrescaler() {
    return prescaler;
}

void Sampler::setDecimation(unsigned char value) {
    decimation = value; 
    decimationTimer = 0;
}

unsigned char Sampler::getDecimation() {
    return decimation;
}

unsigned short Sampler::getLastSample() {
    return lastSample;
}

bool Sampler::applyDecimationFilter() {
    
    if (decimationTimer == decimation) {
        decimationTimer = 0;
        return true;
    }

    decimationTimer++;
    
    return false;
}

unsigned char Sampler::getIIRDenom(unsigned char iIRID) {

    if (iIRID > 1)
        return 0;
    
    return iIRDenum[iIRID];
}

void Sampler::setIIRDenom(unsigned char iIRID, unsigned char value) {

    if (iIRID > 1)
        return;
    
    if (value == 0xFF)
        value = 0;
    
    iIRDenum[iIRID] = value;
    iIRAccumulator[iIRID] = 0.0f;
}

void Sampler::setDitherTool(DitherTool* tool) {
    ditherTool = tool;
}

// Apply the IIR filter. Input and Output are from/to lastSample
void Sampler::applyIIRFilter(unsigned char iiRID) {

    if (iiRID > 1)
        return;
    
    unsigned char *denom = iIRDenum + iiRID;
    
    // The filter is disabled if denominator == 0
    if (*denom == 0) {
        return;
    }
    
    double* S = iIRAccumulator + iiRID;
    
    // S(n) = S(n-1) + 1/den * (I(n) - S(n-1))
    *S = *S + ((double)lastSample - *S)/(*denom);
    
    lastSample = (unsigned short)(ditherTool->applyDithering(*S));
}


bool Sampler::loadPreset(unsigned char myID) {

    // Load prescaler and decimation
    unsigned char prescVal = EEPROM.read(SAMPLER_PRESET_PRESCALER(myID));
    unsigned char decimVal = EEPROM.read(SAMPLER_PRESET_DECIMATION(myID));
    unsigned char iir1Denom = EEPROM.read(SAMPLER_PRESET_IIR1DENOM(myID));
    unsigned char iiR2Denom = EEPROM.read(SAMPLER_PRESET_IIR2DENOM(myID));
    
    // Apply
    setPreScaler(prescVal);
    setDecimation(decimVal);
    setIIRDenom(0, iir1Denom);
    setIIRDenom(1, iiR2Denom);
    
    return true;
}

bool Sampler::savePreset(unsigned char myID) {
    
    // Get values and store them
    EEPROM.write(SAMPLER_PRESET_PRESCALER(myID), getPrescaler());
    EEPROM.write(SAMPLER_PRESET_DECIMATION(myID), getDecimation());
    EEPROM.write(SAMPLER_PRESET_IIR1DENOM(myID), getIIRDenom(0));
    EEPROM.write(SAMPLER_PRESET_IIR2DENOM(myID), getIIRDenom(1));
    
    return true;
}

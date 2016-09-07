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
 *   Laurent Spinelle – laurent.spinelle@jrc.ec.europa.eu and 
 *   Alexander Kotsev - alexander.kotsev@jrc.ec.europa.eu,
 *   European Commission - Joint Research Centre, 
 * - Marco Signorini, marco.signorini@liberaintentio.com
 *
 * ===========================================================================
 */

#include <Arduino.h>
#include "ADC16S626.h"
#include <SPI.h>

ADC16S626::ADC16S626(const unsigned char csPin) : m_csPin(csPin) {
    pinMode(m_csPin, OUTPUT);
    digitalWrite(m_csPin, HIGH);
    
    SPI.begin();
    SPI.setDataMode(SPI_MODE3);
}

ADC16S626::~ADC16S626() {
}

unsigned short ADC16S626::getSample() const {
    
    byte inByteHigh, inByteMid, inByteLow;
    
    digitalWrite(m_csPin, LOW);
    inByteHigh = SPI.transfer(0x00);
    inByteMid = SPI.transfer(0x00);
    inByteLow = SPI.transfer(0x00);
    digitalWrite(m_csPin, HIGH);
    
    unsigned short result = ((inByteLow >> 6) & 0x03);
    result |= (((unsigned short)inByteMid) << 2);
    result |= (((unsigned short)inByteHigh) << 10);
    
    result = toLinear(result);
    
    return result;
}

unsigned short ADC16S626::toLinear(unsigned short twoComplSample) const {

    // 2 complement conversion     
    // in       ->      out
    // 0        ->      32768
    // 32767    ->      65535
    // 32768    ->      0
    // 65535    ->      32767
    if (twoComplSample > 32767) {
            twoComplSample = twoComplSample - 32768;
    } else {
            twoComplSample = twoComplSample + 32768;
    }
    return twoComplSample;
}


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
#include <Wire.h>
#include <SPI.h>
#include <MemoryFree.h>
#include <EEPROM.h>
#include <TimerOne.h>

#include "ChemSensorSampler.h"
#include "SensorsArray.h"
#include "LMP91000.h"
#include "ADC16S626.h"
#include "UR100CD.h"
#include "CommProtocol.h"
#include "Persistence.h"

#define HBLEDPIN              10
#define HBLED_NEWSAMPLE_TMR   3

void EEPROMErase();
void EEPROMDump();


SensorsArray* sensorBoard;
CommProtocol* commProtocol;
unsigned char hbTimer;

void timerInterrupt() {

    if (sensorBoard) {
        sensorBoard->timerTick();
    }
    if (commProtocol) {
        commProtocol->timerTick();
    }
    
    // Heartbeat led
    if (hbTimer != 0) {
        hbTimer--;
        digitalWrite(HBLEDPIN, HIGH);
    } else {
        digitalWrite(HBLEDPIN, LOW);
    }
}

void setup_impl() {
  
    // Initialize the heartbeat pin
    pinMode(HBLEDPIN, OUTPUT);
    digitalWrite(HBLEDPIN, LOW);
    hbTimer = 0;
    
    // Initialize the serial line
    Serial.begin(9600);
          
    // Instantiate the main objects
    sensorBoard = new SensorsArray();
    commProtocol = new CommProtocol(sensorBoard);
        
    // Initialize the timer at 10ms
    Timer1.initialize(10000);
    Timer1.attachInterrupt(timerInterrupt);
}

void loop_impl() {
  
    // Propagate to the sensor array
    bool newSample = sensorBoard->loop();
    
    // Heartbeat led
    if (newSample && (hbTimer == 0)) {
        hbTimer = HBLED_NEWSAMPLE_TMR;
    }
    
    // Handle the serial line
    if (Serial.available()) {
        unsigned char val = Serial.read();
        commProtocol->onDataReceived(val);
    }
}


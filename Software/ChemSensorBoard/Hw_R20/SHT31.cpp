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
 *      European Commission - Joint Research Centre, 
 * - Marco Signorini, marco.signorini@liberaintentio.com
 *
 * ===========================================================================
 */

#include <Arduino.h>
#include "SHT31.h"
#include <Wire.h>

#define SHT31ADDRESS                    0x44
#define SHT31_CMD_START_HIGHREP         0x2400
#define SHT31_CMD_START_MEDREP          0x240B
#define SHT31_CMD_START_LOWREP          0x2416

#define SHT31_CMD_READ_STATUS           0xF32D
#define SHT31_CMD_CLEAR_STATUS          0x3041

SHT31::SHT31() {
}

SHT31::~SHT31() {
}

bool SHT31::begin() {
  
  Wire.begin();
  return sendCommand(SHT31_CMD_CLEAR_STATUS);
}

bool SHT31::startConvertion() const {
  return sendCommand(SHT31_CMD_START_LOWREP);
}

bool SHT31::getSamples(unsigned short *temperature, unsigned short *humidity) const {
  return readData(temperature, humidity);
}

char SHT31::sendCommand(unsigned short command) const {
  Wire.beginTransmission(SHT31ADDRESS);
  Wire.write(command >> 8);
  Wire.write(command & 0xFF);
  return (Wire.endTransmission() == 0)? 1 : 0; 
}

char SHT31::readData(unsigned short *temperature, unsigned short *humidity) const {

  unsigned char data[6];
  
  Wire.requestFrom(SHT31ADDRESS, 6);
  if (Wire.available() != 6) 
    return 0;

  // retrieve data from the sensor
  for (uint8_t i=0; i<6; i++) {
    data[i] = Wire.read();
  }

  // convert to unsigned short
  *temperature = ((unsigned short)data[0] << 8) | data[1];
  *humidity = ((unsigned short)data[3] << 8) | data[4];

  return 1;
}


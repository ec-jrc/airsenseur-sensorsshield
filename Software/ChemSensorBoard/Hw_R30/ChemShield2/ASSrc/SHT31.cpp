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

#include "SHT31.h"
#include "I2CBHelper.h"
#include <string.h>

#define SHT31ONBOARDADDRESS             (0x44<<1)
#define SHT32OFFBOARDADDRESS			(0x45<<1)
#define SHT31_CMD_START_HIGHREP         0x2400
#define SHT31_CMD_START_MEDREP          0x240B
#define SHT31_CMD_START_LOWREP          0x2416

#define SHT31_CMD_READ_STATUS           0xF32D
#define SHT31_CMD_CLEAR_STATUS          0x3041

SHT31::SHT31() : sensorAddress(SHT32OFFBOARDADDRESS), available(false) {
}

SHT31::SHT31(bool internal) {
	sensorAddress = internal? SHT31ONBOARDADDRESS: SHT32OFFBOARDADDRESS;
	available = false;
}

SHT31::~SHT31() {
}

bool SHT31::begin() {

	unsigned char checkNumber = 0;

	available = false;
	while ((checkNumber < 10) && !available) {
		available |= (sendCommand(SHT31_CMD_CLEAR_STATUS) != 0);
		checkNumber++;
	}

	return available;
}

bool SHT31::startConvertion() const {
  if (!available) {
	  return false;
  }

  return sendCommand(SHT31_CMD_START_LOWREP);
}

bool SHT31::getSamples(unsigned short *temperature, unsigned short *humidity) const {
  if (!available) {
	  return false;
  }

  return readData(temperature, humidity);
}

bool SHT31::isAvailable() const {
	return available;
}

char SHT31::sendCommand(unsigned short command) const {

	unsigned char buffer[] = { (unsigned char)(command >> 8), (unsigned char)(command & 0xFF) };
	bool result = I2CB.write(sensorAddress, buffer, 2);

  return (result)?1:0;
}

bool SHT31::getTemperatureChannelName(unsigned char* buffer) const {

	if (!buffer) {
		return false;
	}

	if (sensorAddress == SHT31ONBOARDADDRESS) {
		strcpy((char*)buffer, "SHT31TI");
	} else {
		strcpy((char*)buffer, "SHT31TE");
	}

	return true;
}

bool SHT31::getHumidityChannelName(unsigned char* buffer) const {

	if (!buffer) {
		return false;
	}

	if (sensorAddress == SHT31ONBOARDADDRESS) {
		strcpy((char*)buffer, "SHT31HI");
	} else {
		strcpy((char*)buffer, "SHT31HE");
	}

	return true;
}


char SHT31::readData(unsigned short *temperature, unsigned short *humidity) const {

  unsigned char data[6];
  bool result = I2CB.read(sensorAddress, data, 0x06);

  // convert to unsigned short
  *temperature = ((unsigned short)data[0] << 8) | data[1];
  *humidity = ((unsigned short)data[3] << 8) | data[4];

  return (result)?1:0;
}


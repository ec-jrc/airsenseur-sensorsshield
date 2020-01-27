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

#include "AD5694REval.h"

AD5694REval::AD5694REval(const unsigned char gainPin, const unsigned char address) : AD5694R(gainPin, address) ,
	m_doubleGain(false) {

	for (unsigned char n = 0; n < AD5694_DAC_NUMCHANNELS; n++) {
		value[n] = 0.0f;
	}
}

AD5694REval::~AD5694REval() {
}

bool AD5694REval::setGain(bool doubleGain) {
	bool result = AD5694R::setGain(doubleGain);

	if (result) {
		m_doubleGain = doubleGain;
	}

	return result;
}

bool AD5694REval::writeRegister(unsigned char address, unsigned char msb, unsigned char lsb) {
	bool result = AD5694R::writeRegister(address, msb, lsb);

	if (result) {
		unsigned char channel = 0;
		switch (address & 0x0F) {
			case AD5694_DAC_A:
			default:
				break;

			case AD5694_DAC_B:
				channel = 1;
				break;

			case AD5694_DAC_C:
				channel = 2;
				break;

			case AD5694_DAC_D:
				channel = 3;
				break;
		}

		unsigned short dacValue = ((((unsigned short)msb) << 8) | lsb) >> 4;
		value[channel] = 2.5f * (dacValue / 4095.0f);
	}

	return result;
}

bool AD5694REval::readRegister(unsigned char address, unsigned char* msb, unsigned char* lsb) {
	bool result = AD5694R::readRegister(address, msb, lsb);

	if (result) {
		unsigned char channel = 0;
		switch (address & 0x0F) {
			case AD5694_DAC_A:
			default:
				break;

			case AD5694_DAC_B:
				channel = 1;
				break;

			case AD5694_DAC_C:
				channel = 2;
				break;

			case AD5694_DAC_D:
				channel = 3;
				break;
		}

		unsigned short dacValue = ((((unsigned short)(*msb)) << 8) | (*lsb)) >> 4;
		value[channel] = 2.5f * (dacValue / 4095.0f);
	}

	return result;
}

double AD5694REval::getChannelVoltage(unsigned char channelId) {
	if (channelId >= AD5694_DAC_NUMCHANNELS) {
		return 0.0;
	}

	return value[channelId] * ((m_doubleGain)? 2.0:1.0);
}

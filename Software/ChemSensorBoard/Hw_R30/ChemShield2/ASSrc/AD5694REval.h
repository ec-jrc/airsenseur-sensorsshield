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

#ifndef AD5694REVAL_H_
#define AD5694REVAL_H_

#include "AD5694R.h"

#define AD5694_DAC_NUMCHANNELS 0x04

class AD5694REval : public AD5694R {
public:
	AD5694REval(const unsigned char gainPin, const unsigned char address);
	virtual ~AD5694REval();

public:
	bool setGain(bool doubleGain);
	double getChannelVoltage(unsigned char channelId);

protected:
    virtual bool writeRegister(unsigned char address, unsigned char msb, unsigned char lsb);
    virtual bool readRegister(unsigned char address, unsigned char* msb, unsigned char* lsb);

private:
	bool m_doubleGain;
	double value[AD5694_DAC_NUMCHANNELS];
};

#endif /* AD5694REVAL_H_ */

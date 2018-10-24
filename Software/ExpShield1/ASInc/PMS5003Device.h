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

#ifndef PMS5003DEVICE_H_
#define PMS5003DEVICE_H_

#include "SensorDevice.h"

#define PMS5300_PM1CONC_ST		0x00
#define PMS5300_PM25CONC_ST		0x01
#define PMS5300_PM10CONC_ST		0x02
#define PMS5300_PM1CONC_AT		0x03
#define PMS5300_PM25CONC_AT		0x04
#define PMS5300_PM10CONC_AT		0x05
#define PMS5300_PART03			0x06
#define PMS5300_PART05			0x07
#define PMS5300_PART10			0x08
#define PMS5300_PART25			0x09
#define PMS5300_PART50			0x0A
#define PMS5300_PART100			0x0B

#define PSM5003_NUM_CHANNELS	(PMS5300_PART100 + 1)


class PMS5003Device : public SensorDevice {
public:
	PMS5003Device();
	virtual ~PMS5003Device();
	virtual void onStartSampling();
	virtual void onStopSampling();
	virtual void setLowPowerMode(bool lowPower);
	virtual void loop();
	virtual void tick();
	virtual const char* getChannelName(unsigned char channel) const;

private:
	typedef struct _datastruct {
		unsigned short length;
		unsigned short pm1concSt;
		unsigned short pm25concSt;
		unsigned short pm10concSt;
		unsigned short pm1concAt;
		unsigned short pm25concAt;
		unsigned short pm10concAt;
		unsigned short part03;
		unsigned short part05;
		unsigned short part10;
		unsigned short part25;
		unsigned short part50;
		unsigned short part100;
		unsigned short reserved;
		unsigned short check;
	} datastruct;

	datastruct dataStruct;
	unsigned char curOffset;
	unsigned short startCharacter;
	bool validFrame;

private:
	static const char* channelNames[];

private:
	void onDataReceived(unsigned char pivotChar);
	bool checkCRC();
	void swapEndianess();
	void evaluateFrame();
};

#endif /* PMS5003DEVICE_H_ */

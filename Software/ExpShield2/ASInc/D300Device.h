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

#ifndef D300DEVICE_H_
#define D300DEVICE_H_

#include "SensorDevice.h"

class D300Device : public SensorDevice {
public:
	D300Device();
	virtual ~D300Device();

	virtual void onStartSampling();
	virtual void onStopSampling();
	virtual void setLowPowerMode(bool lowPower);
	virtual bool init();
	virtual void loop();
	virtual void tick();

	virtual const char* getSerial() const;

	virtual bool setChannelName(unsigned char channel, const char* name);
	virtual const char* getChannelName(unsigned char channel) const;
	virtual const char* getMeasurementUnit(unsigned char channel) const;
	virtual float evaluateMeasurement(unsigned char channel, float value) const;

	virtual void triggerSample();

	static const unsigned char defaultSampleRate();

private:
	void powerOn(bool on);

private:
	volatile bool go;
	volatile unsigned short blankTimer;
	bool available;
};

#endif /* D300DEVICE_H_ */

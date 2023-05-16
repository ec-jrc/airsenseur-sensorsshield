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


#ifndef PIDDEVICE_H_
#define PIDDEVICE_H_

#include <stdint.h>
#include "SensorDevice.h"

#define PIDDEV_CHANNEL_PID_HEATER	0x00
#define PIDDEV_CHANNEL_PID_COOLER	0x01

#define PIDDEV_NUM_OF_CHANNELS 		(PIDDEV_CHANNEL_PID_COOLER+1)

class PIDDevice : public SensorDevice {
public:
	virtual ~PIDDevice();

	virtual void onStopSampling();
	virtual void setLowPowerMode(bool lowPower);
	virtual bool init();
	virtual void loop();
	virtual void tick();

	virtual const char* getSerial() const;

	virtual const char* getChannelName(unsigned char channel) const;
	virtual bool setChannelName(unsigned char channel, const char* name);
	virtual const char* getMeasurementUnit(unsigned char channel) const;
	virtual float evaluateMeasurement(unsigned char channel, float value, bool firstSample) const;
	virtual void triggerSample();
	virtual bool writeGenericRegister(unsigned int address, unsigned int value);
	virtual bool readGenericRegister(unsigned int address, unsigned int& value);

	static PIDDevice* const getInstance();
	static const unsigned char defaultSampleRate();

private:
	PIDDevice();

private:
	void applyPIDCoefficients();
	void writePIDCoefficientsToEEPROM();
	float roundPIDCoefficient(float value);

private:
	typedef struct pidcoeffs {
		union __attribute__((__packed__)) {
			int32_t raw[6];
			struct __attribute__((__packed__)) {
				int32_t P;
				int32_t I;
				int32_t D;
				int32_t dzHeat;
				int32_t dzCool;
				int32_t refSource;
			} coeff;
		} data;
		long crc;
	} pidcoeffs;

private:
	pidcoeffs pidData;

private:
	static const char* const channelNames[];
	static const char* const channelMeasurementUnits[];

	static PIDDevice* const instance;

	volatile bool go;
};

#define AS_PIDDevice (*(PIDDevice::getInstance()))


#endif /* PIDDEVICE_H_ */


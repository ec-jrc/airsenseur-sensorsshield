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

#ifndef ADT7470DEVICE_H_
#define ADT7470DEVICE_H_

#include "SensorDevice.h"

#define ADT7470_CHANNEL_T_INT_CHAMBER		0x00
#define ADT7470_CHANNEL_T_EXT_HEATSINK		0x01
#define ADT7470_CHANNEL_T_INT_HEATSINK		0x02
#define ADT7470_CHANNEL_F_EXT_HEATSINK		0x03
#define ADT7470_CHANNEL_F_INT_HEATSINK		0x04
#define ADT7470_CHANNEL_F_AIR_CIR			0x05

#define ADT7470_NUM_TEMPERATURE_CHANNELS	0x03
#define ADT7470_NUM_FAN_CHANNELS			0x03

#define ADT7470_NUM_CHANNELS				(ADT7470_CHANNEL_F_AIR_CIR + 1)
#define ADT7470_SAMPLING_PERIOD				9	/* Reporting sample rate: 10 seconds */
#define ADT7470_COMMUNICATION_PERIOD		99	/* ADT7470 refresh time: 1 second */
#define ADT7470_COMMUNICATION_PERIOD_ON_ERR	249 /* ADT7470 refresh time on error: 2.5 seconds */

class ADT7470Device : public SensorDevice {

public:
	virtual ~ADT7470Device();

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

	virtual bool setSetpointForChannel(unsigned char channel, unsigned short setpoint);
	virtual bool getSetpointForChannel(unsigned char channel, unsigned short& setpoint);
	unsigned short getFanLastSeenRotating(unsigned char fanID);

	static const unsigned char defaultSampleRate();

	void setCirculationFanSpeedAtSetpoint();
	void setCirculationFanSpeed(unsigned char percentage);

	void setExternalFanSpeedAtSetpoint();
	void setExternalFanSpeed(unsigned char percentage);

	void setInternalFanSpeedAtSetpoint();
	void setInternalFanSpeed(unsigned char percentage);

	short getTemperatureForChannel(unsigned char channel);
	short getChamberTemperature();

	static ADT7470Device* const getInstance();

private:
	ADT7470Device();

	void readTemperatures();
	void readFanSpeeds();
	void setFanSpeed(unsigned char fanID, unsigned char percentage);

	void setRegisterFlag(unsigned char address, unsigned char hexFlag);
	void resetRegisterFlag(unsigned char address, unsigned char hexFlag);

private:
	static const char* const channelNames[];
	static const char* const channelMeasurementUnits[];

	static ADT7470Device* const instance;

private:
	volatile bool go;
	bool error;
	unsigned char communicationTimer;
	short temperatures[ADT7470_NUM_TEMPERATURE_CHANNELS];
	unsigned short fansSpeed[ADT7470_NUM_FAN_CHANNELS];
	unsigned short fansLastSeenRotating[ADT7470_NUM_FAN_CHANNELS]; // in seconds

	// Temperature and fan setpoints in 1/100% units
	unsigned short setpoints[ADT7470_NUM_CHANNELS];
};

#define AS_ADT7470 (*(ADT7470Device::getInstance()))

#endif /* ADT7470DEVICE_H_ */

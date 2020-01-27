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

#ifndef OPCN3DEVICE_H_
#define OPCN3DEVICE_H_

#include "SensorDevice.h"
#include "OPCN3Comm.h"

/* We export all bins, all PMS + temperature, humidity, volume, TSampling, FlowRate, LaserStatus */
#define OPCN3_CHAN_NUMBER	(OPCN3_BINS_NUMBER + OPCN3_PMS_NUMBER + 6)

#define OPCN3_FIRST_BIN		0
#define OPCN3_FIRST_PM		(OPCN3_BINS_NUMBER + OPCN3_PMS_NUMBER)

#define OPCN3_BIN0	0
#define OPCN3_BIN1	1
#define OPCN3_BIN2	2
#define OPCN3_BIN3	3
#define OPCN3_BIN4	4
#define OPCN3_BIN5	5
#define OPCN3_BIN6	6
#define OPCN3_BIN7	7
#define OPCN3_BIN8	8
#define OPCN3_BIN9	9
#define OPCN3_BIN10	10
#define OPCN3_BIN11	11
#define OPCN3_BIN12	12
#define OPCN3_BIN13	13
#define OPCN3_BIN14	14
#define OPCN3_BIN15	15
#define OPCN3_BIN16	16
#define OPCN3_BIN17	17
#define OPCN3_BIN18	18
#define OPCN3_BIN19	19
#define OPCN3_BIN20	20
#define OPCN3_BIN21	21
#define OPCN3_BIN22	22
#define OPCN3_BIN23	23
#define OPCN3_PM01	24
#define OPCN3_PM25	25
#define OPCN3_PM10	26
#define OPCN3_TEMP	27
#define OPCN3_HUM	28
#define OPCN3_VOL	29
#define OPCN3_TSA	30
#define OPCN3_FRT	31
#define OPCN3_LSRST	32

#define OPCN3_SERIAL_NUMBER_MAXLENGTH	0x10

class OPCN3Device : public SensorDevice {

public:
	OPCN3Device();
	virtual ~OPCN3Device();

	virtual void onStartSampling();
	virtual void onStopSampling();
	virtual void setLowPowerMode(bool lowPower);
	virtual void loop();
	virtual void tick();

	virtual const char* getSerial() const;

	virtual bool setChannelName(unsigned char channel, const char* name);
	virtual const char* getChannelName(unsigned char channel) const;
	virtual const char* getMeasurementUnit(unsigned char channel) const;
	virtual float evaluateMeasurement(unsigned char channel, float value) const;

	virtual void triggerSample();

private:
	void resetStateMachineOnTimeout();

	bool evaluateInfoString(OPCN3Comm::infostring* infostring);
	bool evaluateSerialString(OPCN3Comm::serialstring* serialstring);
	bool evaluateHistogram(OPCN3Comm::histogram* histogram);

private:
	typedef enum _status {
		IDLE,
		REQ_INFOSTRING,
		WAIT_INFOSTRING,
		REQ_SERIALSTRING,
		WAIT_SERIALSTRING,
		SET_LASER_ON,
		WAIT_LASER_ON,
		SET_FAN_ON,
		WAIT_FAN_ON,
		SAMPLING_READY,
		SAMPLING_START,
		SAMPLING_WAITING,
		SET_LASER_OFF,
		SET_FAN_OFF
	} status;

private:
	static const char* const channelNames[];
	static const char* const channelMeasurementUnits[];
	static double const evaluationFactors[];

private:
	bool lowPowerMode;
	bool samplingEnabled;
	status curStatus;
	OPCN3Comm opcComm;
	bool isOPCN2Unit;
	char serialNumber[OPCN3_SERIAL_NUMBER_MAXLENGTH];
	volatile unsigned char timer;
};

#endif /* OPCN3DEVICE_H_ */

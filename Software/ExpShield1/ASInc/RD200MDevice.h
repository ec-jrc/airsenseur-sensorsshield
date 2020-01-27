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

#ifndef RD200MDEVICE_H_
#define RD200MDEVICE_H_

#include "SensorDevice.h"

class RD200MDevice : public SensorDevice {
public:
	RD200MDevice();
	virtual ~RD200MDevice();
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

	static const unsigned char defaultSampleRate();
	static const unsigned char defaultDecimationValue();

private:
	unsigned char checkSum();
	void requestData(unsigned char command);
	void onDataReceived(unsigned char pivotChar);
	void evaluateRxBuffer();
	bool evaluateReset();

private:

	typedef enum _rxstate {
		RX_IDLE,
		RX_STARTFOUND,
		RX_COMMANDFOUND,
		RX_WAITING_PARAMETERS,
		RX_WAITING_CRC
	} rxstate;

	typedef struct _datastruct {
		unsigned char command;
		unsigned char dataSize;
		union _data {
			unsigned char buffer[4];
			struct _decoded {
				unsigned char status;
				unsigned char minutes;
				unsigned char unit;
				unsigned char cents;
			} decoded;
		} data;
	} datastruct;

	datastruct dataStruct;
	rxstate curStatus;
	unsigned char curRxOffset;
	bool needsReset;
};

#endif /* RD200MDEVICE_H_ */

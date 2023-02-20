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

#include "SensorDevice.h"

#ifndef NEXTPMDEVICE_H_
#define NEXTPMDEVICE_H_

#define NEXTPM_RXBUFFER		0x0F

#define NEXTPM_PM1PCS		0x00
#define NEXTPM_PM25PCS		0x01
#define NEXTPM_PM10PCS		0x02
#define NEXTPM_PM1CONC		0x03
#define NEXTPM_PM25CONC		0x04
#define NEXTPM_PM10CONC		0x05
#define NEXTPM_TEMPERATURE	0x06
#define NEXTPM_HUMIDITY		0x07
#define NEXTPM_STATUS		0x08

#define NEXTPM_NUM_CHANNELS	(NEXTPM_STATUS + 0x01)

#define NEXTPM_DATABUFFERSZ				12
#define NEXTPM_DATABUFFERSZ_TEMPHUMID	4

class NextPMDevice : public SensorDevice {

	public:
		NextPMDevice();
		virtual ~NextPMDevice();

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
		void enterSleepMode();
		void exitSleepMode();

		unsigned char checkSum();
		void requestData(unsigned char command);
		void onDataReceived(unsigned char pivotChar);
		void evaluateRxBuffer();
		bool evaluateState(unsigned char state);

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
			unsigned char state;

			union _data {
				unsigned char buffer[NEXTPM_DATABUFFERSZ];
				struct _decodedpm {
					unsigned short pm1pcs;
					unsigned short pm25pcs;
					unsigned short pm10pcs;
					unsigned short pm1ug;
					unsigned short pm25ug;
					unsigned short pm10ug;
				} decodedpm;
				struct _dedodedth {
					unsigned short temperature;
					unsigned short humidity;
				} decodedth;
			} data;
		} datastruct;

	private:
		static const char* const channelNames[];
		static const char* const channelMeasurementUnits[];
		static double const evaluationFactors[];

	private:
		bool deviceFound;
		bool samplingOn;
		bool go;

		datastruct dataStruct;
		rxstate curStatus;
		unsigned char expectedDataSize;
		unsigned char curRxOffset;
};

#endif /* NEXTPMDEVICE_H_ */

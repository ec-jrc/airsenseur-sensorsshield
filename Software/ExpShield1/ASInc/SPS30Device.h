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

#ifndef SPS30DEVICE_H_
#define SPS30DEVICE_H_

#include "SensorDevice.h"

#define SPS30_PM1CONC			0x00
#define SPS30_PM25CONC			0x01
#define SPS30_PM4CONC			0x02
#define SPS30_PM10CONC			0x03
#define SPS30_PART05			0x04
#define SPS30_PART10			0x05
#define SPS30_PART25			0x06
#define SPS30_PART40			0x07
#define SPS30_PART100			0x08
#define SPS30_TYPSIZE			0x09

#define SPS30_NUM_CHANNELS	(SPS30_TYPSIZE + 1)

#define SPS30_SERIAL_NUMBER_MAXLENGTH	0x10

class SPS30Device : public SensorDevice {
public:
	SPS30Device();
	virtual ~SPS30Device();

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

private:
	bool startMeasurement() const;
	bool stopMeasurement() const;
	bool readDataReady() const;
	bool decodeMeasurements(unsigned char* data, unsigned short* measurements);
	bool decodeMeasurements(unsigned char* data, float* measurements);
	bool readSerialNumber(char* serialNumber) const;
	bool readFirmwareVersion(char* firmwareVersion) const;
	unsigned char crc(unsigned char data[2]) const;

private:
	static const char* const channelNames[];
	static const char* const channelMeasurementUnits[];
	static float const multiplierFactors[];
	static double const evaluationFactors[];

private:
	bool go;
	unsigned short blankTimer;
	bool deviceReady;
	unsigned char maxCheckReady;
	char serialNumber[SPS30_SERIAL_NUMBER_MAXLENGTH];
};

#endif /* SPS30DEVICE_H_ */

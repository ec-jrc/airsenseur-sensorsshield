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

#ifndef INTADDEVICE_H_
#define INTADDEVICE_H_

#include "SensorDevice.h"

#define INTAD_CHANNEL_PLT_VFBK		0x00
#define INTAD_CHANNEL_PLT_CFBK		0x01
#define INTAD_CHANNEL_VIN_FBK		0x02
#define INTAD_CHANNEL_TEMPERATURE	0x03
#define INTAD_CHANNEL_VREFINT		0x04

#define INTAD_NUM_OF_CHANNELS		(INTAD_CHANNEL_VREFINT + 1)
#define ADC_DMA_BUFFER_SIZE			INTAD_NUM_OF_CHANNELS

#define INTAD_SAMPLING_PERIOD		9		/* Reporting sample rate: 10 seconds */
#define AUTOCALIBRATION_PERIOD		60000	/* 60000 tics -> 10 minutes */

class IntADDevice : public SensorDevice {
public:
	virtual ~IntADDevice();

	virtual void onStartSampling();
	virtual void onStopSampling();
	virtual void setLowPowerMode(bool lowPower);
	virtual bool init();
	virtual void loop();
	virtual void tick();

	virtual const char* getSerial() const;

	virtual const char* getChannelName(unsigned char channel) const;
	virtual bool setChannelName(unsigned char channel, const char* name);
	virtual const char* getMeasurementUnit(unsigned char channel) const;
	virtual float evaluateMeasurement(unsigned char channel, float value) const;
	virtual void triggerSample();

	static const unsigned char defaultSampleRate();

	unsigned short onADCScanTerminated();

	static IntADDevice* const getInstance();

private:
	IntADDevice();

private:
	static const char* const channelNames[];
	static const char* const channelMeasurementUnits[];

	static IntADDevice* const instance;

	unsigned short adcDmaBuffer[ADC_DMA_BUFFER_SIZE];

	volatile unsigned short calibrationTimer;
	volatile bool samplesAvailable;
	volatile bool go;
	bool error;
};

#endif /* INTADDEVICE_H_ */

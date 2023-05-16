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

#ifndef SENSORDEVICE_H_
#define SENSORDEVICE_H_

// Basic sensor device, multiple channel capable
class SensorDevice {
public:
	SensorDevice(const unsigned char reqChannels);
	virtual ~SensorDevice();

	virtual void onStartSampling();
	virtual void onStopSampling() = 0;
	virtual void setLowPowerMode(bool lowPower) = 0;
	virtual bool init() = 0;
	virtual void loop() = 0;
	virtual void tick() = 0;

	virtual const char* getSerial() const = 0;

	virtual const unsigned char getNumChannels() const;
	virtual const char* getChannelName(unsigned char channel) const = 0;
	virtual bool setChannelName(unsigned char channel, const char* name) = 0;
	virtual const char* getMeasurementUnit(unsigned char channel) const = 0;
	virtual float evaluateMeasurement(unsigned char channel, float value, bool firstSample) const = 0;

	virtual bool setSetpointForChannel(unsigned char channel, unsigned short setpoint);
	virtual bool getSetpointForChannel(unsigned char channel, unsigned short& setpoint);
	virtual bool writeGenericRegister(unsigned int address, unsigned int value);
	virtual bool readGenericRegister(unsigned int address, unsigned int& value);


	virtual void triggerSample();
	virtual bool sampleAvailable();
	virtual unsigned short getSample(unsigned char channel);

protected:
	void setSample(unsigned char channel, unsigned short sample);

private:
	volatile bool sampleReady;
	const unsigned char numChannels;
	unsigned short *lastSamples;
};


#endif /* SENSORDEVICE_H_ */

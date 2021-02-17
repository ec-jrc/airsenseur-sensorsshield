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


#include "TempSensorSampler.h"

TempSensorSampler::TempSensorSampler(const SHT31& temp) : sensor(temp) {
    startMeasureTime = 0;
    lastHumiditySample = 0;
    startConversion = false;
    humiditySampleReady = false;
}

TempSensorSampler::~TempSensorSampler() {
}

void TempSensorSampler::setPreScaler(unsigned char value) {
    
	// Null perscaler value is used to disable this channel
	// Prescaler less than 10 are not useful with this type of sensor
    if ((value < 10) && (value != 0)){
        value = 10;
    }
    
    prescaler = value;
    startMeasureTime = (prescaler >> 1);
    timer = 0;
}

bool TempSensorSampler::sampleTick() {
    
    if (timer == startMeasureTime) {
        
        // It's time to ask for a new measure
        startConversion = true;

    } else if (timer == prescaler) {
        timer = 0;        
        
        // It's time to retrieve the samples
        go = true;
        humiditySampleReady = false;
        
        return true;
    }
    timer++;    
    
    return false;
}

bool TempSensorSampler::sampleLoop() {
    
    if (startConversion) {
        
        // It's time to start a new measure
        sensor.startConvertion();
        startConversion = false;        
    } else if (go) {

        // Take the new sample
        unsigned short temperatureSample = 0;
        sensor.getSamples(&temperatureSample, &lastHumiditySample);

        go = false;

        bool sensorAvailable = sensor.isAvailable();
        humiditySampleReady = sensorAvailable;

        if (sensorAvailable && enabled) {
        	onReadSample(temperatureSample);

			// Filter with two cascade single pole IIRs
			applyIIRFilter(IIR1);
			applyIIRFilter(IIR2);

			// Apply the decimation filter
			return applyDecimationFilter();
        }
    }
    
    return false;
}

unsigned short TempSensorSampler::getLastHumiditySample() {
    return lastHumiditySample;
}

bool TempSensorSampler::getHumiditySampleReady() {
    bool result = humiditySampleReady;
    humiditySampleReady = false;
    return result;
}

bool TempSensorSampler::getHumidityChannelName(unsigned char* name) {
	return sensor.getHumidityChannelName(name);
}

bool TempSensorSampler::saveChannelName(unsigned char myID, unsigned char* name) {
	return true;
}

bool TempSensorSampler::getChannelName(unsigned char myID, unsigned char* buffer, unsigned char buffSize) const {
	return sensor.getTemperatureChannelName(buffer);
}

const char* TempSensorSampler::getMeasurementUnit() const {
	return "C ";
}

double TempSensorSampler::evaluateMeasurement(unsigned short lastSample) const {
	return ((((double)lastSample)/65535)*175)-45.0;
}


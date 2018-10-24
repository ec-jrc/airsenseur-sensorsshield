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


#ifndef SENSORSARRAY_H
#define	SENSORSARRAY_H

#include "DitherTool.h"
#include "SamplesAverager.h"
#include "OPCN3Device.h"

// Physical sensor devices
#define SENSOR_RD200M		0x00
#define SENSOR_D300			0x01
#define SENSOR_PMS5300		0x02
#define SENSOR_OPCN3			0x03

#define NUM_OF_TOTAL_SENSORS		(SENSOR_OPCN3 + 1)
#define NUM_OF_TOTAL_SAMPLERS	NUM_OF_TOTAL_SENSORS
#define NUM_OF_TOTAL_AVERAGERS	NUM_OF_TOTAL_SENSORS

// Logical data channels
#define CHANNEL_RD200M					0x00
#define CHANNEL_D300						0x01
#define CHANNEL_PMS5300_PM1CONC_ST		0x02
#define CHANNEL_PMS5300_PM25CONC_ST		0x03
#define CHANNEL_PMS5300_PM10CONC_ST		0x04
#define CHANNEL_PMS5300_PM1CONC_AT		0x05
#define CHANNEL_PMS5300_PM25CONC_AT		0x06
#define CHANNEL_PMS5300_PM10CONC_AT		0x07
#define CHANNEL_PMS5300_PART03			0x08
#define CHANNEL_PMS5300_PART05			0x09
#define CHANNEL_PMS5300_PART10			0x0A
#define CHANNEL_PMS5300_PART25			0x0B
#define CHANNEL_PMS5300_PART50			0x0C
#define CHANNEL_PMS5300_PART100			0x0D

#define CHANNEL_OPCN3_FIRST				0x0E
#define CHANNEL_OPCN3_VOLUME				(CHANNEL_OPCN3_FIRST + OPCN3_VOL)
#define CHANNEL_OPCN3_LAST				(CHANNEL_OPCN3_FIRST + OPCN3_CHAN_NUMBER - 1)


#define NUM_OF_TOTAL_CHANNELS    (CHANNEL_OPCN3_LAST + 1)

#define DEFAULT_AVERAGE_SAMPLENUM       60

class Sampler;
class SensorDevice;

class SensorsArray {
public:
    SensorsArray();
    virtual ~SensorsArray();
        
    unsigned char setSamplePrescaler(unsigned char channel, unsigned char prescaler);
    bool getSamplePrescaler(unsigned char channel, unsigned char* prescaler);
    unsigned char setSamplePostscaler(unsigned char channel, unsigned char postscaler);
    bool getSamplePostscaler(unsigned char channel, unsigned char* postscaler);
    unsigned char setSampleDecimation(unsigned char channel, unsigned char decimation);
    bool getSampleDecimation(unsigned char channel, unsigned char* decimation);
    bool setSampleIIRDenominators(unsigned char channel, unsigned char iirDen1, unsigned char iirDen2);
    bool getSampleIIRDenominators(unsigned char channel, unsigned char *iirDen1, unsigned char *iirDen2);
    
    bool getLastSample(unsigned char channel, unsigned short &lastSample, unsigned long &timestamp);
    
    bool timerTick();
    bool loop();
    
    bool enableSampling(bool enable);
    void inquirySensor(unsigned char channel, unsigned char* buffer, unsigned char bufSize);
    bool savePreset(unsigned char channel, unsigned char *presetName, unsigned char bufSize);
    bool loadPreset(unsigned char channel);
    bool saveSensorSerialNumber(unsigned char channel, unsigned char* buffer, unsigned char buffSize);
    bool readSensorSerialNumber(unsigned char channel, unsigned char* buffer, unsigned char buffSize);
    bool saveBoardSerialNumber(unsigned char* buffer, unsigned char buffSize);
    bool readBoardSerialNumber(unsigned char* buffer, unsigned char buffSize);

private:
    typedef struct _channeltosamplersubchannel {
    		unsigned char sampler;
    		unsigned char subchannel;
    		bool firstChannelForSampler;
    } channeltosamplersubchannel;

private:
    static DitherTool ditherTool;                           		// Dithering toolset
    
    // A suitable channel To Subchannel array to speedup access
    static const channeltosamplersubchannel chToSamplerSubChannel[NUM_OF_TOTAL_CHANNELS];

    SensorDevice* sensors[NUM_OF_TOTAL_SENSORS];			   		// Physical sensor units
    Sampler* samplers[NUM_OF_TOTAL_SAMPLERS];               		// Sampler units for all channels
    SamplesAverager* averagers[NUM_OF_TOTAL_AVERAGERS];      		// Average samples calculators
    
    bool samplingEnabled;                                   		// Sampling is default disabled
    unsigned long timestamp;                                		// Internal timestamp timer
    unsigned char globalPrescaler;						   		// Global sampling prescaler

private:
    void powerUp5V(bool enable);
    void powerUp3V3(bool enable);
    void powerUp12V(bool enable);
};

#endif	/* SENSORSARRAY_H */

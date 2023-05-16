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
class SamplesAverager;

// Physical sensor devices
#define SENSOR_SHT31_I		0x00
#define SENSOR_SHT31_E		0x01
#define SENSOR_INTAD		0x02
#define SENSOR_PID			0x03
#define SENSOR_ADT7470		0x04
#define SENSOR_D300			0x05
#define SENSOR_K96			0x06

// Note: There is an averager and a sampler for each physical sensor
#define NUM_OF_TOTAL_SENSORS		(SENSOR_K96 + 1)
#define NUM_OF_TOTAL_SAMPLERS	NUM_OF_TOTAL_SENSORS
#define NUM_OF_TOTAL_AVERAGERS	NUM_OF_TOTAL_SENSORS

// Logical data channels
#define CHANNEL_TEMPERATURE_I			0x00
#define CHANNEL_HUMIDIDY_I				0x01
#define CHANNEL_TEMPERATURE_E			0x02
#define CHANNEL_HUMIDIDY_E				0x03
#define CHANNEL_PELTIER_V				0x04
#define CHANNEL_PELTIER_C				0x05
#define CHANNEL_VIN_FBK					0x06
#define CHANNEL_UCDIE_TEMPERATURE		0x07
#define CHANNEL_PID_HEATER				0x08
#define CHANNEL_PID_COOLER				0x09
#define CHANNEL_ADT7470_T_INT_CHAMBER	0x0A
#define CHANNEL_ADT7470_T_EXT_HEATSINK	0x0B
#define CHANNEL_ADT7470_T_INT_HEATSINK	0x0C
#define CHANNEL_ADT7470_F_EXT_HEATSINK	0x0D
#define CHANNEL_ADT7470_F_INT_HEATSINK	0x0E
#define CHANNEL_ADT7470_F_AIR_CIR		0x0F
#define CHANNEL_D300					0x10
#define CHANNEL_K96_LPL_PC_FLT			0x11
#define CHANNEL_K96_SPL_PC_FLT			0x12
#define CHANNEL_K96_MPL_PC_FLT			0x13
#define CHANNEL_K96_PRESS0				0x14
#define CHANNEL_K96_TEMP_NTC0			0x15
#define CHANNEL_K96_TEMP_NTC1			0x16
#define CHANNEL_K96_TEMP_UCDIE			0x17
#define CHANNEL_K96_RH0					0x18
#define CHANNEL_K96_T_RH0				0x19
#define CHANNEL_K96_LPL_UFLT_IR			0x1A
#define CHANNEL_K96_SPL_UFLT_IR			0x1B
#define CHANNEL_K96_MPL_UFLT_IR			0x1C
#define CHANNEL_K96_ERRORSTATUS			0x1D
#define CHANNEL_K96_LPL_UFLT_ERR		0x1E
#define CHANNEL_K96_SPL_UFLT_ERR		0x1F
#define CHANNEL_K96_MPL_UFLT_ERR		0x20

#define NUM_OF_TOTAL_CHANNELS    		(CHANNEL_K96_MPL_UFLT_ERR+1)

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
    bool getLastSample(unsigned char channel, float &lastSample, unsigned long &timestamp);

    bool setSetpoint(unsigned char channel, unsigned short setPointVal);
    bool getSetpoint(unsigned char channel, unsigned short& setPointVal);

    void inquirySensor(unsigned char channel, unsigned char* buffer, unsigned char bufSize);
    bool savePreset(unsigned char channel, unsigned char *presetName, unsigned char bufSize);
    bool loadPreset(unsigned char channel);
    bool saveSensorSerialNumber(unsigned char channel, unsigned char* buffer, unsigned char buffSize);
    bool readSensorSerialNumber(unsigned char channel, unsigned char* buffer, unsigned char buffSize);
    bool readChannelSamplePeriod(unsigned char channel, unsigned long *samplePeriod);
    bool getUnitForChannel(unsigned char channel, unsigned char* buffer, unsigned char buffSize);
    bool setEnableChannel(unsigned char channel, unsigned char enabled);
    bool getChannelIsEnabled(unsigned char channel, unsigned char *enabled);
    bool writeGenericRegisterChannel(unsigned char channel, unsigned int address, unsigned int value);
    bool readGenericRegisterChannel(unsigned char channel, unsigned int address, unsigned int& value);

    bool enableSampling(bool enable);

    bool saveBoardSerialNumber(unsigned char* buffer, unsigned char buffSize);
    bool readBoardSerialNumber(unsigned char* buffer, unsigned char buffSize);
    unsigned short getBoardType();
    unsigned short getBoardNumChannels();

    bool getIsFlyboardReady();

    bool init();
    bool timerTick();
    bool loop();

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

    SensorDevice* const sensors[NUM_OF_TOTAL_SENSORS];			   	// Physical sensor units
    Sampler* const samplers[NUM_OF_TOTAL_SAMPLERS];               	// Sampler units for all channels
    SamplesAverager* const averagers[NUM_OF_TOTAL_AVERAGERS];   	// Average samples calculators
    
    bool samplingEnabled;                                   		// Sampling is default disabled
    volatile unsigned long timestamp;                               // Internal timestamp timer (in 0.01s)
    volatile unsigned char globalPrescaler;						   	// Global sampling prescaler
};

#endif	/* SENSORSARRAY_H */

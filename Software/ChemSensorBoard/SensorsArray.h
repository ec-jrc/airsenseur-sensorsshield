/* ========================================================================
 * Copyright 2015 EUROPEAN UNION
 *
 * Licensed under the EUPL, Version 1.1 or - as soon they will be approved by 
 * the European Commission - subsequent versions of the EUPL (the "Licence"); 
 * You may not use this work except in compliance with the Licence. 
 * You may obtain a copy of the Licence at: http://ec.europa.eu/idabc/eupl
 * Unless required by applicable law or agreed to in writing, software distributed 
 * under the Licence is distributed on an "AS IS" basis, WITHOUT WARRANTIES OR 
 * CONDITIONS OF ANY KIND, either express or implied. See the Licence for the 
 * specific language governing permissions and limitations under the Licence.
 * Date: 02/04/2015
 * Authors
 * - Michel Gerboles  - michel.gerboles@jrc.ec.europa.eu,  
 *                     European Commission - Joint Research Centre, 
 * - Laurent Spinelle - laurent.spinelle@jrc.ec.europa.eu,
 *                     European Commission - Joint Research Centre, 
 * - Marco Signorini  - marco.signorini@liberaintentio.com
 * 
 * ======================================================================== 
 */


#ifndef SENSORSARRAY_H
#define	SENSORSARRAY_H

#include "LMP91000.h"
#include "ADC16S626.h"
#include "AD5694R.h"
#include "UR100CD.h"
#include "SFE_BMP180.h"
#include "DitherTool.h"
#include "SamplesAverager.h"

#define NUM_OF_CHEM_SENSORS     4
#define NUM_OF_UR100_SENSORS    1
#define NUM_OF_BMP180_SENSORS   1

#define CHEMSENSOR_1            0x00
#define CHEMSENSOR_2            0x01
#define CHEMSENSOR_3            0x02
#define CHEMSENSOR_4            0x03
#define PRESSENSOR_1            0x04
#define TEMPSENSOR_1            0x05
#define HUMSENSOR_1             0x06

#define NUM_OF_TOTAL_SENSORS    (HUMSENSOR_1 + 1)

#define BMP180SENSOR_1          0x00
#define UR100SENSOR_1           0x00

#define DEFAULT_AVERAGE_SAMPLENUM       60

class Sampler;

class SensorsArray {
public:
    SensorsArray();
    virtual ~SensorsArray();
    
    const LMP91000* getAFE(unsigned char channel);
    const ADC16S626* getADC(unsigned char channel);
    const AD5694R* getDAC(unsigned char channel);
    UR100CD* getUR100(unsigned char channel);
    SFE_BMP180* getBMP180(unsigned char channel);
    
    unsigned char setSamplePrescaler(unsigned char channel, unsigned char prescaler);
    bool getSamplePrescaler(unsigned char channel, unsigned char* prescaler);
    unsigned char setSamplePostscaler(unsigned char channel, unsigned char postscaler);
    bool getSamplePostscaler(unsigned char channel, unsigned char* postscaler);
    unsigned char setSampleDecimation(unsigned char channel, unsigned char decimation);
    bool getSampleDecimation(unsigned char channel, unsigned char* decimation);
    bool setSampleIIRDenominators(unsigned char channel, unsigned char iirDen1, unsigned char iirDen2);
    bool getSampleIIRDenominators(unsigned char channel, unsigned char *iirDen1, unsigned char *iirDen2);
    
    bool getLastSample(unsigned char channel, unsigned short &lastSample, unsigned long &timestamp);
    
    bool timeTick();
    bool loop();
    
    bool enableSampling(bool enable);
    void inquirySensor(unsigned char channel, unsigned char* buffer, unsigned char bufSize);
    bool writeAFERegisters(unsigned char channel, unsigned char tia, unsigned char ref, unsigned char mode);
    bool readAFERegisters(unsigned char channel, unsigned char *tia, unsigned char *ref, unsigned char *mode);
    bool writeDACRegisters(unsigned char channel, unsigned char subchannel, unsigned short value, bool gain);
    bool readDACRegisters(unsigned char channel, unsigned char subchannel, unsigned short* value, bool *gain);
    bool savePreset(unsigned char channel, unsigned char *presetName, unsigned char bufSize);
    bool loadPreset(unsigned char channel);
    
private:
    unsigned short twoComplement(unsigned short sample);
    
private:
    static const LMP91000 AFEList[NUM_OF_CHEM_SENSORS];     // The analog frontend for chemical sensors
    static const ADC16S626 ADCList[NUM_OF_CHEM_SENSORS];    // The ADC devices for chemical sensors
    static const AD5694R DACList[NUM_OF_CHEM_SENSORS];      // The DAC devices for chemical sensors
    static UR100CD ur100cd;                                 // Temperature and humidity sensor
    static SFE_BMP180 bmp180;                               // Pressure sensor
    static DitherTool ditherTool;                           // Dithering toolset
    
    Sampler* samplers[NUM_OF_TOTAL_SENSORS];                // Sampler units for all sensors
    SamplesAverager* averagers[NUM_OF_TOTAL_SENSORS];       // Average samples calculators
    
    bool samplingEnabled;                                   // Sampling is default disabled
    unsigned long timestamp;                                // Internal timestamp timer
};

#endif	/* SENSORSARRAY_H */

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

#ifndef SAMPLER_H
#define	SAMPLER_H

class SensorDevice;

/* This is the base class for a sampler unit. 
   A sampler unit implements a basic multichannel aware sampler with prescaler and decimation timing option
*/
class Sampler {
    
public:    
    Sampler(unsigned char channels, SensorDevice *sensor);
    virtual ~Sampler() { };
    
    virtual const unsigned char getNumChannels() const;

    virtual void setPreScaler(unsigned char value);
    virtual unsigned char getPrescaler();
    
    virtual void setDecimation(unsigned char value);
    virtual unsigned char getDecimation();
    
    virtual bool savePreset(unsigned char myID);
    virtual bool loadPreset(unsigned char myID);
    
    virtual void onStartSampling();
    virtual void onStopSampling();

    virtual bool sampleTick();
    virtual bool sampleLoop();
    
    virtual void setLowPowerMode(bool lowPower);

    virtual unsigned short getLastSample(unsigned char channel);
    
protected:
    virtual bool applyDecimationFilter();
    virtual void onReadSample(unsigned char channel, unsigned short newSample);
    SensorDevice* getSensor();
    
protected:
    volatile bool  go;                   // it's time for a new sample (shared info from interrupt)
    unsigned char  prescaler;            // basic samples are taken at sampleTick/prescaler
    unsigned char  timer;                // this is the prescaler counter
    
    unsigned char  decimation;           // decimation filter length
    unsigned char  decimationTimer;      // this is the decimation counter

    unsigned char  numChannels;			// Number of channels the sampler is valid to handle
    unsigned short *lastSample;          // last valid sample read buffers

    SensorDevice *sensor;				// The sensor associated to this sampler
};

#endif	/* SAMPLER_H */

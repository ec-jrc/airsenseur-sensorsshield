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


#ifndef HUMSENSORSAMPLER_H
#define	HUMSENSORSAMPLER_H

#include "Sampler.h"
#include "TempSensorSampler.h"

class TempSensorSampler;

class HumSensorSampler : public Sampler {
public:
    HumSensorSampler(TempSensorSampler* temperatureSampler);
    virtual ~HumSensorSampler();
    
    virtual void setPreScaler(unsigned char value);
    virtual unsigned char getPrescaler();
    virtual bool sampleTick();
    virtual bool sampleLoop();

    virtual bool saveChannelName(unsigned char myID, unsigned char* name);
    virtual bool getChannelName(unsigned char myID, unsigned char* buffer, unsigned char buffSize) const;

    virtual const char* getMeasurementUnit() const;
    virtual double evaluateMeasurement(unsigned short lastSample) const;

private:
    TempSensorSampler *tempSampler;     // This is a "fake" sampler and uses
                                        // an already retrieved value from the
                                        // TempSensorSampler. We opted for this
                                        // solution in order to have a better
                                        // efficiency (the UR100CD is an hybrid sensor
                                        // that provides two values)
};

#endif	/* HUMSENSORSAMPLER_H */


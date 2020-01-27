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

#ifndef SAMPLESAVERAGER_H
#define	SAMPLESAVERAGER_H

class DitherTool;

class SamplesAverager {
public:
    SamplesAverager();
    virtual ~SamplesAverager();
    
    unsigned char init(unsigned char size);
    void onStartSampling();
    bool collectSample(unsigned short sample, unsigned long _timestamp);
    unsigned char getBufferSize();
    
    virtual void setDitherTool(DitherTool* tool);    
    
    bool savePreset(unsigned char myID);
    bool loadPreset(unsigned char myID);
    
    unsigned short lastAveragedValue();
    unsigned long lastTimeStamp();
    
private:
    void reset();
    void countAverage();
    
private:
    unsigned char bufferSize;
    unsigned char sampleOffset;
    unsigned short* dataBuffer;
    
    unsigned long accumulator;
    unsigned short lastAverageSample;
    
    unsigned long timestamp;
    
    bool consolidated;
    
    static DitherTool* ditherTool;       // A static pointer to a singleton dithering tool object    
};

#endif	/* SAMPLESAVERAGER_H */


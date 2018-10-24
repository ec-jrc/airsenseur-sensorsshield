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

#ifndef APPAVERAGER_H
#define	APPAVERAGER_H

#ifdef	__cplusplus
extern "C" {
#endif
    
    typedef struct app_averagerdata_ {
        
        uint8_t maxBufferSize;
        uint8_t currentBufferSize;
        uint8_t sampleOffset;
        float* dataBuffer;

        float accumulator;
        float lastAverageSample;

        uint32_t timestamp;
    
        bool consolidated;

    } app_averagerdata;

    void APP_Avg_Init(app_averagerdata* averagerData, float* databuffer, uint8_t bufferSize);
    bool APP_Avg_SetBufferDeep(app_averagerdata* averagerData, uint8_t bufferDeep);
    bool APP_Avg_CollectSample(app_averagerdata* averagerData, float sample, unsigned long _timestamp);
    uint8_t APP_Avg_GetBufferDeep(app_averagerdata* averagerData);
    float APP_Avg_LastAveragedValue(app_averagerdata* averagerData);
    float APP_Avg_LastCumulatedValue(app_averagerdata* averagerData);
    uint32_t APP_Avg_LastTimeStamp(app_averagerdata* averagerData);
    

#ifdef	__cplusplus
}
#endif

#endif	/* APPAVERAGER_H */


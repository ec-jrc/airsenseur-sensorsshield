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

#ifndef APPSAMPLER_H
#define	APPSAMPLER_H

#ifdef	__cplusplus
extern "C" {
#endif
    
    typedef bool (*app_handler)(struct app_samplerdata_ *samplerData, bool go);
    typedef struct app_samplerdata_ {
        
        uint16_t prescaler;                     // basic samples are taken at sampleTick/prescaler
        uint16_t timer;                         // this is the prescaler counter
        uint8_t go;                             // it's time for a new sample
        uint32_t runningSampleTimeStamp;        // it's the absolute timestamp for the new sample (interrupt handled)
        uint32_t latchedSampleTimeStamp;       // it's the absolute timestamp used by the main loop
        
        // We manually implement a set of "members overload"
        app_handler taskHandler;
        
    } app_samplerdata;
    
    void APP_Sam_Init(app_samplerdata *samplerData);
    void APP_Sam_SetTaskFunction(app_samplerdata *samplerData, app_handler function);

    bool APP_Sam_Task(app_samplerdata *samplerData);
    bool APP_Sam_Tick(app_samplerdata *samplerData);

    void APP_Sam_SetPrescaler(app_samplerdata *samplerData, uint8_t value);
    uint8_t APP_Sam_GetPrescaler(app_samplerdata *samplerData);
    
#ifdef	__cplusplus
}
#endif

#endif	/* APPSAMPLER_H */


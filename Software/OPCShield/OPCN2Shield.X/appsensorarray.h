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

#ifndef APPSENSORARRAY_H
#define	APPSENSORARRAY_H

#ifdef	__cplusplus
extern "C" {
#endif

    void APP_SAR_Init(void);
    void APP_SAR_Task(void);
    void APP_SAR_Tick(void);
    
    bool APP_SAR_SetSamplePrescaler(uint8_t channel, uint8_t prescaler);
    bool APP_SAR_GetSamplePrescaler(uint8_t channel, uint8_t* prescaler);
    bool APP_SAR_SetSamplePostscaler(uint8_t channel, uint8_t postscaler);
    bool APP_SAR_GetSamplePostscaler(uint8_t channel, uint8_t* postscaler);
    bool APP_SAR_EnableSampling(bool enable);
    bool APP_SAR_InquirySensor(uint8_t channel, uint8_t* buffer, uint8_t bufSize);
    bool APP_SAR_GetLastSample(uint8_t channel, float *lastSample, uint32_t *lastTimestamp);
    
    bool APP_SAR_SavePreset(uint8_t channel);
    bool APP_SAR_LoadPreset(uint8_t channel);

#ifdef	__cplusplus
}
#endif

#endif	/* APPSENSORARRAY_H */


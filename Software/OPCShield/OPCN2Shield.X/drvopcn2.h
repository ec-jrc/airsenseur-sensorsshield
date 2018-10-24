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

#ifndef DRV_OPCN2_H
#define	DRV_OPCN2_H

#ifdef	__cplusplus
extern "C" {
#endif
    
    #define HISTOGRAM_BINS_NUM          16
    #define HISTOGRAM_MTOF_BINS_NUM     4
    
    typedef struct _opcn2_config_variables {
        uint16_t samplingIntervalCount;
        uint16_t idleIntervalCount;
        uint8_t fanOnInIdle;
        uint8_t laserOnInIdle;
        uint16_t maxDataArraysInFile;
        uint8_t onlySavePMData;
    } opcn2_config_variables;
    
    typedef struct _opcn2_pmdata {
        float pm1;
        float pm25;
        float pm10;
    } opcn2_pmdata;
    
    typedef struct _opcn2_histogram_data {
        uint16_t bins[HISTOGRAM_BINS_NUM];
        uint8_t mToF[HISTOGRAM_MTOF_BINS_NUM];
        float sampleFlowRate;
        uint32_t temperaturePressure;
        float samplingPeriod;
        uint16_t checksum;
        opcn2_pmdata pm;
    } opcn2_histogram_data;

    void DRV_OPCN2Init(void);
    void DRV_OPCN2Task(void);
    
    uint8_t DRV_OPCN2_Ready(void);
    
    void DRV_OPCN2_SetLaser(uint8_t on);
    void DRV_OPCN2_SetFan(uint8_t on);
    void DRV_OPCN2_SetFanAndLaser(uint8_t on);
    void DRV_OPCN2_SetFanPower(uint8_t power);
    
    void DRV_OPCN2_AskInformationString();
    char* DRV_OPCN2_GetInformationString();
    
    void DRV_OPCN2_AskSerialNumberString();
    char* DRV_OPCN2_GetSerialNumberString();

    void DRV_OPCN2_AskConfigurationVariables();
    opcn2_config_variables* DRV_OPCN2_GetConfigurationVariables();
    void DRV_OPCN2_WriteConfigurationVariables(opcn2_config_variables *config);
    
    void DRV_OPCN2_AskHistogramData();
    opcn2_histogram_data* DRV_OPCN2_GetHistogramData();
    
    void DRV_OPCN2_AskPMData();
    opcn2_pmdata* DRV_OPCN2_GetPMData();

#ifdef	__cplusplus
}
#endif

#endif	/* DRV_OPCN2_H */


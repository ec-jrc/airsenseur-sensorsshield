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

#ifndef DRVVZ89_H
#define	DRVVZ89_H

#ifdef	__cplusplus
extern "C" {
#endif

    typedef struct _vz89_data {
        uint32_t rawSensorValueTm1; // Previous sample data. Used to filter out glitches sometimes present in the sensor answer
        uint32_t rawSensorValue;
    } vz89_data;
    
    void I2CInit(void);
    void DRV_VZ89Init(void);
    void DRV_VZ89Task(void);
    
    uint8_t DRV_VZ89_Ready(void);
    
    void DRV_VZ89_AskData();
    vz89_data* DRV_VZ89_GetReadData();
    

#ifdef	__cplusplus
}
#endif

#endif	/* DRVVZ89_H */


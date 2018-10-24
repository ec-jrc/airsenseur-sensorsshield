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

#include "mcc_generated_files/mcc.h"
#include "appsampler.h"
#include "drvsystimer.h"

void APP_Sam_Init(app_samplerdata *samplerData) {
    if (samplerData != NULL) {
        samplerData->prescaler = 0;
        samplerData->timer = 0;
        samplerData->go = 0;
        samplerData->runningSampleTimeStamp = 0;
        samplerData->taskHandler = NULL;
    }
}

void APP_Sam_SetTaskFunction(app_samplerdata *samplerData, app_handler function) {
    if (!samplerData) {
        return;
    }
    
    samplerData->taskHandler = function;
}

bool APP_Sam_Task(app_samplerdata *samplerData) {

    INTERRUPT_GlobalInterruptHighDisable();
    bool result = samplerData->go;
    if (samplerData->go) {
        samplerData->go = false;
        samplerData->latchedSampleTimeStamp = samplerData->runningSampleTimeStamp;
    }
    INTERRUPT_GlobalInterruptHighEnable();
    
    if (samplerData->taskHandler) {
        result = samplerData->taskHandler(samplerData, result);
    } 
    
    return result;
}

// We expect this function called each 10 milliseconds
bool APP_Sam_Tick(app_samplerdata *samplerData) {
    
    if (samplerData->timer == (samplerData->prescaler-1)) {
        
        // It's time for a new sample
        samplerData->timer = 0;
        samplerData->go = true;
        samplerData->runningSampleTimeStamp = DRV_SysTimerGetTime();
        return true;
    }
    samplerData->timer++;
    
    return false;
}

void APP_Sam_SetPrescaler(app_samplerdata *samplerData, uint8_t value) {
    
    if (!samplerData) {
        return;
    }
    
    // We expect prescaler values in 1/10 seconds
    samplerData->prescaler = value * 10;
    samplerData->timer = 0;
}

uint8_t APP_Sam_GetPrescaler(app_samplerdata *samplerData) {
       if (!samplerData) {
        return 0;
    }
 
    return samplerData->prescaler / 10;
}

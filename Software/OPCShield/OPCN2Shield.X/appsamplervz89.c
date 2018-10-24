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
#include "appsamplervz89.h"

static vz89_data* customData;

bool APP_Sam_VZ89_TaskFunction(app_samplerdata* samplerData, bool go) {

    if (go) {
        DRV_VZ89_AskData();
        customData = NULL;
        return false;
    }
    
    customData = DRV_VZ89_GetReadData();
    return (customData != NULL);
}

vz89_data* APP_Sam_VZ89_GetCustomData(void) {
    return customData;
}

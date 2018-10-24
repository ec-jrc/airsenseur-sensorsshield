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
#include "apppersister.h"


void APP_Persister_Save8bit(uint16_t address, uint8_t data) {
    DATAEE_WriteByte(address, data);
}

uint8_t APP_Persister_Load8bit(uint16_t address) {
    return DATAEE_ReadByte(address);
}

void APP_Persister_Save16bit(uint16_t address, uint16_t data) {
    DATAEE_WriteByte(address, data&0xFF);
    DATAEE_WriteByte(address+1, (data>>8)&0xFF);
}

uint16_t APP_Persister_Load16bit(uint16_t address) {
    return DATAEE_ReadByte(address) | ((DATAEE_ReadByte(address+1)<<8)&0xFF00);
}


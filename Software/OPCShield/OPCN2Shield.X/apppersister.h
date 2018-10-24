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

#ifndef PERSISTER_H
#define	PERSISTER_H

    #define BASEADDRESS_SAMPLERS            0x0000
    #define BASEADDRESS_AVERAGERS_INNER     0x0010
    #define BASEADDRESS_AVERAGERS_OUTER     0x0110

    void APP_Persister_Save8bit(uint16_t address, uint8_t data);
    uint8_t APP_Persister_Load8bit(uint16_t address);
    
    void APP_Persister_Save16bit(uint16_t address, uint16_t data);
    uint16_t APP_Persister_Load16bit(uint16_t address);

#endif	/* PERSISTER_H */


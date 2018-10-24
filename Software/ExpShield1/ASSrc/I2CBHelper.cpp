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

#include <GlobalHalHandlers.h>
#include <I2CBHelper.h>

// Singleton I2CAHelper instance
I2CBHelper I2CBHelper::instance;

I2CBHelper::I2CBHelper() {
}

I2CBHelper::~I2CBHelper() {
}


bool I2CBHelper::write(unsigned short deviceAddress, unsigned short regAddress, unsigned char addrSize, unsigned char* pData, unsigned short size) const {

	HAL_StatusTypeDef res = HAL_I2C_Mem_Write(&hi2c2, deviceAddress, regAddress, addrSize, pData, size, 500);
	return (res == HAL_OK);
}

bool I2CBHelper::write(unsigned short deviceAddress, unsigned char *pData, unsigned short size) const {

	HAL_StatusTypeDef res = HAL_I2C_Master_Transmit(&hi2c2, deviceAddress, pData, size, 500);
	return (res == HAL_OK);
}

bool I2CBHelper::read(unsigned short deviceAddress, unsigned short regAddress, unsigned char addrSize, unsigned char* pData, unsigned short size) const {

	HAL_StatusTypeDef res = HAL_I2C_Mem_Read(&hi2c2, deviceAddress, regAddress, addrSize, pData, size, 500);
	return (res == HAL_OK);
}

bool I2CBHelper::read(unsigned short deviceAddress, unsigned char* pData, unsigned short size) const {
	HAL_StatusTypeDef res = HAL_I2C_Master_Receive(&hi2c2, deviceAddress, pData, size, 500);
	return (res == HAL_OK);
}

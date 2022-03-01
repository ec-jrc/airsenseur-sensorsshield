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

#ifndef I2CBHELPER_H_
#define I2CBHELPER_H_

#include "I2CHelper.h"

class I2CBHelper : public I2CHelper {

private:
	I2CBHelper();

public:
	virtual ~I2CBHelper();

public:
	static inline I2CBHelper* getInstance() { return &instance; }
	bool write(unsigned short deviceAddress, unsigned short regAddress, unsigned char addrSize, unsigned char* pData, unsigned short size) const;
	bool write(unsigned short deviceAddress, unsigned char *pData, unsigned short size) const;
	bool read(unsigned short deviceAddress, unsigned short regAddress, unsigned char addrSize, unsigned char* pData, unsigned short size) const;
	bool read(unsigned short deviceAddress, unsigned char* pData, unsigned short size) const;

private:
	static I2CBHelper instance;

};

#define I2CB (*(I2CBHelper::getInstance()))

#endif /* I2CBHELPER_H_ */

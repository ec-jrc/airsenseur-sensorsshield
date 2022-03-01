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

#ifndef EEPROMHELPER_H_
#define EEPROMHELPER_H_

#define MIN_TIME_BETWEEN_WRITING 1	/* Number of ticks between writing operations */

class EEPROMHelper {
private:
	EEPROMHelper();

public:
	virtual ~EEPROMHelper();

public:
	static inline EEPROMHelper* getInstance() { return &instance; }
	bool write(unsigned short address, unsigned char value);
	bool write(unsigned short address, unsigned char* pData, unsigned char size);
	unsigned char read(unsigned short address);
	bool read(unsigned short address, unsigned char* pData, unsigned char size);

	// Functions to be called externally in order to
	// properly handle the internal writer state machine/writing queue
	void tick();
	void mainLoop();

private:
	bool pushRequest(unsigned short address, unsigned char* pData, unsigned char size);

private:
	typedef struct _datapacket {
		unsigned short address;
		unsigned char* pData;
		unsigned char size;

		_datapacket* nextItem;
	} datapacket;

private:
	static EEPROMHelper instance;

	datapacket* writingItem;
	datapacket* lastItem;
	volatile unsigned char timer;
	unsigned char writingTimestamp;
};

#define EEPROM (*(EEPROMHelper::getInstance()))

#endif /* EEPROMHELPER_H_ */

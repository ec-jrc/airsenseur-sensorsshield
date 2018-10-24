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

#include <string.h>

#include "OPCN3Comm.h"
#include "GPIOHelper.h"
#include "SPIHelper.h"

#define OPC_RES_BUSY			0x31
#define OPC_RES_READY		0xF3
#define POLYNOMIAL 			0xA001 //Generator polynomial for CRC

const OPCN3Comm::cmdinfo OPCN3Comm::cmdInfoList[]  = {
		{ W_PERIPH_POWER_STATUS, 0x00, 0x01, IDLE, 0x02 }, /* Set Fan Dig Pot Off */
		{ W_PERIPH_POWER_STATUS, 0x00, 0x01, IDLE, 0x03 }, /* Set Fan Dig Pot On */
		{ W_PERIPH_POWER_STATUS, 0x00, 0x01, IDLE, 0x04 }, /* Set Laser Dig Pot Off */
		{ W_PERIPH_POWER_STATUS, 0x00, 0x01, IDLE, 0x05 }, /* Set Laser Dig Pot On */
		{ W_PERIPH_POWER_STATUS, 0x00, 0x01, IDLE, 0x06 }, /* Set Laser Switch Off */
		{ W_PERIPH_POWER_STATUS, 0x00, 0x01, IDLE, 0x07 }, /* Set Laser Switch On */
		{ R_HIST_DATA, sizeof(OPCN3Comm::histogram), 0x00, EVALUATE, 0x00 },		  /* Read histogram */
};

OPCN3Comm::OPCN3Comm() {
	resetStateMachine();

	memset(smData.buffer, 0, OPCN3_RXBUFFER);

	AS_GPIO.digitalWrite(OPC_SS, true);
}

OPCN3Comm::~OPCN3Comm() {
}

void OPCN3Comm::loop() {

	switch (smData.status) {

	case IDLE:
	case EVALUATE:
		break;

	case WAITING_READY: {

		if (smData.timer > 3) {
			unsigned char rxData = write(cmdInfoList[smData.currCmdOffset].cmdStatement);
			smData.timer = 0;
			smData.retry++;
			if (rxData == OPC_RES_READY) {

				if (cmdInfoList[smData.currCmdOffset].rxLength != 0) {
					smData.currRxDataOffset = 0;
					while (smData.currRxDataOffset < cmdInfoList[smData.currCmdOffset].rxLength) {
						smData.buffer[smData.currRxDataOffset] = write(cmdInfoList[smData.currCmdOffset].cmdStatement);
						smData.currRxDataOffset++;
					}
				} else {
					smData.currTxDataOffset = 0;
					if (write(cmdInfoList[smData.currCmdOffset].data) == OPC_RES_BUSY) {
						if (smData.retry > 150) {
							resetStateMachine();
						}
						break;
					}
				}

				smData.status = cmdInfoList[smData.currCmdOffset].fallbackStatus;
				if (smData.status == IDLE) {
					resetStateMachine();
				}

			} else {

				if (smData.retry > 150) {
					resetStateMachine();
				}
			}
		}
	}
	break;

	default:
		resetStateMachine();
		break;
	}
}

void OPCN3Comm::tick() {
	smData.timer++;
}


bool OPCN3Comm::triggerCommand(cmdOffset cmdOffset) {

	if ((smData.status != IDLE) && (cmdOffset >= NONE)) {
		return false;
	}

	smData.currCmdOffset = cmdOffset;
	smData.timer = 0;
	smData.retry = 0;
	smData.status = WAITING_READY;

	return true;
}

bool OPCN3Comm::ready() {
	return (smData.status == IDLE);
}

void OPCN3Comm::resetStateMachine() {
	smData.status = IDLE;
	smData.currCmdOffset = NONE;
	smData.timer = 0;
	smData.retry = 0;
	smData.currRxDataOffset = 0;
	smData.currTxDataOffset = 0;
}

unsigned char OPCN3Comm::write(unsigned char data) {

    unsigned char result;

    AS_GPIO.digitalWrite(OPC_SS, false);
    result = AS_SPI.transfer(data);
    AS_GPIO.digitalWrite(OPC_SS, true);

    return result;
}

OPCN3Comm::histogram* OPCN3Comm::getLastHistogram() {

	if ((smData.status == EVALUATE) && (smData.currCmdOffset == READ_HISTOGRAM)) {

		unsigned short crc = CalcCRC(smData.buffer, sizeof(histogram)-2);
		resetStateMachine();

		histogram* recHistogram = (histogram*)smData.buffer;
		if (crc == recHistogram->checksum) {
			return recHistogram;
		}
	}

	return NULL;
}


float OPCN3Comm::toPmValue(type32 *data) {

	// Convert incoming buffer into float value representation
	unsigned char* fPointer = (unsigned char*)&fHelper;
	for (unsigned char n = 0; n < 4; n++, fPointer++) {
		*fPointer = data->x[n];
	}

	if (fHelper < 0) {
		// This should never happens, but it's better to have a check here
		return 0.0f;
	}

	return fHelper;
}


unsigned short OPCN3Comm::CalcCRC(unsigned char data[], unsigned char nbrOfBytes) {
	unsigned char _bit;
	unsigned short crc = 0xFFFF;
	unsigned char byteCtr;

	for (byteCtr = 0; byteCtr < nbrOfBytes; byteCtr++) {
		crc ^= (unsigned int) data[byteCtr];
		for (_bit = 0; _bit < 8; _bit++) {
			if (crc & 1) //if bit0 of crc is 1
					{
				crc >>= 1;
				crc ^= POLYNOMIAL;
			} else
				crc >>= 1;
		}
	}
	return crc;
}

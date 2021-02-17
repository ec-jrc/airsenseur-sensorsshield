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
#include "SensorBusWrapper.h"
#include "CommProtocol.h"
#include "SerialBHelper.h"
#include "CRC32Helper.h"

#define SET_PROTOCOL_VERSION_IN_HEADER(v) txHeader[1] = (v)

char SensorBusWrapper::txHeader[]  = { COMMPROTOCOL_PTM_SLAVE_HEADER, COMMPROTOCOL_PTM_VERSION_ZERO,
									COMMPROTOCOL_MYBOARDID_MSB,	COMMPROTOCOL_BOARDID_BROADCAST, 0x00 };


SensorBusWrapper::SensorBusWrapper(CommProtocol* lowLayerProtocol)
	: rxStatus(IDLE), myBoardId(0), rxBoardId(COMMPROTOCOL_BOARDID_BROADCAST), rxProtocolVersion(COMMPROTOCOL_PTM_VERSION_ZERO),
	  timerCounter(0), rxProtocolTimeout(0), commProtocol(lowLayerProtocol)  {
}

SensorBusWrapper::~SensorBusWrapper() {
}

void SensorBusWrapper::init(unsigned char boardID) {

    myBoardId = boardID & 0x0F;

    // Evaluate the boardId LSB for future use
    char rxBoardIdLSB = '0' + myBoardId;
    if ((myBoardId > 9) && (myBoardId < 16)) {
        rxBoardIdLSB = 'A' + myBoardId - 10;
    }

    // Update the header, used to answer, with the board ID information
    txHeader[3] = rxBoardIdLSB;

    // Initialize all other variables
    rxStatus = IDLE;
    rxBoardId = COMMPROTOCOL_BOARDID_BROADCAST;

    timerCounter = 0;
    rxProtocolTimeout = 0;
}

void SensorBusWrapper::timerTick() {
    timerCounter++;
}

void SensorBusWrapper::onDataReceived(unsigned char rxChar) {

    // Whatever is my status, if an header is found, go to the IDLE status
    if (rxChar == COMMPROTOCOL_PTM_HOST_HEADER) {
        rxStatus = IDLE;
    }

    // When not in IDLE, evaluate a timeout
    // (we expect a max defined period between start and end frame)
    if (rxStatus  != IDLE) {
        if ((timerCounter - rxProtocolTimeout) > SENSORBUS_FRAME_TIMEOUT) {
            rxStatus = IDLE;
        }
    }

    // Evaluate the received char based on current state machine status
    switch (rxStatus) {

        case IDLE: {
            // Searching for an header
            if (rxChar == COMMPROTOCOL_PTM_HOST_HEADER) {
                rxStatus = HEADER_FOUND;
                rxBoardId = COMMPROTOCOL_BOARDID_BROADCAST;
                rxProtocolTimeout = timerCounter;
            }
        }
        break;

        case HEADER_FOUND: {
            if ((rxChar == COMMPROTOCOL_PTM_VERSION_ZERO) ||
            	(rxChar == COMMPROTOCOL_PTM_VERSION_ONE)) {
                // We expect a compatible protocol version
            	rxProtocolVersion = rxChar;
                rxStatus = VERSION_FOUND;
            } else {
                // ... but we found something not valid...
                rxStatus = IDLE;
            }
        }
        break;

        case VERSION_FOUND: {
            // We expect an hex digit, msb of target boardId
            rxStatus = BOARDID1_FOUND;
            if ((rxChar >= '0') && (rxChar <= '9')) {
                rxBoardId = (rxChar - '0') << 4;
            } else if ((rxChar >= 'a') && (rxChar <= 'f')) {
                rxBoardId = (rxChar - 'a' + 10) << 4;
            } else if ((rxChar >= 'A') && (rxChar <= 'F')) {
                rxBoardId = (rxChar - 'A' + 10) << 4;
            } else {
                rxStatus = IDLE;
            }
        }
        break;

        case BOARDID1_FOUND: {
            // We expect an hex digit, lsb of target boardId
            rxStatus = BOARDID_FOUND;
            if ((rxChar >= '0') && (rxChar <= '9')) {
                rxBoardId |= (rxChar - '0');
            } else if ((rxChar >= 'a') && (rxChar <= 'f')) {
                rxBoardId |= (rxChar - 'a' + 10);
            } else if ((rxChar >= 'A') && (rxChar <= 'F')) {
                rxBoardId |= (rxChar - 'A' + 10);
            } else {
                rxStatus = IDLE;
            }

            // Validate the received boardId
            if (myBoardId != rxBoardId) {
                rxStatus = IDLE;
            } else {

                // Start sending the header to the lower layer
                commProtocol->onDataReceived(COMMPROTOCOL_SHIELDPAYLOAD_HEADER, CommProtocol::SOURCE_SENSORBUS);
            }
        }
        break;

        case BOARDID_FOUND: {
            // Searching for a trailer
            if (rxChar == COMMPROTOCOL_PTM_HOST_TRAILER) {

                // End of frame found.
                // Send the trailer to the shield
            		commProtocol->onDataReceived(COMMPROTOCOL_SHIELDPAYLOAD_TRAILER, CommProtocol::SOURCE_SENSORBUS);

                // Enable answer data transfer going in idle state
                rxStatus = IDLE;

            } else {
                // All received data are considered as payload
                // and will be echoed to the shield serial line
            		commProtocol->onDataReceived(rxChar, CommProtocol::SOURCE_SENSORBUS);
            }
        }
        break;

        default: {
            rxStatus = IDLE;
        }
    }
}

#define NIBBLEBINTOHEX(a) ((a)>0x09)?(((a)-0x0A)+'A'):((a)+'0');

unsigned short SensorBusWrapper::write(char* buffer) const {

	// Start sending the SensorBus header
	SET_PROTOCOL_VERSION_IN_HEADER(rxProtocolVersion);
	SerialB.write(txHeader);

	// Replace the low level trailer with the sensor bus trailer
	size_t len = strlen(buffer);
	buffer[len-1] = COMMPROTOCOL_PTM_SLAVE_TRAILER;

	// Send the payload, with no low level header, and the trailer
	if (rxProtocolVersion == COMMPROTOCOL_PTM_VERSION_ZERO) {
		return SerialB.write(buffer+1) + 4;
	} else {

		// Calculate the CRC of the payload
		long crc = CRC32.getCRC32((long*)(buffer+1), len-2);

		// Remove the trailer from the original buffer
		buffer[len-1] = '\0';

		// Convert to ASCII
		char valBuf[10];
		valBuf[0] = NIBBLEBINTOHEX(((crc>>28) & 0x0F));
		valBuf[1] = NIBBLEBINTOHEX(((crc>>24) & 0x0F));
		valBuf[2] = NIBBLEBINTOHEX(((crc>>20) & 0x0F));
		valBuf[3] = NIBBLEBINTOHEX(((crc>>16) & 0x0F));
		valBuf[4] = NIBBLEBINTOHEX(((crc>>12) & 0x0F));
		valBuf[5] = NIBBLEBINTOHEX(((crc>>8) & 0x0F));
		valBuf[6] = NIBBLEBINTOHEX(((crc>>4) & 0x0F));
		valBuf[7] = NIBBLEBINTOHEX((crc & 0x0F));
		valBuf[8] = COMMPROTOCOL_PTM_SLAVE_TRAILER;
		valBuf[9] = '\0';

		// Send the payload followed by the CRC-32 and the trailer
		return 4 + SerialB.write(buffer+1) + SerialB.write(valBuf);
	}
}

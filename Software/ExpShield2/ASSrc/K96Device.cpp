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
 *      European Commission - Joint Research Centre,
 * - Marco Signorini, marco.signorini@liberaintentio.com
 *
 * ===========================================================================
 */

#include "K96Device.h"
#include "SerialCHelper.h"
#include "GPIOHelper.h"
#include "IntChamberTempRef.h"
#include <string.h>

#define K96_MODBUS_TIMEOUT		19		/* We expect K96 answers in less 0.2 seconds */
#define K96_CMD_TIMEOUT			49		/* ModBus command retry timeout (half a second) */
#define K96_MAX_RETRY			9		/* Maximum ModBus command retries before error */
#define K96_POWERUP_TIME		99		/* 1 second timer for K96 powerup */
#define K96_BLANK_TIME			2		/* 20 millisecond blank time after each answers */

#define K96_MODBUS_ANYS_ADDRESS	0xFE
#define K96_MODBUS_DEFA_ADDRESS	0x68

#define K96_MODBUS_ADDRESS		K96_MODBUS_ANYS_ADDRESS
#define	K96_MODBUS_CRC_INITVAL	0xFFFF

#define K96_CMD_READ_INPUTREG	0x04
#define K96_CMD_READ_FROMRAM	0x44
#define K96_CMD_READ_FROMEEPROM	0x46
#define K96_CMD_WRITE_TORAM		0x41

#define K96_CMD_ERROR_CODE_FLAG	0x80
#define K96_NO_ERRORS			0x00

#define K96_REG_LPL_CONCPC		0x00
#define K96_REG_SPL_CONCPC		0x01
#define K96_REG_MPL_CONCPC		0x02
#define K96_REG_P_SENSOR0		0x03
#define K96_REG_NTC0_TEMP		0x04
#define K96_REG_NTC1_TEMP		0x05
#define K96_REG_ADUCDIE_TEMP	0x07
#define K96_REG_RH_SENSOR0		0x08
#define K96_REG_RH_T_SENSOR0	0x09
#define K96_REG_ERRORSTATUS		0x0E

#define K96_FLAG_FATAL_ERROR			0x0001
#define K96_FLAG_CONFIGURATION_ERROR	0x0004
#define K96_FLAG_CALIBRATION_ERROR		0x0008
#define K96_FLAG_SELFDIAG_ERROR			0x0010
#define K96_FLAG_MEMORY_ERROR			0x0040
#define K96_FLAG_WARMUP					0x0080
#define K96_FLAG_LOGGER_ERROR			0x0100
#define K96_FLAG_SPI_ERROR				0x0200
#define K96_FLAG_NTC_TEMP_ERROR			0x0400
#define K96_FLAG_UCDIE_TEMP_ERROR		0x0800
#define K96_FLAG_MPL_CALC_ERROR			0x2000
#define K96_FLAG_SPL_CALC_ERROR			0x4000
#define K96_FLAG_LPL_CALC_ERROR			0x8000

#define K96_RAM_METERID					0x0028
#define K96_RAM_MPL_UFLT_IR_SIGNAL		0x0384
#define K96_RAM_LPL_UFLT_IR_SIGNAL		0x0424
#define K96_RAM_SPL_UFLT_IR_SIGNAL		0x0484


#define K96_MEASUREMENTS_NUMREG	10
#define K96_ERRORSTATUS_REGSIZE	1

#define NIBBLEBINTOHEX(a) ((a)>0x09)?(((a)-0x0A)+'A'):((a)+'0');

const char* const K96Device::channelNames[] {
		"LPLCPC", "SPLCPC", "MPLCPC",
		"PSEN0", "TNTC0", "TNTC1", "TUCDIE", "RH0", "TRH0",
		"UFLPIR", "UFSPIR", "UFMPIR",
		"ERRST", "LPLUER", "SPLUER", "MPLUER"
};

const char* const K96Device::channelMeasurementUnits[] = {
		"ppm", "ppm", "ppm",
		"hPa", "C", "C", "C", "%", "C",
		"u16", "u16", "u16",
		"bit", "bit", "bit", "bit"
};

// ROM table for CRC calculation speedup
// It was left global to avoid ROM to RAM copy at runtime by the compiler,
// due to the heap allocation of the K96Device instance. A suitable static
// reference pointer in the K96Device object is initialized below.
const unsigned short _crcTable[] = {
	0x0000,	0xc0c1, 0xc181, 0x0140, 0xc301, 0x03c0, 0x0280, 0xc241,
	0xc601, 0x06c0, 0x0780, 0xc741, 0x0500, 0xc5c1, 0xc481, 0x0440,
	0xcc01, 0x0cc0, 0x0d80, 0xcd41, 0x0f00, 0xcfc1, 0xce81, 0x0e40,
	0x0a00, 0xcac1, 0xcb81, 0x0b40, 0xc901, 0x09c0, 0x0880, 0xc841,
	0xd801, 0x18c0, 0x1980, 0xd941, 0x1b00, 0xdbc1, 0xda81, 0x1a40,
	0x1e00, 0xdec1, 0xdf81, 0x1f40, 0xdd01, 0x1dc0, 0x1c80, 0xdc41,
	0x1400, 0xd4c1, 0xd581, 0x1540, 0xd701, 0x17c0, 0x1680, 0xd641,
	0xd201, 0x12c0, 0x1380, 0xd341, 0x1100, 0xd1c1, 0xd081, 0x1040,
	0xf001, 0x30c0, 0x3180, 0xf141, 0x3300, 0xf3c1, 0xf281, 0x3240,
	0x3600, 0xf6c1, 0xf781, 0x3740, 0xf501, 0x35c0, 0x3480, 0xf441,
	0x3c00, 0xfcc1, 0xfd81, 0x3d40, 0xff01, 0x3fc0, 0x3e80, 0xfe41,
	0xfa01, 0x3ac0, 0x3b80, 0xfb41, 0x3900, 0xf9c1, 0xf881, 0x3840,
	0x2800, 0xe8c1, 0xe981, 0x2940, 0xeb01, 0x2bc0, 0x2a80, 0xea41,
	0xee01, 0x2ec0, 0x2f80, 0xef41, 0x2d00, 0xedc1, 0xec81, 0x2c40,
	0xe401, 0x24c0, 0x2580, 0xe541, 0x2700, 0xe7c1, 0xe681, 0x2640,
	0x2200, 0xe2c1, 0xe381, 0x2340, 0xe101, 0x21c0, 0x2080, 0xe041,
	0xa001, 0x60c0, 0x6180, 0xa141, 0x6300, 0xa3c1, 0xa281, 0x6240,
	0x6600, 0xa6c1, 0xa781, 0x6740, 0xa501, 0x65c0, 0x6480, 0xa441,
	0x6c00, 0xacc1, 0xad81, 0x6d40, 0xaf01, 0x6fc0, 0x6e80, 0xae41,
	0xaa01, 0x6ac0, 0x6b80, 0xab41, 0x6900, 0xa9c1, 0xa881, 0x6840,
	0x7800, 0xb8c1, 0xb981, 0x7940, 0xbb01, 0x7bc0, 0x7a80, 0xba41,
	0xbe01, 0x7ec0, 0x7f80, 0xbf41, 0x7d00, 0xbdc1, 0xbc81, 0x7c40,
	0xb401, 0x74c0, 0x7580, 0xb541, 0x7700, 0xb7c1, 0xb681, 0x7640,
	0x7200, 0xb2c1, 0xb381, 0x7340, 0xb101, 0x71c0, 0x7080, 0xb041,
	0x5000, 0x90c1, 0x9181, 0x5140, 0x9301, 0x53c0, 0x5280, 0x9241,
	0x9601, 0x56c0, 0x5780, 0x9741, 0x5500, 0x95c1, 0x9481, 0x5440,
	0x9c01, 0x5cc0, 0x5d80, 0x9d41, 0x5f00, 0x9fc1, 0x9e81, 0x5e40,
	0x5a00, 0x9ac1, 0x9b81, 0x5b40, 0x9901, 0x59c0, 0x5880, 0x9841,
	0x8801, 0x48c0, 0x4980, 0x8941, 0x4b00, 0x8bc1, 0x8a81, 0x4a40,
	0x4e00, 0x8ec1, 0x8f81, 0x4f40, 0x8d01, 0x4dc0, 0x4c80, 0x8c41,
	0x4400, 0x84c1, 0x8581, 0x4540, 0x8701, 0x47c0, 0x4680, 0x8641,
	0x8201, 0x42c0, 0x4380, 0x8341, 0x4100, 0x81c1, 0x8081, 0x4040
};

// This pointer is not really needed but is introduced to maintain a good C++ programming style
const unsigned short* K96Device::crcTable = _crcTable;

K96Device::K96Device() : SensorDevice(K96_NUM_OF_CHANNELS) {

	// Initialize the SerialC line
	SerialC.init();

	// PowerOff the unit
	powerOn(false);

	// Initialize the serial ID
	strcpy(serialID, "NA");
}

K96Device::~K96Device() {
}


void K96Device::powerOn(bool on) {

	AS_GPIO.digitalWrite(K96_EN, on);
	if (on) {
		powerUpTimer = K96_POWERUP_TIME;
	} else {
		if (status != UNAVAILABLE) {
			status = IDLE_READY;
		}
	}
}

bool K96Device::triggerCheckPresence() {

	// Start reading the unit ID stored in the sensor
	retry = 0;
	status = READ_UNIT_ID;

	return true;
}


void K96Device::triggerReadInputRegister(unsigned short address, unsigned char numRegisters) {

	unsigned char txBuf[8];

	txBuf[0] = K96_MODBUS_ADDRESS;
	txBuf[1] = K96_CMD_READ_INPUTREG;
	txBuf[2] = (address>>8) & 0xFF;
	txBuf[3] = address & 0xFF;
	txBuf[4] = 0x00;
	txBuf[5] = numRegisters;

	// Append CRC
	countCRC(txBuf, 6, false);

	// Initialize the Rx state machine
	rxTimeoutTimer = 0;
	cmdTimeoutTimer = 0;
	expFunctionCode = txBuf[1];
	lastExceptionCode = K96_NO_ERRORS;
	rxStatus = WAITING_DEVICE_ADDRESS;

	// Send to the device through UART
	SerialC.write((char*)txBuf, 8);
}

void K96Device::triggerReadRAM(unsigned short address, unsigned char byteSize) {

	unsigned char txBuf[8];

	txBuf[0] = K96_MODBUS_ADDRESS;
	txBuf[1] = K96_CMD_READ_FROMRAM;
	txBuf[2] = (address>>8) & 0xFF;
	txBuf[3] = address & 0xFF;
	txBuf[4] = byteSize;

	// Append CRC
	countCRC(txBuf, 5, false);

	// Initialize the Rx state machine
	rxTimeoutTimer = 0;
	cmdTimeoutTimer = 0;
	expFunctionCode = txBuf[1];
	lastExceptionCode = K96_NO_ERRORS;
	rxStatus = WAITING_DEVICE_ADDRESS;

	// Send to the device through UART
	SerialC.write((char*)txBuf, 7);
}

void K96Device::triggerWriteRAM(unsigned short address, unsigned char value) {

	unsigned char txBuf[9];

	txBuf[0] = K96_MODBUS_ADDRESS;
	txBuf[1] = K96_CMD_WRITE_TORAM;
	txBuf[2] = (address>>8) & 0xFF;
	txBuf[3] = address & 0xFF;
	txBuf[4] = 1;
	txBuf[5] = value;

	// Append CRC
	countCRC(txBuf, 6, false);

	rxTimeoutTimer = 0;
	cmdTimeoutTimer = 0;
	expFunctionCode = txBuf[1];
	lastExceptionCode = K96_NO_ERRORS;
	rxStatus = WAITING_DEVICE_ADDRESS;

	// Send to the device through UART
	SerialC.write((char*)txBuf, 8);
}

// Hadle the low level modbus communications by receiving
// data from the K96 sensor through the serial line.
// The function returns true if a valid packet has been received from the sensor
bool K96Device::onDataReceived(unsigned char rxChar) {

	// Handle incoming chars based on internal rx state machine
	unsigned char* pRxBuffer = (unsigned char*)&rxBuffer;
	switch(rxStatus) {
		case IDLE: {
			return false;
		}

		case WAITING_DEVICE_ADDRESS: {
			if ((rxChar == K96_MODBUS_ANYS_ADDRESS) ||
					(rxChar == K96_MODBUS_DEFA_ADDRESS)) {
				currRxOffset = 0;
				curRxPayloadBytes = 0;
				*(pRxBuffer+currRxOffset) = rxChar;
				rxStatus = WAITING_FUNCTION_CODE;
			}
			return false;
		}

		case WAITING_FUNCTION_CODE: {
			if (rxChar == expFunctionCode) { // All done. Proceed.
				rxStatus = (expFunctionCode == K96_CMD_WRITE_TORAM)? WAITING_CRC0 : WAITING_DATALENGTH;
				expByteLength = 0;
				++currRxOffset;
				*(pRxBuffer+currRxOffset) = rxChar;
			} else if (rxChar == expFunctionCode || K96_CMD_ERROR_CODE_FLAG) { // An error in the call parameters
				rxStatus = WAITING_EXEPTION_CODE;
				++currRxOffset;
				*(pRxBuffer+currRxOffset) = rxChar;
			} else {
				rxStatus = IDLE;
			}
			return false;
		}

		case WAITING_EXEPTION_CODE: {
			*(pRxBuffer+currRxOffset) = rxChar;
			lastExceptionCode = rxChar;
			rxStatus = WAITING_CRC0;
			return false;
		}

		case WAITING_DATALENGTH: {
			expByteLength = rxChar;
			if (expByteLength < (K96_RX_BUFFERLENGTH-2)) {
				++currRxOffset;
				*(pRxBuffer+currRxOffset) = rxChar;
				rxStatus = WAITING_DATA;
			} else {
				rxStatus = IDLE;
			}
			return false;
		}

		case WAITING_DATA: {
			++currRxOffset;
			*(pRxBuffer+currRxOffset) = rxChar;
			curRxPayloadBytes++;
			if (curRxPayloadBytes == expByteLength) {
				rxStatus = WAITING_CRC0;
			}
			return false;
		}

		case WAITING_CRC0: {
			++currRxOffset;
			*(pRxBuffer+currRxOffset) = rxChar;
			rxStatus = WAITING_CRC1;
			return false;
		}

		case WAITING_CRC1: {
			++currRxOffset;
			*(pRxBuffer+currRxOffset) = rxChar;

			rxStatus = IDLE;

			// Rx packet completed. Check for CRC validity
			return countCRC(pRxBuffer, currRxOffset-1, true);
		}

		default:
			break;
	}

	return false;
}


void K96Device::renderUnitID(unsigned int unitID) {

	// Convert in the XX:XX:XX:XX form the unit ID
	union {
		unsigned char unitIDs[4];
		unsigned int unitID;
	} value;
	value.unitID = unitID;

	for (unsigned char n = 0 ,m = 0; n < 4; n++, m++) {
		serialID[m] = NIBBLEBINTOHEX(((value.unitIDs[n]>>4) & 0x0F));
		serialID[++m] = NIBBLEBINTOHEX((value.unitIDs[n] & 0x0F));
		serialID[++m] = ':';
	}

	serialID[K96_SERIALID_BUFFERLENGTH-1] = 0x00;
}


// Count or check the CRC located in the buffer at location payloadSize+1 and payloadSize+2
// If checkCRC is true, the counted CRC is checked with one provided in the buffer and
// the result check is returned by the function.
// If checkCRC is false, this function populates the CRC fields in the buffer and always
// returns true
bool K96Device::countCRC(unsigned char *buffer, unsigned short payloadSize, bool checkCRC) {

	// Calculate the CRC based on data located in the buffer
	unsigned short crc = modBusCRC(buffer, payloadSize);

	// CRC checking has been required
	if (checkCRC) {
		return ((buffer[payloadSize] == (crc & 0xFF)) &&
				buffer[payloadSize+1] == ((crc >> 8) & 0xFF));
	}

	// CRC counting has been required. Apply to the end of the buffer
	buffer[payloadSize] = (crc & 0xFF);
	buffer[payloadSize+1] = ((crc >> 8) & 0xFF);


	return true;
}


// Calculates a ModBus compatible CRC
unsigned short K96Device::modBusCRC(const unsigned char *buffer, unsigned char bufLength) {

	unsigned short result = K96_MODBUS_CRC_INITVAL;
	if (buffer == NULL) {
		return result;
	}

	for (unsigned char n = 0; n < bufLength; n++) {
		result = (result >> 8) ^ crcTable[(result ^ (unsigned short) *buffer++) & 0x00FF];
	}

	return result;
}

void K96Device::onStartSampling() {
	SensorDevice::onStartSampling();
	powerOn(true);
}

void K96Device::onStopSampling() {
	powerOn(false);
}

void K96Device::setLowPowerMode(bool lowPower) {
}

bool K96Device::init() {

	powerOn(true);
	triggerCheckPresence();

	return true;
}

#define SWAP_BYTES(a) (((((a)&0xFF00)>>8)&0xFF) | ((((a)&0xFF)<<8)&0xFF00))
#define FROM_SHORT_TO_UNSIGNED_SHORT(a) ((unsigned short)((a) + 0x8000))
#define FROM_UNSIGNED_SHORT_TO_SIGNED_SHORT(a) ((short)((a) - 0x8000))
#define VALIDATE(errorStatus, flag, value) (((errorStatus) & (flag))? 0.0f : (value))

void K96Device::loop() {

	// Low level communication timeouts
	if ((rxStatus != IDLE) && (rxTimeoutTimer > K96_MODBUS_TIMEOUT)) {
		rxTimeoutTimer = 0;
		rxStatus = IDLE;
	}

	// Check for incoming data
	bool packetReceived = false;
	if (SerialC.available()) {
		unsigned char rxChar = SerialC.read();
		packetReceived = onDataReceived(rxChar);
	}

	// High level communication timeouts
	bool cmdTimeout = false;
	if ((status != IDLE_READY) && (status != UNAVAILABLE) &&
			(cmdTimeoutTimer > K96_CMD_TIMEOUT) && !packetReceived) {

		cmdTimeoutTimer = 0;
		cmdTimeout = true;
	}

	// Unit is powering up. Wait.
	if (powerUpTimer) {
		return;
	}

	// Process incoming data based on current FMS
	switch (status) {

		case UNAVAILABLE:
		case IDLE_READY:
			break;

		// Ask for a complete packet sample
		case START_SAMPLING: {
			triggerReadInputRegister(K96_REG_LPL_CONCPC, K96_MEASUREMENTS_NUMREG);
			status = WAITING_SAMPLE;
		}
		break;

		// Waiting for the complete packet sample
		case WAITING_SAMPLE: {
			if (packetReceived) {

				// Evaluate the received packet ad store samples
				short tNtc0 = SWAP_BYTES(rxBuffer.payload.measures.ntc0_Temp_flt);
				short tNtc1 = SWAP_BYTES(rxBuffer.payload.measures.ntc1_Temp_flt);
				short tRh = SWAP_BYTES(rxBuffer.payload.measures.rh_T_Sensor0);

				setSample(K96_CHANNEL_LPL_PC_FLT, FROM_SHORT_TO_UNSIGNED_SHORT(SWAP_BYTES(rxBuffer.payload.measures.lpl_ConcPC_flt)));
				setSample(K96_CHANNEL_SPL_PC_FLT, FROM_SHORT_TO_UNSIGNED_SHORT(SWAP_BYTES(rxBuffer.payload.measures.spl_ConcPC_flt)));
				setSample(K96_CHANNEL_MPL_PC_FLT, FROM_SHORT_TO_UNSIGNED_SHORT(SWAP_BYTES(rxBuffer.payload.measures.mpl_ConcPC_flt)));
				setSample(K96_CHANNEL_PRESS0, FROM_SHORT_TO_UNSIGNED_SHORT(SWAP_BYTES(rxBuffer.payload.measures.p_Sensor0_flt)));
				setSample(K96_CHANNEL_TEMP_NTC0, FROM_SHORT_TO_UNSIGNED_SHORT(tNtc0));
				setSample(K96_CHANNEL_TEMP_NTC1, FROM_SHORT_TO_UNSIGNED_SHORT(tNtc1));
				setSample(K96_CHANNEL_TEMP_UCDIE, FROM_SHORT_TO_UNSIGNED_SHORT(SWAP_BYTES(rxBuffer.payload.measures.aduCDie_Temp_flt)));
				setSample(K96_CHANNEL_RH0,FROM_SHORT_TO_UNSIGNED_SHORT(SWAP_BYTES(rxBuffer.payload.measures.rh_Sensor0)));
				setSample(K96_CHANNEL_T_RH0, FROM_SHORT_TO_UNSIGNED_SHORT(tRh));
				setSample(K96_CHANNEL_ERRORSTATUS,lastErrorStatus);

				// Propagate the internal chamber temperature to the temperature
				// reference control helper. Note: K96 already reports temperatures in 1/100 C
				// as required by the reference control helper
				AS_INTCH_TEMPREF.setReadTemperature(IntChamberTempRef::SOURCE_K96_TEMP_NTC0, tNtc0);
				AS_INTCH_TEMPREF.setReadTemperature(IntChamberTempRef::SOURCE_K96_TEMP_NTC1, tNtc1);
				AS_INTCH_TEMPREF.setReadTemperature(IntChamberTempRef::SOURCE_K96_T_RH0, tRh);

				status = IDLE_READY;

			} else if (cmdTimeout) {
				retry++;
				if (retry < K96_MAX_RETRY) {
					status = START_SAMPLING;
				} else {

					// Mark the unit as unavailable
					status = UNAVAILABLE;
					powerOn(false);
				}
			}
		}
		break;

		// Ask for LPL_UFLT_IR RAM register
		case READ_LPL_UFLT_IR: {
			triggerReadRAM(K96_RAM_LPL_UFLT_IR_SIGNAL, sizeof(rxBuffer.payload.xPL_uflt_ram_con_cal));
			status = WAITING_LPL_UFLT_IR;
		}
		break;

		// Ask for SPL_UFLT_IR RAM register
		case READ_SPL_UFLT_IR: {
			triggerReadRAM(K96_RAM_SPL_UFLT_IR_SIGNAL, sizeof(rxBuffer.payload.xPL_uflt_ram_con_cal));
			status = WAITING_SPL_UFLT_IR;
		}
		break;

		// Ask for MPL_UFLT_IR RAM register
		case READ_MPL_UFLT_IR: {
			triggerReadRAM(K96_RAM_MPL_UFLT_IR_SIGNAL, sizeof(rxBuffer.payload.xPL_uflt_ram_con_cal));
			status = WAITING_MPL_UFLT_IR;
		}
		break;

		case WAITING_LPL_UFLT_IR:
		case WAITING_SPL_UFLT_IR:
		case WAITING_MPL_UFLT_IR: {
			if (packetReceived) {

				// Evaluate the read values
				unsigned short irSignal = SWAP_BYTES(rxBuffer.payload.xPL_uflt_ram_con_cal.iR_Signal);
				unsigned char channel = K96_CHANNEL_LPL_UFLT_IR + (int)status - (int)WAITING_LPL_UFLT_IR;

				unsigned short error = SWAP_BYTES(rxBuffer.payload.xPL_uflt_ram_con_cal.error);
				unsigned char errorChannel = K96_CHANNEL_LPL_UFLT_ERR + (int)status - (int)WAITING_LPL_UFLT_IR;

				setSample(channel, irSignal);
				setSample(errorChannel, error);

				// Go to next status.
				// For speed optimization, we suppose to have the LPL, SPL and MPL waiting
				// status definitions all together in a sequence
				if (status == WAITING_MPL_UFLT_IR) {
					status = START_SAMPLING;
				} else {
					status = (k96states) ((int)READ_LPL_UFLT_IR + (int)status - (int)WAITING_LPL_UFLT_IR + 1);
				}

				powerUpTimer = K96_BLANK_TIME;
				retry = 0;

			} else if (cmdTimeout) {
				retry++;
				if (retry < K96_MAX_RETRY) {

					// For speed optimization we suppose to have the LPL, SPL and MPL read
					// status definitions all together in a sequence
					status = (k96states)(READ_LPL_UFLT_IR + (int)status - (int)WAITING_LPL_UFLT_IR);
				} else {

					// Mark the unit as unavailable
					status = UNAVAILABLE;
					powerOn(false);
				}
			}
		}
		break;

		case START_READ_ERROR: {
			triggerReadInputRegister(K96_REG_ERRORSTATUS, K96_ERRORSTATUS_REGSIZE);
			status = WAITING_ERROR_REG;
		}
		break;

		case WAITING_ERROR_REG: {
			if (packetReceived) {
				lastErrorStatus = SWAP_BYTES(rxBuffer.payload.reg15.errorStatus);

				// For fatal or configuration errors, avoid to read samples.
				if (lastErrorStatus & (
						K96_FLAG_FATAL_ERROR |
						K96_FLAG_CONFIGURATION_ERROR |
						K96_FLAG_CALIBRATION_ERROR |
						K96_FLAG_SELFDIAG_ERROR |
						K96_FLAG_MEMORY_ERROR)) {

					// Report the error to the host, then skip sampling
					setSample(K96_CHANNEL_ERRORSTATUS,lastErrorStatus);
					status = IDLE_READY;

				} else {

					// No fatal errors found. Read the samples after a small time wait
					powerUpTimer = K96_BLANK_TIME;
					retry = 0;
					status = READ_LPL_UFLT_IR;
				}

			} else if (cmdTimeout) {
				retry++;
				if (retry < K96_MAX_RETRY) {
					status = START_READ_ERROR;
				} else {

					// Mark the unit as unavailable
					status = UNAVAILABLE;
					powerOn(false);
				}
			}
		}
		break;

		// Start reading the remote unit ID
		case READ_UNIT_ID: {
			triggerReadRAM(K96_RAM_METERID, 4);
			status = WAITING_UNIT_ID;
		}
		break;

		// Waiting for the remote unit ID answer
		case WAITING_UNIT_ID: {
			if (packetReceived) {

				// Evaluate the received unit ID
				unsigned int unitID = (rxBuffer.payload.raw[0] |
									  (rxBuffer.payload.raw[1]<<8) |
									  (rxBuffer.payload.raw[2]<<16) |
									  (rxBuffer.payload.raw[3]<<24));
				renderUnitID(unitID);

				// Turn off the unit and wait
				powerOn(false);
				status = IDLE_READY;

			} else if (cmdTimeout) {
				retry++;
				if (retry < K96_MAX_RETRY) {
					status = READ_UNIT_ID;
				} else {

					// Mark the unit as unavailable
					status = UNAVAILABLE;
					powerOn(false);
				}
			}
		}
		break;

		default:
			break;
	}
}


// Called externally at 0.01s period
void K96Device::tick() {

	if (rxStatus != IDLE) {
		rxTimeoutTimer++;
	}

	if ((status == WAITING_ERROR_REG) ||
		(status == WAITING_SAMPLE) ||
		(status == WAITING_UNIT_ID)) {
		cmdTimeoutTimer++;
	}

	if (powerUpTimer != 0) {
		powerUpTimer--;
	}
}

const char* K96Device::getSerial() const {
	return serialID;
}

const char* K96Device::getChannelName(unsigned char channel) const {
	if (channel < K96_NUM_OF_CHANNELS) {
		return channelNames[channel];
	}

	return "";
}

bool K96Device::setChannelName(unsigned char channel, const char* name) {
	return false;
}

const char* K96Device::getMeasurementUnit(unsigned char channel) const {
	if (channel < K96_NUM_OF_CHANNELS) {
		return channelMeasurementUnits[channel];
	}

	return "";
}

float K96Device::evaluateMeasurement(unsigned char channel, float value, bool firstSample) const {

	// At the very startup, when no other samples are stored in the averager,
	// best would be to answer with a 0 value, instead of trying to track which channel
	// is in "S16" value and subtrack the offset (see note below)
	if (firstSample) {
		return 0.0f;
	}

	// Channels defined as "S16" in the K96 datasheet were handled as unsigned 16 bits
	// in the shield, but with an added offset of "0x8000". We need to remove
	// this offset before evaluating the float value
	if ((channel >= K96_CHANNEL_LPL_PC_FLT) && (channel < K96_CHANNEL_PRESS0)) {
		return value - 0x8000;
	} else if (channel == K96_CHANNEL_PRESS0) {
		return (value - 0x8000) / 10;
	} else if ((channel >= K96_CHANNEL_TEMP_NTC0) && (channel < K96_CHANNEL_LPL_UFLT_IR)) {
		return (value - 0x8000) / 100;
	} else if (channel >= K96_CHANNEL_LPL_UFLT_IR) {
		return value;
	}

	return 0.0f;
}

void K96Device::triggerSample() {

	SensorDevice::triggerSample();

	if (status == IDLE_READY) {
		retry = 0;
		status = START_READ_ERROR;
	}
}

// This function is only for setup and debug purposes. It can't be used in the regular
// sampling period because it stops the whole main loop for several milliseconds.
bool K96Device::writeGenericRegister(unsigned int address, unsigned int value, unsigned char* buffer, unsigned char buffSize) {

	// Ask for a RAM write command
	triggerWriteRAM((unsigned short) address, (unsigned char)value);

	// Wait for an answer from the sensor by artificially pumping the serial line
	// until a valid answer or a timeout
	bool stop = false;
	bool packetReceived = false;
	while (!stop) {

		if (SerialC.available()) {
			unsigned char rxChar = SerialC.read();
			packetReceived = onDataReceived(rxChar);
		}

		stop = packetReceived || (lastExceptionCode != K96_NO_ERRORS) || (rxTimeoutTimer > K96_MODBUS_TIMEOUT);
	}

	// Send back the result
	if (packetReceived) {
		buffer[0] = 'O';
		buffer[1] = 'K';
		buffer[2] = '\0';
		return true;
	}

	// Send back the error status
	if (lastExceptionCode) {
		buffer[0] = NIBBLEBINTOHEX((lastErrorStatus>>4)&0x0F);
		buffer[1] = NIBBLEBINTOHEX(lastErrorStatus&0x0F);
		buffer[2] = '\0';
		return true;
	}

	// Send back a timeout error
	buffer[0] = 'T';
	buffer[1] = 'O';
	buffer[2] = '\0';

	return true;
}

// This function is only for setup and debug purposes. It can't be used in the regular
// sampling period because it stops the whole main loop for several milliseconds.
bool K96Device::readGenericRegister(unsigned int address, unsigned char* buffer, unsigned char buffSize) {

	// Ask for a RAM read command
	triggerReadRAM((unsigned short) address, 1);
	// Wait for an answer from the sensor by artificially pumping the serial line
	// until a valid answer or a timeout
	bool stop = false;
	bool packetReceived = false;
	while (!stop) {

		if (SerialC.available()) {
			unsigned char rxChar = SerialC.read();
			packetReceived = onDataReceived(rxChar);
		}

		stop = packetReceived || (lastExceptionCode != K96_NO_ERRORS) || (rxTimeoutTimer > K96_MODBUS_TIMEOUT);
	}

	// Send back the result
	if (packetReceived) {
		buffer[0] = NIBBLEBINTOHEX((rxBuffer.payload.raw[0]>>4)&0x0F);
		buffer[1] = NIBBLEBINTOHEX(rxBuffer.payload.raw[0]&0x0F);
		buffer[2] = '\0';
		return true;
	}

	// Send back the error status
	if (lastExceptionCode) {
		buffer[0] = NIBBLEBINTOHEX((lastErrorStatus>>4)&0x0F);
		buffer[1] = NIBBLEBINTOHEX(lastErrorStatus&0x0F);
		buffer[2] = '\0';
		return true;
	}

	// Send back a timeout error
	buffer[0] = 'T';
	buffer[1] = 'O';
	buffer[2] = '\0';

	return true;
}

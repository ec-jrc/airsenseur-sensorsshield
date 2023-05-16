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


#ifndef K96DEVICE_H_
#define K96DEVICE_H_

#include "SensorDevice.h"

#define K96_CHANNEL_LPL_PC_FLT		0x00
#define K96_CHANNEL_SPL_PC_FLT		0x01
#define K96_CHANNEL_MPL_PC_FLT 		0x02
#define K96_CHANNEL_PRESS0			0x03
#define K96_CHANNEL_TEMP_NTC0		0x04
#define K96_CHANNEL_TEMP_NTC1		0x05
#define K96_CHANNEL_TEMP_UCDIE		0x06
#define K96_CHANNEL_RH0				0x07
#define K96_CHANNEL_T_RH0			0x08
#define K96_CHANNEL_LPL_UFLT_IR		0x09
#define K96_CHANNEL_SPL_UFLT_IR		0x0A
#define K96_CHANNEL_MPL_UFLT_IR		0x0B
#define K96_CHANNEL_ERRORSTATUS		0x0C
#define K96_CHANNEL_LPL_UFLT_ERR	0x0D
#define K96_CHANNEL_SPL_UFLT_ERR	0x0E
#define K96_CHANNEL_MPL_UFLT_ERR	0x0F

#define K96_NUM_OF_CHANNELS	(K96_CHANNEL_MPL_UFLT_ERR+1)

#define K96_RX_BUFFERLENGTH			64
#define K96_SERIALID_BUFFERLENGTH	12

class K96Device : public SensorDevice {
public:
	K96Device();
	virtual ~K96Device();

	virtual void onStartSampling();
	virtual void onStopSampling();
	virtual void setLowPowerMode(bool lowPower);
	virtual bool init();
	virtual void loop();
	virtual void tick();

	virtual const char* getSerial() const;

	virtual const char* getChannelName(unsigned char channel) const;
	virtual bool setChannelName(unsigned char channel, const char* name);
	virtual const char* getMeasurementUnit(unsigned char channel) const;
	virtual float evaluateMeasurement(unsigned char channel, float value, bool firstSample) const;

	virtual bool writeGenericRegister(unsigned int address, unsigned int value, unsigned char* buffer, unsigned char buffSize);
	virtual bool readGenericRegister(unsigned int address, unsigned char* buffer, unsigned char buffSize);

	virtual void triggerSample();

private:
	void powerOn(bool on);
	bool triggerCheckPresence();
	void triggerReadInputRegister(unsigned short address, unsigned char numRegisters);
	void triggerReadRAM(unsigned short address, unsigned char byteSize);
	void triggerWriteRAM(unsigned short address, unsigned char value);

private:
	bool onDataReceived(unsigned char rxChar);
	void renderUnitID(unsigned int unitID);
	bool countCRC(unsigned char *buffer, unsigned short payloadSize, bool checkCRC);
	unsigned short modBusCRC(const unsigned char *buffer, unsigned char bufLength);

private:
	typedef enum _commstates {
		IDLE,
		WAITING_DEVICE_ADDRESS,
		WAITING_FUNCTION_CODE,
		WAITING_EXEPTION_CODE,
		WAITING_DATALENGTH,
		WAITING_DATA,
		WAITING_CRC0,
		WAITING_CRC1,
	} commstates;

	typedef enum _k96states {
		UNAVAILABLE,
		IDLE_READY,

		READ_UNIT_ID,
		WAITING_UNIT_ID,

		START_READ_ERROR,
		WAITING_ERROR_REG,

		READ_LPL_UFLT_IR,		// LPL, SPL and MPL reads needs to be sequentially defined
		READ_SPL_UFLT_IR,
		READ_MPL_UFLT_IR,

		WAITING_LPL_UFLT_IR,	// LPL, SPL and MPL waiting needs to be sequentially defined
		WAITING_SPL_UFLT_IR,
		WAITING_MPL_UFLT_IR,

		START_SAMPLING,
		WAITING_SAMPLE

	} k96states;

	typedef struct _ramresponse {
		unsigned char devAddress;
		unsigned char functionCode;
		unsigned char dataLen;
		union __attribute__((__packed__)) {
			unsigned char raw[K96_RX_BUFFERLENGTH-3];
			struct __attribute__((__packed__)) {
				short lpl_ConcPC_flt;				// IR1
				short spl_ConcPC_flt;				// IR2
				short mpl_ConcPC_flt;				// IR3
				short p_Sensor0_flt;				// IR4
				short ntc0_Temp_flt;				// IR5
				short ntc1_Temp_flt;				// IR6
				short reserved;						// IR7
				short aduCDie_Temp_flt;				// IR8
				short rh_Sensor0;					// IR9
				short rh_T_Sensor0;					// IR10
			} measures;
			struct __attribute__((__packed__)) {
				unsigned short iR_Signal;			// U16	(0x0384)
				short dT;							// S16	(0x0386)
				short unspecified;					// S16  (0x0388)
				short conc;							// S16	(0x038A)
				short concPC;						// S16	(0x038C)
				unsigned short error;				// B16	(0x038E)
			} xPL_uflt_ram_con_cal;
			struct __attribute__ ((__packed__)) {
				unsigned short errorStatus;			// IR15
			} reg15;
			struct __attribute__ ((__packed__)) {
				unsigned short regValue;			// U16
			} u16;
		} payload;
	} ramresponse;

private:

	commstates rxStatus;							// ModBus low level FSM status
	volatile unsigned char rxTimeoutTimer;			// ModBus low level timeout
	volatile unsigned char cmdTimeoutTimer;			// ModBus command timeout
	volatile unsigned char powerUpTimer;			// Avoid communications after power up

	unsigned char expFunctionCode;					// Expected function code in answers
	unsigned char expByteLength;					// Expected payload length in answers
	unsigned char currRxOffset;						// Current pointer in the rx buffer
	unsigned char curRxPayloadBytes;				// Current byte number received
	unsigned char lastExceptionCode;				// Exception code received from last cmd.

	ramresponse rxBuffer;							// Rx buffer

	unsigned char retry;							// Num of ModBus packet retry
	unsigned short lastErrorStatus;

	k96states status;								// ModBus high level FSM status

	char serialID[K96_SERIALID_BUFFERLENGTH];		// Serial ID read from the sensor

	static const char* const channelNames[];
	static const char* const channelMeasurementUnits[];
	static const unsigned short* crcTable;
};

#endif /* K96DEVICE_H_ */

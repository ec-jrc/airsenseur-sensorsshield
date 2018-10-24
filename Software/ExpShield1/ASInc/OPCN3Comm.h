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

#ifndef OPCN3COMM_H_
#define OPCN3COMM_H_

#define OPCN3_RXBUFFER	0x80

#define OPCN3_BINS_NUMBER		24
#define OPCN3_BINM2F_NUMBER		4
#define OPCN3_PMS_NUMBER			3

class OPCN3Comm {

public:
	typedef enum _cmdOffset {
		SET_FAN_DIGPOT_OFF,
		SET_FAN_DIGPOT_ON,
		SET_LASER_DIGPOT_OFF,
		SET_LASER_DIGPOT_ON,
		SET_LASER_SWITCH_OFF,
		SET_LASER_SWITCH_ON,
		READ_HISTOGRAM,
		NONE,
	} cmdOffset;

	typedef union _pmval {
		float fVal;
		unsigned char ucVals[4];
		long lVal;
	} pmval;

	typedef struct type32 { unsigned char x[4]; } type32;

	typedef struct _histogram {
		unsigned short bins[OPCN3_BINS_NUMBER];
		unsigned char binMtoF[OPCN3_BINM2F_NUMBER];
		unsigned short samplingPeriod;
		unsigned short samplingFlowRate;
		unsigned short temperature;
		unsigned short relHumidity;
		type32 pmVal[OPCN3_PMS_NUMBER];
		unsigned short rejGlitch;
		unsigned short rejLongTOF;
		unsigned short ratio;
		unsigned short outOfRange;
		unsigned short fanRevCount;
		unsigned short laserStatus;
		unsigned short checksum;
	} histogram;


public:
	OPCN3Comm();
	virtual ~OPCN3Comm();

	virtual void loop();
	virtual void tick();

	virtual bool triggerCommand(cmdOffset cmdOffset);
	virtual bool ready();

	virtual histogram* getLastHistogram();

	float toPmValue(type32 *data);

private:

	typedef enum _smstatus {
		IDLE,
		WAITING_READY,
		EVALUATE,
	} smstatus;

	typedef struct _smdata {
		smstatus status;
		cmdOffset currCmdOffset;
		unsigned char timer;
		unsigned char retry;
		unsigned char currRxDataOffset;
		unsigned char currTxDataOffset;
		unsigned char buffer[OPCN3_RXBUFFER];
	} smdata;

	typedef enum _opcn3command {
		W_PERIPH_POWER_STATUS = 0x03,
		W_RESET = 0x06,
		R_SERIAL_NUMBER = 0x10,
		R_FW_VERSION = 0x12,
		R_HIST_DATA = 0x30,
		R_PM_DATA = 0x32,
		R_INFO_STRING = 0x3F
	} opcn3command;

	typedef struct _cmdinfo {
		opcn3command cmdStatement;
		unsigned char rxLength;
		unsigned char txLength;
		smstatus fallbackStatus;
		unsigned char data;
	} cmdinfo;

private:
	void resetStateMachine();
	unsigned char write(unsigned char data);
	unsigned short CalcCRC(unsigned char data[], unsigned char nbrOfBytes);

private:
	static const cmdinfo cmdInfoList[];
	float fHelper;
	smdata smData;
};

#endif /* OPCN3DEVICE_H_ */

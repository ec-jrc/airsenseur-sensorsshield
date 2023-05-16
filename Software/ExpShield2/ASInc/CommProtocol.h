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

#ifndef COMMPROTOCOL_H
#define	COMMPROTOCOL_H

#define FIRMWARE_VERSON                 "FW1.1.0 P3.1"

#define COMMPROTOCOL_HEADER             '{'
#define COMMPROTOCOL_TRAILER            '}'
#define COMMPROTOCOL_ERROR              "{*}"

#define COMMPROTOCOL_SENSOR_INQUIRY     'I'
#define COMMPROTOCOL_ECHO               'E'
#define COMMPROTOCOL_SAMPLE_ENABLE      'S'
#define COMMPROTOCOL_SAMPLE_DISABLE     'X'
#define COMMPROTOCOL_SET_SAMPLEPRESC    'P'
#define COMMPROTOCOL_GET_SAMPLEPRESC    'Q'
#define COMMPROTOCOL_SET_SAMPLEPOSTS    'O'
#define COMMPROTOCOL_GET_SAMPLEPOSTS    'N'
#define COMMPROTOCOL_SET_SAMPLEDECIM    'D'
#define COMMPROTOCOL_GET_SAMPLEDECIM    'F'
#define COMMPROTOCOL_LASTSAMPLE         'G'
#define COMMPROTOCOL_LASTSAMPLEHRES     'Y'
#define COMMPROTOCOL_FREEMEMORY         'M'
#define COMMPROTOCOL_LOADPRESET         'L'
#define COMMPROTOCOL_SAVEPRESET         'W'
#define COMMPROTOCOL_WRITE_STP_REG      'R'
#define COMMPROTOCOL_READ_STP_REG       'T'
#define COMMPROTOCOL_WRITE_SSERIAL      'J'
#define COMMPROTOCOL_READ_SSERIAL       'K'
#define COMMPROTOCOL_WRITE_BOARDSERIAL  'U'
#define COMMPROTOCOL_READ_BOARDSERIAL   'V'
#define COMMPROTOCOL_READ_FWVERSION     'Z'
#define COMMPROTOCOL_READ_SAMPLEPERIOD	'a'
#define COMMPROTOCOL_READ_UNITS			'b'
#define COMMPROTOCOL_READ_BOARDTYPE		'c'
#define COMMPROTOCOL_WRITE_CHANENABLE	'd'
#define COMMPROTOCOL_READ_CHANENABLE	'e'
#define COMMPROTOCOL_WRITE_REGISTER		'f'
#define COMMPROTOCOL_READ_REGISTER		'g'

#define MAX_SERIAL_BUFLENGTH			 64							// Stack temporary buffer size
#define MAX_INQUIRY_BUFLENGTH            MAX_SERIAL_BUFLENGTH		// Maximum preset/channel name
#define COMMPROTOCOL_BUFFER_LENGTH       (2*MAX_SERIAL_BUFLENGTH)	// Buffer used to store a single incoming data packet

class SensorsArray;
class SensorBusWrapper;

class CommProtocol {

public:
	typedef enum _source {
		SOURCE_SERIAL,
		SOURCE_SENSORBUS,
		SOURCE_USB,
		SOURCE_NONE
	} source;

public:
    CommProtocol(SensorsArray *sensors);
    virtual ~CommProtocol();
    
    void timerTick();
    void onDataReceived(unsigned char pivotChar, source sourceId);
    void setSensorBusWrapper(SensorBusWrapper* sensorBusWrapper);
    
private:
    void reset();
    void processBuffer();
    bool renderOKAnswer(unsigned char cmdOffset, unsigned char param);
    unsigned char getParameter(unsigned char parNum);
    unsigned short getShortParameter(unsigned char parNum);
    unsigned int getInt32Parameter(unsigned char parNum);
    void writeValue(unsigned char value, bool last);
    void writeValue(unsigned short value, bool last);
    void writeValue(unsigned int value, bool last);
    void writeValue(unsigned long value, bool last);
    void writeValue(float value, bool last);
    void writeString(unsigned char* value, bool last);
    
private:
    static bool echo(CommProtocol* context, unsigned char cmdOffset);
    static bool sampleEnable(CommProtocol* context, unsigned char cmdOffset);
    static bool sampleDisable(CommProtocol* context, unsigned char cmdOffset);
    static bool setSamplePrescaler(CommProtocol* context, unsigned char cmdOffset);
    static bool getSamplePrescaler(CommProtocol* context, unsigned char cmdOffset);
    static bool setSamplePostscaler(CommProtocol* context, unsigned char cmdOffset);
    static bool getSamplePostscaler(CommProtocol* context, unsigned char cmdOffset);
    static bool setSampleDecimation(CommProtocol* context, unsigned char cmdOffset);
    static bool getSampleDecimation(CommProtocol* context, unsigned char cmdOffset);
    static bool getFreeMemory(CommProtocol* context, unsigned char cmdOffset);
    static bool lastSample(CommProtocol* context, unsigned char cmdOffset);
    static bool lastSampleHRes(CommProtocol* context, unsigned char cmdOffset);
    static bool sensorInquiry(CommProtocol* context, unsigned char cmdOffset);
    static bool loadPreset(CommProtocol* context, unsigned char cmdOffset);
    static bool savePreset(CommProtocol* context, unsigned char cmdOffset);
    static bool setSetpointRegister(CommProtocol* context, unsigned char cmdOffset);
    static bool getSetpointRegister(CommProtocol* context, unsigned char cmdOffset);
    static bool writeSensorSerialNumber(CommProtocol* context, unsigned char cmdOffset);
    static bool readSensorSerialNumber(CommProtocol* context, unsigned char cmdOffset);
    static bool writeBoardSerialNumber(CommProtocol* context, unsigned char cmdOffset);
    static bool readBoardSerialNumber(CommProtocol* context, unsigned char cmdOffset);
    static bool readFirmwareVersion(CommProtocol* context, unsigned char cmdOffset);
    static bool readSamplePeriod(CommProtocol* context, unsigned char cmdOffset);
    static bool readUnits(CommProtocol* context, unsigned char cmdOffset);
    static bool readBoardType(CommProtocol* context, unsigned char cmdOffset);
    static bool writeChannelEnable(CommProtocol* context, unsigned char cmdOffset);
    static bool readChannelEnable(CommProtocol* context, unsigned char cmdOffset);
    static bool writeRegister(CommProtocol* context, unsigned char cmdOffset);
    static bool readRegister(CommProtocol* context, unsigned char cmdOffset);
    
private:
    
    // Structure used to simplify the protocol parsing operations
    typedef struct _commandinfo {
        unsigned char commandID;                                                // A specific command ID
        unsigned char parNum;                                                   // How many parameters are expected
        bool (*handler)(CommProtocol* context, unsigned char cmdOffset);        // The associated handler
    } commandinfo;
    
    typedef enum _rxstatus {
        RX_IDLE,
        RX_HEADER_FOUND
    } rxstatus;
    
private:

    static const commandinfo validCommands[];
    static const char commProtocolErrorString[];
    
    unsigned char buffer[COMMPROTOCOL_BUFFER_LENGTH];
    unsigned char offset;
    rxstatus rxStatus;
    source lastSourceId;
    
    volatile unsigned short timer;
    
    SensorsArray* sensorsArray;                 // A reference to the array sensor
    SensorBusWrapper* sensorBus;	               // A reference to sesor bus wrapper
};

#endif	/* COMMPROTOCOL_H */


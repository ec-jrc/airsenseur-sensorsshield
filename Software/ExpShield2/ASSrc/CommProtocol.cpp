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


#include <CommProtocol.h>
#include <LEDsHelper.h>
#include <string.h>
#include "SensorsArray.h"

#include <SensorBusWrapper.h>
#include <SerialAHelper.h>
#include <SerialBHelper.h>
#include <SerialUSBHelper.h>

#define COMMPROTOCOL_TIMEOUT  500   /* in 10ms steps -> 5seconds */

const CommProtocol::commandinfo CommProtocol::validCommands[] = {

	{ COMMPROTOCOL_LASTSAMPLE, 1, &CommProtocol::lastSample },
	{ COMMPROTOCOL_LASTSAMPLEHRES, 1, &CommProtocol::lastSampleHRes },
    { COMMPROTOCOL_SENSOR_INQUIRY, 1, &CommProtocol::sensorInquiry },
    { COMMPROTOCOL_ECHO, 0, &CommProtocol::echo },
    { COMMPROTOCOL_SAMPLE_ENABLE, 0, &CommProtocol::sampleEnable },
    { COMMPROTOCOL_SAMPLE_DISABLE , 0, &CommProtocol::sampleDisable },
    { COMMPROTOCOL_SET_SAMPLEPRESC, 2, &CommProtocol::setSamplePrescaler },
    { COMMPROTOCOL_GET_SAMPLEPRESC, 1, &CommProtocol::getSamplePrescaler },
    { COMMPROTOCOL_SET_SAMPLEPOSTS, 2, &CommProtocol::setSamplePostscaler },
    { COMMPROTOCOL_GET_SAMPLEPOSTS, 1, &CommProtocol::getSamplePostscaler },
    { COMMPROTOCOL_SET_SAMPLEDECIM, 2, &CommProtocol::setSampleDecimation },
    { COMMPROTOCOL_GET_SAMPLEDECIM, 1, &CommProtocol::getSampleDecimation },
	{ COMMPROTOCOL_WRITE_STP_REG, 2, &CommProtocol::setSetpointRegister },
	{ COMMPROTOCOL_READ_STP_REG, 1, &CommProtocol::getSetpointRegister },
    { COMMPROTOCOL_FREEMEMORY, 0, &CommProtocol::getFreeMemory },
    { COMMPROTOCOL_LOADPRESET, 1, &CommProtocol::loadPreset },
    { COMMPROTOCOL_SAVEPRESET, MAX_INQUIRY_BUFLENGTH+1, &CommProtocol::savePreset },
    { COMMPROTOCOL_WRITE_SSERIAL, 2, &CommProtocol::writeSensorSerialNumber },
    { COMMPROTOCOL_READ_SSERIAL, 1, &CommProtocol::readSensorSerialNumber },
	{ COMMPROTOCOL_WRITE_BOARDSERIAL, 2, &CommProtocol::writeBoardSerialNumber },
	{ COMMPROTOCOL_READ_BOARDSERIAL, 1, &CommProtocol::readBoardSerialNumber },
	{ COMMPROTOCOL_READ_FWVERSION, 1, &CommProtocol::readFirmwareVersion },
	{ COMMPROTOCOL_READ_SAMPLEPERIOD, 1, &CommProtocol::readSamplePeriod },
	{ COMMPROTOCOL_READ_UNITS, 1, &CommProtocol::readUnits },
	{ COMMPROTOCOL_READ_BOARDTYPE, 1, &CommProtocol::readBoardType },
	{ COMMPROTOCOL_WRITE_CHANENABLE, 1, &CommProtocol::writeChannelEnable },
	{ COMMPROTOCOL_READ_CHANENABLE, 1, &CommProtocol::readChannelEnable },
	{ COMMPROTOCOL_WRITE_REGISTER, 3, &CommProtocol::writeRegister },
	{ COMMPROTOCOL_READ_REGISTER, 2, &CommProtocol::readRegister }
};

const char CommProtocol::commProtocolErrorString[] = { COMMPROTOCOL_ERROR };

CommProtocol::CommProtocol(SensorsArray* sensors) : sensorsArray(sensors) {
    reset();
}

CommProtocol::~CommProtocol() {
}

void CommProtocol::reset() {
    memset(buffer, 0, sizeof(buffer));
    rxStatus = RX_IDLE;
    offset = 0;
    timer = 0;
    lastSourceId = SOURCE_SERIAL;
}

void CommProtocol::timerTick() {

    if (rxStatus != RX_IDLE) {

        timer++;
        if (timer >= COMMPROTOCOL_TIMEOUT) {
            reset();
        }
    }
}

void CommProtocol::onDataReceived(unsigned char pivotChar, source sourceId) {
    
    switch (rxStatus) {
        case RX_IDLE: {
            // Searching for an header
            if (pivotChar == COMMPROTOCOL_HEADER) {
                reset();
                rxStatus = RX_HEADER_FOUND;
                lastSourceId = sourceId;
            }
        }
            break;

        case RX_HEADER_FOUND: {
        		if (sourceId == lastSourceId) {
				   // Searching for a trailer
				   if(pivotChar == COMMPROTOCOL_TRAILER) {
					   processBuffer();

				   } else if(pivotChar == COMMPROTOCOL_HEADER) {
					   // ... but we found an header again...
					   // Discard all.
					   memset(buffer, 0, sizeof(buffer));
					   offset = 0;
					   timer = 0;

				   } else if(offset == (sizeof(buffer)-1)) {
					   // We did not found any trailer and
					   // the buffer is empty. Discard all
					   rxStatus = RX_IDLE;

				   } else {

						// Collecting payload
						buffer[offset] = pivotChar;
						offset++;
				   }
        		}
        }
            break;
    }
}

void CommProtocol::setSensorBusWrapper(SensorBusWrapper* sensorBusWrapper) {
	sensorBus = sensorBusWrapper;
}

void CommProtocol::processBuffer() {
    
    // In the 1st position we expect the command ID, validate it
    bool valid = false;
    unsigned char offsetId = 0;
    for (unsigned char n = 0; (n < sizeof(validCommands)/sizeof(commandinfo)) && (!valid); n++) {
        if ((*buffer) == validCommands[n].commandID) {
            valid = true;
            offsetId = n;
        }
    }
    
    // Execute the action
    typedef bool (*fpointer)(CommProtocol* context, unsigned char cmdOffset);
    fpointer handler = validCommands[offsetId].handler;
    if (valid && handler != 0) {
        valid = (*handler)(this, offsetId);
    }
    
    // Signal an invalid/fault condition
    if (!valid) {
        strcpy((char*)buffer, commProtocolErrorString);
    }
    
    // Send back the result
    switch (lastSourceId) {
    		case SOURCE_SERIAL: {
    			SerialA.write((char*)buffer);
    		}
    			break;

    		case SOURCE_SENSORBUS: {
    			if (sensorBus) {
    				sensorBus->write((char*)buffer);
    			}
    		}
    			break;

    		case SOURCE_USB: {
    			SerialUSB.write((char*)buffer);
    		}
    			break;

    		case SOURCE_NONE:
    			break;
    }
    
    // Signal TX transmission
    LEDs.pulse(LEDsHelper::TXDATA);

    // Reset the buffer and the rx status machine
    reset();
}

// Retrieve the parameter value from the incoming buffer
#define HEXTONIBBLE(a) (((a) <= '9')?((a)-'0'):(((a)-'A') + 0x0A))
unsigned char CommProtocol::getParameter(unsigned char parNum) {

    unsigned char* data = buffer + 1 + (parNum<<1);
    unsigned char result = HEXTONIBBLE(*data) << 4;
    data++;
    result |= HEXTONIBBLE(*data);
        
    return result;
}

unsigned short CommProtocol::getShortParameter(unsigned char parNum) {

	unsigned char MSB = getParameter(parNum);
	unsigned char LSB = getParameter(parNum+1);

	return ((((unsigned short)MSB) << 8) & 0xFF00) | LSB;
}

unsigned int CommProtocol::getInt32Parameter(unsigned char parNum) {

	unsigned char MMSB = getParameter(parNum);
	unsigned char MLSB = getParameter(parNum+1);
	unsigned char LMSB = getParameter(parNum+2);
	unsigned char LLSB = getParameter(parNum+3);

	return ((((unsigned int)MMSB) << 24) & 0xFF000000) |
		   ((((unsigned int)MLSB) << 16) & 0x00FF0000) |
		   ((((unsigned short)LMSB) << 8) & 0xFF00) | (LLSB);

}

#define NIBBLEBINTOHEX(a) ((a)>0x09)?(((a)-0x0A)+'A'):((a)+'0');

void CommProtocol::writeValue(unsigned char value, bool last) {
    
    unsigned char valBuf[4];
    
    valBuf[0] = NIBBLEBINTOHEX(((value>>4) & 0x0F));
    valBuf[1] = NIBBLEBINTOHEX((value & 0x0F));
    valBuf[2] = (last)? '}':0;    
    valBuf[3] = 0;
    
    strcat((char*)buffer, (char*)valBuf);
}

void CommProtocol::writeValue(unsigned short value, bool last) {
    
    writeValue((unsigned char)((value>>8)&0xFF), false);
    writeValue((unsigned char)(value&0xFF), last);
}

void CommProtocol::writeValue(unsigned int value, bool last) {
	writeValue((unsigned short)((value>>16) & 0xFFFF), false);
	writeValue((unsigned short)(value & 0xFFFF), last);
}

void CommProtocol::writeValue(unsigned long value, bool last) {
    
    writeValue((unsigned char)((value>>24)&0xFF), false);
    writeValue((unsigned char)((value>>16)&0xFF), false);
    writeValue((unsigned char)((value>>8)&0xFF), false);
    writeValue((unsigned char)(value&0xFF), last);
}

void CommProtocol::writeValue(float value, bool last) {

    unsigned long iValue = *((unsigned long*)&value);
    writeValue(iValue, last);
}

void CommProtocol::writeString(unsigned char* value, bool last) {

    unsigned char length = strlen((char*)value);
    for (unsigned char n = 0; n < length; n++) {
        writeValue(*(value+n), (n == (length-1)) && last);
    }
}

bool CommProtocol::renderOKAnswer(unsigned char cmdOffset, unsigned char param) {
    
    buffer[0] = COMMPROTOCOL_HEADER;
    buffer[1] = validCommands[cmdOffset].commandID;
    buffer[2] = 0;
    writeValue(param, true);
    
    return true;
}

// Function handler: echo back the received buffer
bool CommProtocol::echo(CommProtocol* context, unsigned char cmdOffset) {
    
    context->buffer[0] = COMMPROTOCOL_HEADER;
    context->buffer[1] = validCommands[cmdOffset].commandID;
    context->buffer[2] = COMMPROTOCOL_TRAILER;
    context->buffer[3] = 0;
    return true;
}

// Function handler: start sampling
bool CommProtocol::sampleEnable(CommProtocol* context, unsigned char cmdOffset) {
    if (context->sensorsArray->enableSampling(true)) {
        return context->echo(context, cmdOffset);
    }
    return false;
}

// Function handler: stop sampling
bool CommProtocol::sampleDisable(CommProtocol* context, unsigned char cmdOffset) {
    if (context->sensorsArray->enableSampling(false)) {
        return context->echo(context, cmdOffset);
    }
    return false;
}

// Function handler: set sampler prescaler
bool CommProtocol::setSamplePrescaler(CommProtocol* context, unsigned char cmdOffset) {
    
    unsigned char channel = context->getParameter(0);
    unsigned char prescaler = context->getParameter(1);
    if (context->sensorsArray->setSamplePrescaler(channel, prescaler)) {
        return context->renderOKAnswer(cmdOffset, channel);
    }
    return false;
}

// Function handler: set sampler prescaler
bool CommProtocol::getSamplePrescaler(CommProtocol* context, unsigned char cmdOffset) {

    unsigned char channel = context->getParameter(0);
    unsigned char prescaler;
    if (!context->sensorsArray->getSamplePrescaler(channel, &prescaler)) {
        return false;
    }

    context->buffer[0] = COMMPROTOCOL_HEADER;
    context->buffer[1] = validCommands[cmdOffset].commandID;
    context->buffer[2] = 0;
    context->writeValue(channel, false);
    context->writeValue(prescaler, true);

    return true;
}

// Function handler: set sampler postscaler
bool CommProtocol::setSamplePostscaler(CommProtocol* context, unsigned char cmdOffset) {

    unsigned char channel = context->getParameter(0);
    unsigned char postscaler = context->getParameter(1);
    if (context->sensorsArray->setSamplePostscaler(channel, postscaler)) {
        return context->renderOKAnswer(cmdOffset, channel);
    }
    return false;
}

// Function handler: get sampler postscaler
bool CommProtocol::getSamplePostscaler(CommProtocol* context, unsigned char cmdOffset) {

    unsigned char channel = context->getParameter(0);
    unsigned char postscaler;
    if (!context->sensorsArray->getSamplePostscaler(channel, &postscaler)) {
        return false;
    }

    context->buffer[0] = COMMPROTOCOL_HEADER;
    context->buffer[1] = validCommands[cmdOffset].commandID;
    context->buffer[2] = 0;
    context->writeValue(channel, false);
    context->writeValue(postscaler, true);

    return true;
}

// Function handler: set sampler decimation filter
bool CommProtocol::setSampleDecimation(CommProtocol* context, unsigned char cmdOffset) {
    
    unsigned char channel = context->getParameter(0);
    unsigned char decimation = context->getParameter(1);    
    if (context->sensorsArray->setSampleDecimation(channel, decimation)) {
        return context->renderOKAnswer(cmdOffset, channel);
    }
    return false;
}

// Function handler: get sampler decimation filter
bool CommProtocol::getSampleDecimation(CommProtocol* context, unsigned char cmdOffset) {

    unsigned char channel = context->getParameter(0);
    unsigned char decimation;
    if (!context->sensorsArray->getSampleDecimation(channel, &decimation)) {
        return false;
    }

    context->buffer[0] = COMMPROTOCOL_HEADER;
    context->buffer[1] = validCommands[cmdOffset].commandID;
    context->buffer[2] = 0;
    context->writeValue(channel, false);
    context->writeValue(decimation, true);

    return true;
}


// Function handler: retrieve the free RAM memory (this command is not supported by current firmware release)
bool CommProtocol::getFreeMemory(CommProtocol* context, unsigned char cmdOffset) {

    context->buffer[0] = COMMPROTOCOL_HEADER;
    context->buffer[1] = validCommands[cmdOffset].commandID;
    context->buffer[2] = 0;
    context->writeValue((unsigned short) 0x0100, true);

    return true;
}

// Function handler: take the last sample
bool CommProtocol::lastSample(CommProtocol* context, unsigned char cmdOffset) {
    
    unsigned short lastSample = 0;
    unsigned long lastTimestamp = 0;
    unsigned char channel = context->getParameter(0);
    if (!context->sensorsArray->getLastSample(channel, lastSample, lastTimestamp)) {
        return false;
    }
    
    context->buffer[0] = COMMPROTOCOL_HEADER;
    context->buffer[1] = validCommands[cmdOffset].commandID;
    context->buffer[2] = 0;
    context->writeValue(channel, false);
    context->writeValue(lastSample, false);
    context->writeValue(lastTimestamp, true);
    
    return true;
}

// Function handler: take the last sample in high resolution mode (float)
bool CommProtocol::lastSampleHRes(CommProtocol* context, unsigned char cmdOffset) {

	float lastSample = 0.0f;
	unsigned long lastTimestamp = 0;
	unsigned char channel = context->getParameter(0);
    if (!context->sensorsArray->getLastSample(channel, lastSample, lastTimestamp)) {
        return false;
    }

    context->buffer[0] = COMMPROTOCOL_HEADER;
    context->buffer[1] = validCommands[cmdOffset].commandID;
    context->buffer[2] = 0;
    context->writeValue(channel, false);
    context->writeValue(lastSample, false);
    context->writeValue(lastTimestamp, true);

    return true;
}

// Function handler: get the sensor name
bool CommProtocol::sensorInquiry(CommProtocol* context, unsigned char cmdOffset) {

    unsigned char result[MAX_INQUIRY_BUFLENGTH];
    unsigned char channel = context->getParameter(0);
    
    context->sensorsArray->inquirySensor(channel, result, MAX_INQUIRY_BUFLENGTH);
    
    if (strlen((char*)result) == 0) {
        return false;
    }
    
    context->buffer[0] = COMMPROTOCOL_HEADER;
    context->buffer[1] = validCommands[cmdOffset].commandID;
    context->buffer[2] = 0;
    context->writeValue(channel, false);
    context->writeString(result, true);
    
    return true;
}

// Function handler: load a sensor preset from the EEPROM
bool CommProtocol::loadPreset(CommProtocol* context, unsigned char cmdOffset) {
    
    unsigned char channel = context->getParameter(0);
    
    if (context->sensorsArray->loadPreset(channel)) {
        return context->renderOKAnswer(cmdOffset, channel);
    }
    
    return false;
}

// Function handler: save a sensor preset to the EEPROM
bool CommProtocol::savePreset(CommProtocol* context, unsigned char cmdOffset) {
    
    unsigned char result[MAX_INQUIRY_BUFLENGTH];
    unsigned char channel = context->getParameter(0);

    for (unsigned char par = 0; par < MAX_INQUIRY_BUFLENGTH; par++) {
        result[par] = context->getParameter(par+1);
    }
    result[MAX_INQUIRY_BUFLENGTH-1] = 0;
        
    if (context->sensorsArray->savePreset(channel, result, MAX_INQUIRY_BUFLENGTH)) {
        return context->renderOKAnswer(cmdOffset, channel);
    }
    
    return false;
}

// Function handler: write the sensor serial number
bool CommProtocol::setSetpointRegister(CommProtocol* context, unsigned char cmdOffset) {

	unsigned char channel = context->getParameter(0);
	unsigned short setpointVal = context->getShortParameter(1);

	if (context->sensorsArray->setSetpoint(channel, setpointVal)) {
		return context->renderOKAnswer(cmdOffset, channel);
	}

	return false;
}

// Function handler: write the sensor serial number
bool CommProtocol::getSetpointRegister(CommProtocol* context, unsigned char cmdOffset) {

	unsigned char channel = context->getParameter(0);
	unsigned short setpointVal;

	if(!context->sensorsArray->getSetpoint(channel, setpointVal)) {
		return false;
	}

    context->buffer[0] = COMMPROTOCOL_HEADER;
    context->buffer[1] = validCommands[cmdOffset].commandID;
    context->buffer[2] = 0;
    context->writeValue(channel, false);
    context->writeValue(setpointVal, true);

    return true;
}

// Function handler: write the sensor serial number
bool CommProtocol::writeSensorSerialNumber(CommProtocol* context, unsigned char cmdOffset) {

    unsigned char result[MAX_SERIAL_BUFLENGTH];
    unsigned char channel = context->getParameter(0);

    for (unsigned char par = 0; par < MAX_SERIAL_BUFLENGTH; par++) {
        result[par] = context->getParameter(par+1);
    }
    result[MAX_SERIAL_BUFLENGTH-1] = 0;

    if (context->sensorsArray->saveSensorSerialNumber(channel, result, MAX_SERIAL_BUFLENGTH)) {
        return context->renderOKAnswer(cmdOffset, channel);
    }

    return false;
}

// Function handler: read the sensor serial number
bool CommProtocol::readSensorSerialNumber(CommProtocol* context, unsigned char cmdOffset) {

	unsigned char result[MAX_SERIAL_BUFLENGTH];
    unsigned char channel = context->getParameter(0);

    bool ok = context->sensorsArray->readSensorSerialNumber(channel, result, MAX_SERIAL_BUFLENGTH);
    if (!ok) {
    	return false;
    }

    // Override to NA if not set
    if (strlen((char*)result) == 0) {
        strcpy((char*)result, "NA");
    }

    context->buffer[0] = COMMPROTOCOL_HEADER;
    context->buffer[1] = validCommands[cmdOffset].commandID;
    context->buffer[2] = 0;
    context->writeValue(channel, false);
    context->writeString(result, true);

    return true;
}

// Function handler: write the board serial number
bool CommProtocol::writeBoardSerialNumber(CommProtocol* context, unsigned char cmdOffset) {

    unsigned char result[MAX_SERIAL_BUFLENGTH];

    for (unsigned char par = 0; par < MAX_SERIAL_BUFLENGTH; par++) {
        result[par] = context->getParameter(par);
    }
    result[MAX_SERIAL_BUFLENGTH-1] = 0;

    if (context->sensorsArray->saveBoardSerialNumber(result, MAX_SERIAL_BUFLENGTH)) {
        return context->echo(context, cmdOffset);
    }

    return false;
}

// Function handler: read the board serial number
bool CommProtocol::readBoardSerialNumber(CommProtocol* context, unsigned char cmdOffset) {

	unsigned char result[MAX_SERIAL_BUFLENGTH];

    context->sensorsArray->readBoardSerialNumber(result, MAX_SERIAL_BUFLENGTH);

    if (strlen((char*)result) == 0) {
        return false;
    }

    context->buffer[0] = COMMPROTOCOL_HEADER;
    context->buffer[1] = validCommands[cmdOffset].commandID;
    context->buffer[2] = 0;
    context->writeString(result, true);

    return true;
}

// Function handler: read firmware version
bool CommProtocol::readFirmwareVersion(CommProtocol* context, unsigned char cmdOffset) {

    context->buffer[0] = COMMPROTOCOL_HEADER;
    context->buffer[1] = validCommands[cmdOffset].commandID;
    context->buffer[2] = 0;
    context->writeString((unsigned char*)FIRMWARE_VERSON, true);

	return true;
}

// Function handler: read sample period for a specific channel
bool CommProtocol::readSamplePeriod(CommProtocol* context, unsigned char cmdOffset) {

	unsigned long samplePeriod;
	unsigned char channel = context->getParameter(0);

    if (!context->sensorsArray->readChannelSamplePeriod(channel, &samplePeriod)) {
        return false;
    }

    context->buffer[0] = COMMPROTOCOL_HEADER;
    context->buffer[1] = validCommands[cmdOffset].commandID;
    context->buffer[2] = 0;
    context->writeValue(channel, false);
    context->writeValue(samplePeriod, true);

    return true;

}

// Function handler: read units for a specific channel
bool CommProtocol::readUnits(CommProtocol* context, unsigned char cmdOffset) {

    unsigned char result[MAX_SERIAL_BUFLENGTH];
    unsigned char channel = context->getParameter(0);

    context->sensorsArray->getUnitForChannel(channel, result, MAX_SERIAL_BUFLENGTH);

    if (strlen((char*)result) == 0) {
        return false;
    }

    context->buffer[0] = COMMPROTOCOL_HEADER;
    context->buffer[1] = validCommands[cmdOffset].commandID;
    context->buffer[2] = 0;
    context->writeValue(channel, false);
    context->writeString(result, true);

    return true;
}

// Function handler: read board type and number of supported channel
bool CommProtocol::readBoardType(CommProtocol* context, unsigned char cmdOffset) {

	unsigned short boardType = context->sensorsArray->getBoardType();
	unsigned short channelNumber = context->sensorsArray->getBoardNumChannels();

	context->buffer[0] = COMMPROTOCOL_HEADER;
	context->buffer[1] = validCommands[cmdOffset].commandID;
	context->buffer[2] = 0;
	context->writeValue(boardType, false);
	context->writeValue(channelNumber, true);

	return true;
}

// Function handler: enable/disable a specified channel
bool CommProtocol::writeChannelEnable(CommProtocol* context, unsigned char cmdOffset) {

    unsigned char channel = context->getParameter(0);
    unsigned char enabled = context->getParameter(1);

    if (context->sensorsArray->setEnableChannel(channel, enabled)) {
        return context->renderOKAnswer(cmdOffset, channel);
    }
    return false;
}

// Function handler: inquiry for enabled/disabled status for a specified channel
bool CommProtocol::readChannelEnable(CommProtocol* context, unsigned char cmdOffset) {

    unsigned char channel = context->getParameter(0);
    unsigned char enabled;
    if (!context->sensorsArray->getChannelIsEnabled(channel, &enabled)) {
        return false;
    }

    context->buffer[0] = COMMPROTOCOL_HEADER;
    context->buffer[1] = validCommands[cmdOffset].commandID;
    context->buffer[2] = 0;
    context->writeValue(channel, false);
    context->writeValue(enabled, true);

    return true;
}

// Function handler: write an int (32bit) value into a specified register (32bit address)
// for a specified channel
bool CommProtocol::writeRegister(CommProtocol* context, unsigned char cmdOffset) {

    unsigned char channel = context->getParameter(0);

    unsigned int address = context->getInt32Parameter(1);
    unsigned int value = context->getInt32Parameter(5);

    if (!context->sensorsArray->writeGenericRegisterChannel(channel, address, value)) {
    	return false;
    }

    context->buffer[0] = COMMPROTOCOL_HEADER;
    context->buffer[1] = validCommands[cmdOffset].commandID;
    context->buffer[2] = 0;
    context->writeValue(channel, false);
    context->writeValue(address, false);
    context->writeValue(value, true);

    return true;
}

// Function handler: read an int (32bit) value from a specified register (32bit address)
// for a specified channel
bool CommProtocol::readRegister(CommProtocol* context, unsigned char cmdOffset) {

    unsigned char channel = context->getParameter(0);

    unsigned int address = context->getInt32Parameter(1);

    unsigned int result = 0;
    if (!context->sensorsArray->readGenericRegisterChannel(channel, address, result)) {
    	return false;
    }

    context->buffer[0] = COMMPROTOCOL_HEADER;
    context->buffer[1] = validCommands[cmdOffset].commandID;
    context->buffer[2] = 0;
    context->writeValue(channel, false);
    context->writeValue(address, false);
    context->writeValue(result, true);

    return true;
}

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
#include "SensorsArray.h"
#include "CommProtocol.h"
#include "SerialAHelper.h"
#include "SerialBHelper.h"
#include "SerialUSBHelper.h"
#include "SensorBusWrapper.h"
#include "LEDsHelper.h"

#define COMMPROTOCOL_TIMEOUT  500   /* in 10ms steps -> 5seconds */

const CommProtocol::commandinfo CommProtocol::validCommands[] = {

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
    { COMMPROTOCOL_SET_IIRDENOMVALUES, 2, &CommProtocol::setSampleIIRDenominators },
    { COMMPROTOCOL_GET_IIRDENOMVALUES, 1, &CommProtocol::getSampleIIRDenominators },
    { COMMPROTOCOL_LASTSAMPLE, 1, &CommProtocol::lastSample },
    { COMMPROTOCOL_FREEMEMORY, 0, &CommProtocol::getFreeMemory },
    { COMMPROTOCOL_LOADPRESET, 1, &CommProtocol::loadPreset },
    { COMMPROTOCOL_SAVEPRESET, MAX_INQUIRY_BUFLENGTH+1, &CommProtocol::savePreset },
    { COMMPROTOCOL_WRITE_AFE_REG, 4, &CommProtocol::writeAFERegisters },
    { COMMPROTOCOL_READ_AFE_REG, 1, &CommProtocol::readAFERegisters },
    { COMMPROTOCOL_WRITE_DAC_REG, 4, &CommProtocol::writeDACRegisters },
    { COMMPROTOCOL_READ_DAC_REG, 2, &CommProtocol::readDACRegisters },
    { COMMPROTOCOL_WRITE_SSERIAL, 2, &CommProtocol::writeSensorSerialNumber },
    { COMMPROTOCOL_READ_SSERIAL, 1, &CommProtocol::readSensorSerialNumber },
	{ COMMPROTOCOL_WRITE_BOARDSERIAL, 2, &CommProtocol::writeBoardSerialNumber },
	{ COMMPROTOCOL_READ_BOARDSERIAL, 1, &CommProtocol::readBoardSerialNumber },
	{ COMMPROTOCOL_READ_FWVERSION, 1, &CommProtocol::readFirmwareVersion }
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

void CommProtocol::writeValue(unsigned long value, bool last) {
    
    writeValue((unsigned char)((value>>24)&0xFF), false);
    writeValue((unsigned char)((value>>16)&0xFF), false);
    writeValue((unsigned char)((value>>8)&0xFF), false);
    writeValue((unsigned char)(value&0xFF), last);
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

// Function handler: retrieve the IIR denominator values for a specific channel
bool CommProtocol::getSampleIIRDenominators(CommProtocol* context, unsigned char cmdOffset) {

    unsigned char iirDen1 = 0;
    unsigned char iirDen2 = 0;
    unsigned char channel = context->getParameter(0);
    if (!context->sensorsArray->getSampleIIRDenominators(channel, &iirDen1, &iirDen2)) {
        return false;
    }
    
    context->buffer[0] = COMMPROTOCOL_HEADER;
    context->buffer[1] = validCommands[cmdOffset].commandID;
    context->buffer[2] = 0;
    context->writeValue(channel, false);
    context->writeValue(iirDen1, false);
    context->writeValue(iirDen2, true);
    
    return true;
}

// Function handler: set the IIR denominator values for a specific channel
bool CommProtocol::setSampleIIRDenominators(CommProtocol* context, unsigned char cmdOffset) {

    unsigned char channel = context->getParameter(0);
    unsigned char iirDenom1 = context->getParameter(1);
    unsigned char iirDenom2 = context->getParameter(2);

    if (context->sensorsArray->setSampleIIRDenominators(channel, iirDenom1, iirDenom2)) {
        return context->renderOKAnswer(cmdOffset, channel);
    }
    return false;
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

// Function handler: write the AFE related configuration registers
bool CommProtocol::writeAFERegisters(CommProtocol* context, unsigned char cmdOffset) {

    unsigned char channel = context->getParameter(0);
    unsigned char tia = context->getParameter(1);
    unsigned char ref = context->getParameter(2);
    unsigned char mode = context->getParameter(3);
    
    if (context->sensorsArray->writeAFERegisters(channel, tia, ref, mode)) {
        return context->renderOKAnswer(cmdOffset, channel);
    }
    
    return false;
}

// Function Handler: read the AFE related configuration registers
bool CommProtocol::readAFERegisters(CommProtocol* context, unsigned char cmdOffset) {
    
    unsigned char tia, ref, mode;
    unsigned char channel = context->getParameter(0);
    
    if (!context->sensorsArray->readAFERegisters(channel, &tia, &ref, &mode)) {
        return false;
    }
    
    context->buffer[0] = COMMPROTOCOL_HEADER;
    context->buffer[1] = validCommands[cmdOffset].commandID;
    context->buffer[2] = 0;
    context->writeValue(channel, false);
    context->writeValue(tia, false);
    context->writeValue(ref, false);
    context->writeValue(mode, true);

    return true;
}

bool CommProtocol::writeDACRegisters(CommProtocol* context, unsigned char cmdOffset) {

    unsigned char channel = context->getParameter(0);
    unsigned char subChannel= context->getParameter(1);
    unsigned char msbValue = context->getParameter(2);
    unsigned char lsbValue = context->getParameter(3);
    unsigned char gain = context->getParameter(4);
    
    if (context->sensorsArray->writeDACRegisters(channel, subChannel, (((unsigned short)msbValue<<8)|(lsbValue)), gain!=0)) {
        return context->renderOKAnswer(cmdOffset, channel);
    }
    
    return false;
}

bool CommProtocol::readDACRegisters(CommProtocol* context, unsigned char cmdOffset) {

    unsigned short value;
    bool gain;
    
    unsigned char channel = context->getParameter(0);
    unsigned char subChannel = context->getParameter(1);
    
    if (!context->sensorsArray->readDACRegisters(channel, subChannel, &value, &gain)) {
        return false;
    }

    context->buffer[0] = COMMPROTOCOL_HEADER;
    context->buffer[1] = validCommands[cmdOffset].commandID;
    context->buffer[2] = 0;
    context->writeValue(channel, false);
    context->writeValue(subChannel, false);
    context->writeValue(value, false);
    context->writeValue((unsigned char)((gain)? 0x01 : 0x00), true);
    
    return true;
}


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

bool CommProtocol::readSensorSerialNumber(CommProtocol* context, unsigned char cmdOffset) {

	unsigned char result[MAX_SERIAL_BUFLENGTH];
    unsigned char channel = context->getParameter(0);

    context->sensorsArray->readSensorSerialNumber(channel, result, MAX_SERIAL_BUFLENGTH);

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


bool CommProtocol::readFirmwareVersion(CommProtocol* context, unsigned char cmdOffset) {

    context->buffer[0] = COMMPROTOCOL_HEADER;
    context->buffer[1] = validCommands[cmdOffset].commandID;
    context->buffer[2] = 0;
    context->writeString((unsigned char*)FIRMWARE_VERSON, true);

	return true;
}

/* ========================================================================
 * Copyright 2015 EUROPEAN UNION
 *
 * Licensed under the EUPL, Version 1.1 or - as soon they will be approved by 
 * the European Commission - subsequent versions of the EUPL (the "Licence"); 
 * You may not use this work except in compliance with the Licence. 
 * You may obtain a copy of the Licence at: http://ec.europa.eu/idabc/eupl
 * Unless required by applicable law or agreed to in writing, software distributed 
 * under the Licence is distributed on an "AS IS" basis, WITHOUT WARRANTIES OR 
 * CONDITIONS OF ANY KIND, either express or implied. See the Licence for the 
 * specific language governing permissions and limitations under the Licence.
 * Date: 02/04/2015
 * Authors
 * - Michel Gerboles  - michel.gerboles@jrc.ec.europa.eu,  
 *                     European Commission - Joint Research Centre, 
 * - Laurent Spinelle - laurent.spinelle@jrc.ec.europa.eu,
 *                     European Commission - Joint Research Centre, 
 * - Marco Signorini  - marco.signorini@liberaintentio.com
 * 
 * ======================================================================== 
 */

#ifndef COMMPROTOCOL_H
#define	COMMPROTOCOL_H

#define COMMPROTOCOL_HEADER             '{'
#define COMMPROTOCOL_TRAILER            '}'
#define COMMPROTOCOL_ERROR              "{*}"
#define COMMPROTOCOL_BUFFER_LENGTH      32

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
#define COMMPROTOCOL_SET_IIRDENOMVALUES 'A'
#define COMMPROTOCOL_GET_IIRDENOMVALUES 'B'
#define COMMPROTOCOL_LASTSAMPLE         'G'
#define COMMPROTOCOL_FREEMEMORY         'M'
#define COMMPROTOCOL_LOADPRESET         'L'
#define COMMPROTOCOL_SAVEPRESET         'W'
#define COMMPROTOCOL_WRITE_AFE_REG      'R'
#define COMMPROTOCOL_READ_AFE_REG       'T'
#define COMMPROTOCOL_WRITE_DAC_REG      'C'
#define COMMPROTOCOL_READ_DAC_REG       'H'

#define MAX_INQUIRY_BUFLENGTH           8

class SensorsArray;

class CommProtocol {
public:
    CommProtocol(SensorsArray *sensors);
    virtual ~CommProtocol();
    
    void timerTick();
    void onDataReceived(unsigned char pivotChar);
    
private:
    void reset();
    void processBuffer();
    bool renderOKAnswer(unsigned char cmdOffset, unsigned char param);
    unsigned char getParameter(unsigned char parNum);
    void writeValue(unsigned char value, bool last);
    void writeValue(unsigned short value, bool last);
    void writeValue(unsigned long value, bool last);
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
    static bool setSampleIIRDenominators(CommProtocol* context, unsigned char cmdOffset);
    static bool getSampleIIRDenominators(CommProtocol* context, unsigned char cmdOffset);
    static bool getFreeMemory(CommProtocol* context, unsigned char cmdOffset);
    static bool lastSample(CommProtocol* context, unsigned char cmdOffset);
    static bool sensorInquiry(CommProtocol* context, unsigned char cmdOffset);
    static bool loadPreset(CommProtocol* context, unsigned char cmdOffset);
    static bool savePreset(CommProtocol* context, unsigned char cmdOffset);
    static bool writeAFERegisters(CommProtocol *context, unsigned char cmdOffset);
    static bool readAFERegisters(CommProtocol *context, unsigned char cmdOffset);
    static bool writeDACRegisters(CommProtocol *context, unsigned char cmdOffset);
    static bool readDACRegisters(CommProtocol *context, unsigned char cmdOffset);
    
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
    
    unsigned short timer;
    
    SensorsArray* sensorsArray;                 // A reference to the array sensor
};

#endif	/* COMMPROTOCOL_H */


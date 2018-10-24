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


#include "mcc_generated_files/mcc.h"
#include "drvvz89.h"
#include "drvsystimer.h"

#define DRV_VZ89_VZ89T_ADDRESS 0x70
#define DRV_VZ89_GET_STATUS_COMMAND 0x09

#define DRV_VZ89_MAX_I2C_TIMEOUT 10
#define DRV_VZ89_EXECUTION_TIME_WAIT    25  /* > 100 ms */

#define DRV_VZ89_MAX_ALLOWED_DIFF_BTW_SAMPLES   0x1000

typedef enum drvvz89status_ {
    SENSOR_CHECKING,
    IDLE,
    START,
    DATA_WAIT,
    DATA_READY,
    SENSOR_NOT_FOUND,
} drvvz89status;

typedef struct drvvz89data_ {
    drvvz89status status;
    uint32_t timer;
    vz89_data sensorData;
} drvvz89data;

drvvz89data drvVz89Data;

// Internal function prototypes
void I2CStart(void);
void I2CStop(void);
bool I2CBusReset(void);
void I2CSendAck(void);
void I2CSendNak(void);
void I2CWait(void);
bool nackDetected(void);
bool I2CSendByte(unsigned char value);
bool I2CReadByte(uint8_t *value);

bool sendStatusCommandToSensor(void);
bool readStatusAnswerFromSensor(void);
bool waitForCommandExecution(void);

// To be removed if inserting a delay between sending command and reading answer removes glitches
bool readValuesFromSensor(void);
void removeGlitches(void);


void DRV_VZ89Init(void) {
    
    drvVz89Data.status = SENSOR_CHECKING;
    drvVz89Data.sensorData.rawSensorValueTm1 = 0;
    drvVz89Data.sensorData.rawSensorValue = 0;
}


void DRV_VZ89Task(void) {
    
    switch (drvVz89Data.status) {
        
        case SENSOR_CHECKING: {
            
            if (sendStatusCommandToSensor()) {
                drvVz89Data.status = IDLE;
            } else {
                drvVz89Data.status = SENSOR_NOT_FOUND;
            }
            
        } break;
        
        case IDLE:
        case SENSOR_NOT_FOUND:
        case DATA_READY:
            break;
            
        case START: {
            if (sendStatusCommandToSensor()) {
                drvVz89Data.status = DATA_WAIT;
            } else {
                drvVz89Data.status = IDLE;
            }
        } 
        break;
        
        case DATA_WAIT: {
            if (waitForCommandExecution()) {
                if (readStatusAnswerFromSensor()) {
                    // removeGlitches();
                    drvVz89Data.status = DATA_READY;
                } else {
                    drvVz89Data.status = IDLE;
                }
            }
        } 
        break;
            
        default:
            drvVz89Data.status = SENSOR_CHECKING;
            break;
    }
}

uint8_t DRV_VZ89_Ready(void) {
    return drvVz89Data.status == IDLE;
}

void DRV_VZ89_AskData(void) {
    
    if (drvVz89Data.status == IDLE) {
        drvVz89Data.status = START;
    }
}

vz89_data* DRV_VZ89_GetReadData(void) {
    
    if (drvVz89Data.status != DATA_READY) {
        return NULL;
    }
    
    drvVz89Data.status = IDLE;
    
    return (drvVz89Data.sensorData.rawSensorValue == 0) ? NULL : &drvVz89Data.sensorData;
}

void I2CInit(void) {
    
    // MSSP2 is used as Master I2C, 90KHz
	SSP2STAT |= 0x80; 
	SSP2CON1 = 0x28;   
    SSP2CON3 = 0x00;
	SSP2ADD = 0xAF;
    
    // Set RC4 and RC5 pin status
    LATCbits.LATC4 = 0;
    LATCbits.LATC5 = 0;
    TRISCbits.TRISC4 = 1;
    TRISCbits.TRISC5 = 1;
    ANSELCbits.ANSELC4 = 0;
    ANSELCbits.ANSELC5 = 0;
    WPUCbits.WPUC4 = 0;
    WPUCbits.WPUC5 = 0;
    ODCONCbits.ODCC4 = 0;
    ODCONCbits.ODCC5 = 0;
    
    // Set SDA2 and SCL2 to RC4 and RC5
    PPSLOCK = 0x55;
    PPSLOCK = 0xAA;
    PPSLOCKbits.PPSLOCKED = 0x00;
    SSP2DATPPSbits.SSPDATPPS = 0x14;   //RC4->MSSP2:SDA2;
    RC4PPS = 0x12;   //RC4->MSSP2:SDA2;
    SSP2CLKPPSbits.SSPCLKPPS = 0x15;   //RC5->MSSP2:SCL2;
    RC5PPS = 0x11;   //RC5->MSSP2:SCL2;
    PPSLOCK = 0x55;
    PPSLOCK = 0xAA;
    PPSLOCKbits.PPSLOCKED = 0x01; // lock PPS
    
    // Disable interrupts for MSSP2
    PIR3bits.SSP2IF = 0;
    PIE3bits.SSP2IE = 0;
}
     
void I2CStart(void) {
	SSP2CON2bits.SEN = 1;
	while(SSP2CON2bits.SEN);
}
 
void I2CStop(void) {
	SSP2CON2bits.PEN = 1;
	while(SSP2CON2bits.PEN);
}

bool I2CBusReset(void) {
    I2CStop();
    return false;
}
 
void I2CSendAck(void) {
	SSP2CON2bits.ACKDT = 0;
	SSP2CON2bits.ACKEN = 1;
	while(SSP2CON2bits.ACKEN);
}
 
void I2CSendNak(void) {
	SSP2CON2bits.ACKDT = 1;
	SSP2CON2bits.ACKEN = 1;
	while(SSP2CON2bits.ACKEN);
}
 
void I2CWait(void) {
	while ((SSP2CON2 & 0x1F ) || ( SSP2STAT & 0x04 ) );
}

bool nackDetected(void) {
    return SSP2CON2bits.ACKSTAT;
}
 
bool I2CSendByte(unsigned char value) {
	SSP2BUF = value;
    if ( SSP2CON1bits.WCOL ) {
        SSP2CON1bits.WCOL = 0;
        return false;
    }
	while(SSP2STATbits.BF);
	I2CWait();
    
    return !nackDetected();
}
 
bool I2CReadByte(uint8_t *value) {
    uint32_t timestamp = DRV_SysTimerGetTime();
	SSP2CON2bits.RCEN = 1; 
	while(!SSP2STATbits.BF) {
        if ((DRV_SysTimerGetTime() - timestamp) > DRV_VZ89_MAX_I2C_TIMEOUT) {
            return false;
        }
    }
	*value = SSP2BUF;
	I2CWait();
    
    return true;
}

bool sendStatusCommandToSensor(void) {
    I2CStart();
    if (!I2CSendByte(DRV_VZ89_VZ89T_ADDRESS<<1)) {
        return I2CBusReset();
    }
    if (!I2CSendByte(DRV_VZ89_GET_STATUS_COMMAND)) {
        return I2CBusReset();
    }

    I2CStop();
    
    // Initialize the internal timer to be used
    // to wait for command execution
    drvVz89Data.timer = DRV_SysTimerGetTime();
    
    return true;
}

bool readStatusAnswerFromSensor(void) {
    
    I2CStart();
    
    if (!I2CSendByte((DRV_VZ89_VZ89T_ADDRESS<<1) | 0x01)) {
        return I2CBusReset();
    }

    uint8_t readBuffer[6];
    for (uint8_t n = 0; n < 5; n++) {
        if (!I2CReadByte(readBuffer+n)) {
            return I2CBusReset();
        }
        I2CSendAck();
    }
    
    if (!I2CReadByte(readBuffer+5) ) {
        return I2CBusReset();
    }
    I2CSendNak();
    I2CStop();
    
    unsigned char lsb = readBuffer[3];
    unsigned char misb = readBuffer[4];
    unsigned char msb = readBuffer[5];

    drvVz89Data.sensorData.rawSensorValue = ((((uint32_t)msb) << 16) & 0xFF0000) | (((uint32_t)misb)<<8) & 0xFF00 | lsb;
    
    return true;    
}

bool waitForCommandExecution(void) {
    return ((DRV_SysTimerGetTime() - drvVz89Data.timer) > DRV_VZ89_EXECUTION_TIME_WAIT);
}

bool readValuesFromSensor(void) {
    
    I2CStart();
    if (!I2CSendByte(DRV_VZ89_VZ89T_ADDRESS<<1)) {
        return I2CBusReset();
    }
    if (!I2CSendByte(DRV_VZ89_GET_STATUS_COMMAND)) {
        return I2CBusReset();
    }

    I2CStop();
    I2CStart();
    
    if (!I2CSendByte((DRV_VZ89_VZ89T_ADDRESS<<1) | 0x01)) {
        return I2CBusReset();
    }

    uint8_t readBuffer[6];
    for (uint8_t n = 0; n < 5; n++) {
        if (!I2CReadByte(readBuffer+n)) {
            return I2CBusReset();
        }
        I2CSendAck();
    }
    
    if (!I2CReadByte(readBuffer+5) ) {
        return I2CBusReset();
    }
    I2CSendNak();
    I2CStop();
    
    unsigned char lsb = readBuffer[3];
    unsigned char misb = readBuffer[4];
    unsigned char msb = readBuffer[5];

    drvVz89Data.sensorData.rawSensorValue = ((((uint32_t)msb) << 16) & 0xFF0000) | (((uint32_t)misb)<<8) & 0xFF00 | lsb;
    
    return true;
}

void removeGlitches(void) {
    
    // If T-1 sample is available and differs more than what we expect, 
    // forget last sample and use T-1 instead.
    // This should reduce glitches and/or possible MOX firmware issues found in our 
    // test sample device
    if (drvVz89Data.sensorData.rawSensorValueTm1 != 0) {
        
        if (drvVz89Data.sensorData.rawSensorValue > drvVz89Data.sensorData.rawSensorValueTm1) {
            if ((drvVz89Data.sensorData.rawSensorValue - drvVz89Data.sensorData.rawSensorValueTm1) > DRV_VZ89_MAX_ALLOWED_DIFF_BTW_SAMPLES) {
                drvVz89Data.sensorData.rawSensorValue = drvVz89Data.sensorData.rawSensorValueTm1;
            }
        } else {
            if ((drvVz89Data.sensorData.rawSensorValueTm1 - drvVz89Data.sensorData.rawSensorValue) > DRV_VZ89_MAX_ALLOWED_DIFF_BTW_SAMPLES) {
                drvVz89Data.sensorData.rawSensorValue = drvVz89Data.sensorData.rawSensorValueTm1;
            }
        }
    }
    
    drvVz89Data.sensorData.rawSensorValueTm1 = drvVz89Data.sensorData.rawSensorValue;
}
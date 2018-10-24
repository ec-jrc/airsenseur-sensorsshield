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
#include "appsensorbus.h"


#define COMMPROTOCOL_PTM_HOST_HEADER    '['
#define COMMPROTOCOL_PTM_HOST_TRAILER   ']'

#define COMMPROTOCOL_PTM_SLAVE_HEADER   '('
#define COMMPROTOCOL_PTM_SLAVE_TRAILER  ')'

#define COMMPROTOCOL_PTM_VERSION        '0'

#define COMMPROTOCOL_BOARDID_BROADCAST  0xFF
#define COMMPROTOCOL_MYBOARDID_MSB      '0'

#define COMMPROTOCOL_SHIELDPAYLOAD_HEADER   '{'
#define COMMPROTOCOL_SHIELDPAYLOAD_TRAILER  '}'

#define SENSORBUS_FRAME_TIMEOUT         4000  /* In milliseconds (max 65535) */
#define LED_ENABLED_TIMEOUT             60000 /* In milliseconds (max 65535) */
#define LED_FLASH_TIMER                 250   /* In milliseconds */

#define START_RXLED_PULSE  { if (appSensorBusData.flags.DEBUGLEDSEnabled) { appSensorBusData.rxLEDTimer = appSensorBusData.timerCounter; appSensorBusData.flags.RXLEDEnabled = true; RXLED_SetLow(); } }
#define START_TXLED_PULSE  { if (appSensorBusData.flags.DEBUGLEDSEnabled) { appSensorBusData.txLEDTimer = appSensorBusData.timerCounter; appSensorBusData.flags.TXLEDEnabled = true; TXLED_SetLow(); } }

typedef enum _rxstatuses {
    IDLE, HEADER_FOUND, VERSION_FOUND, BOARDID1_FOUND, BOARDID_FOUND,
} rxstatuses;

typedef struct _appsensorbusflags {
    unsigned int TXLEDEnabled : 1;
    unsigned int RXLEDEnabled : 1;
    unsigned int DEBUGLEDSEnabled : 1;        
} appsensorbusflags;

typedef struct _appsensorbus {
    
    rxstatuses rxStatus;
    
    uint8_t myBoardId;
    uint8_t rxBoardId;
    char rxBoardIdLSB;
    
    uint16_t timerCounter;
    uint16_t rxProtocolTimeout;

    uint16_t txLEDTimer;
    uint16_t rxLEDTimer;
    uint16_t lEDEnabledTimer;
    appsensorbusflags flags;
    
} appsensorbus;

static appsensorbus appSensorBusData;


// Initialize the SensorBus layer
void APP_SensorBusInit(uint8_t myBoardId) {

    myBoardId &= 0x0F;
    appSensorBusData.myBoardId = myBoardId;

    // Evaluate the boardId LSB for future use
    if (myBoardId < 10) {
        appSensorBusData.rxBoardIdLSB = '0' + myBoardId;
    } else {
        appSensorBusData.rxBoardIdLSB = 'A' + myBoardId - 10;
    }
    
    // Initialize all other variables
    appSensorBusData.rxStatus = IDLE;
    appSensorBusData.rxBoardId = COMMPROTOCOL_BOARDID_BROADCAST;
    
    appSensorBusData.timerCounter = 0;
    appSensorBusData.rxProtocolTimeout = 0;
    appSensorBusData.lEDEnabledTimer = 0;
    appSensorBusData.txLEDTimer = 0;
    appSensorBusData.rxLEDTimer = 0;
    
    appSensorBusData.flags.DEBUGLEDSEnabled = true;
}

// Timer tick (for timeout and other stuffs). 
// To be called periodically each 1000ms
void APP_SensorBusTimerTick(void) {
    appSensorBusData.timerCounter++;
    
    if (appSensorBusData.flags.DEBUGLEDSEnabled) {
        if ((appSensorBusData.timerCounter - appSensorBusData.lEDEnabledTimer) > LED_ENABLED_TIMEOUT) {
            appSensorBusData.flags.DEBUGLEDSEnabled = false;
        }
        
        if (appSensorBusData.flags.TXLEDEnabled) {
            if ((appSensorBusData.timerCounter - appSensorBusData.txLEDTimer) > LED_FLASH_TIMER) {
                appSensorBusData.flags.TXLEDEnabled = false;
                TXLED_SetHigh();
            }
        } 
        
        if (appSensorBusData.flags.RXLEDEnabled) {
            if ((appSensorBusData.timerCounter - appSensorBusData.rxLEDTimer) > LED_FLASH_TIMER) {
                appSensorBusData.flags.RXLEDEnabled = false;
                RXLED_SetHigh();
            }
        } 
    } else {
        TXLED_SetHigh();
        RXLED_SetHigh();
    }
}

// Receive and evaluate data coming from the SensorBus
void APP_SensorBusRxHandler(void) {
    
    if (!EUSART1_DataReady) {
        return;
    }

    char rxChar = EUSART1_Read();

    // Whatever is my status, if an header is found, go to the IDLE status
    if (rxChar == COMMPROTOCOL_PTM_HOST_HEADER) {
        appSensorBusData.rxStatus = IDLE;
    }
    
    // When not in IDLE, evaluate a timeout
    // (we expect a max defined period between start and end frame)
    if (appSensorBusData.rxStatus  != IDLE) {
        if ((appSensorBusData.timerCounter - appSensorBusData.rxProtocolTimeout) > SENSORBUS_FRAME_TIMEOUT) {
            appSensorBusData.rxStatus = IDLE;
        }
    }
    
    // Evaluate the received char based on current state machine status
    switch (appSensorBusData.rxStatus) {

        case IDLE: {
            // Searching for an header
            if (rxChar == COMMPROTOCOL_PTM_HOST_HEADER) {
                appSensorBusData.rxStatus = HEADER_FOUND;
                appSensorBusData.rxBoardId = COMMPROTOCOL_BOARDID_BROADCAST;
                appSensorBusData.rxProtocolTimeout = appSensorBusData.timerCounter;
            }
        }
        break;

        case HEADER_FOUND: {
            if (rxChar == COMMPROTOCOL_PTM_VERSION) {
                // We expect a compatible protocol version
                appSensorBusData.rxStatus = VERSION_FOUND;
            } else {
                // ... but we found something not valid...
                appSensorBusData.rxStatus = IDLE;
            }
        }
        break;

        case VERSION_FOUND: {
            // We expect an hex digit, msb of target boardId
            appSensorBusData.rxStatus = BOARDID1_FOUND;
            if ((rxChar >= '0') && (rxChar <= '9')) {
                appSensorBusData.rxBoardId = (rxChar - '0') << 4;
            } else if ((rxChar >= 'a') && (rxChar <= 'f')) {
                appSensorBusData.rxBoardId = (rxChar - 'a' + 10) << 4;
            } else if ((rxChar >= 'A') && (rxChar <= 'F')) {
                appSensorBusData.rxBoardId = (rxChar - 'A' + 10) << 4;                    
            } else {
                appSensorBusData.rxStatus = IDLE;
            }
        }
        break;

        case BOARDID1_FOUND: {
            // We expect an hex digit, lsb of target boardId
            appSensorBusData.rxStatus = BOARDID_FOUND;
            if ((rxChar >= '0') && (rxChar <= '9')) {
                appSensorBusData.rxBoardId |= (rxChar - '0');
            } else if ((rxChar >= 'a') && (rxChar <= 'f')) {
                appSensorBusData.rxBoardId |= (rxChar - 'a' + 10);
            } else if ((rxChar >= 'A') && (rxChar <= 'F')) {
                appSensorBusData.rxBoardId |= (rxChar - 'A' + 10);
            } else {
                appSensorBusData.rxStatus = IDLE;
            }

            // Validate the received boardId
            if (appSensorBusData.myBoardId != appSensorBusData.rxBoardId) {
                appSensorBusData.rxStatus = IDLE;
            } else {
                // Start sending the header to the shield
                EUSART2_Write(COMMPROTOCOL_SHIELDPAYLOAD_HEADER);
            }
        }
        break;

        case BOARDID_FOUND: {
            // Searching for a trailer
            if (rxChar == COMMPROTOCOL_PTM_HOST_TRAILER) {
                // End of frame found. 
                // Send the trailer to the shield
                EUSART2_Write(COMMPROTOCOL_SHIELDPAYLOAD_TRAILER);
                
                // Enable answer data transfer going in idle state
                appSensorBusData.rxStatus = IDLE;
                
                // Signal for debug
                START_RXLED_PULSE;

            } else {
                // All received data are considered as payload
                // and will be echoed to the shield serial line
                EUSART2_Write(rxChar);
            }
        }
        break;

        default: {
            appSensorBusData.rxStatus = IDLE;
        }
    }
}

// Receive and evaluate data coming from the shield
void APP_SensorBusTxHandler(void) {
    
    // Nothing to do if receiver is not in IDLE status
    if (appSensorBusData.rxStatus != IDLE) {
        return;
    }
    
    // Nothing to do if no data are flowing from the shield
    if (!EUSART2_DataReady) {
        return;
    }
    
    // Evaluate the char received from the shield
    char shieldChar = EUSART2_Read();
    
    
    if (shieldChar == COMMPROTOCOL_SHIELDPAYLOAD_HEADER) {
        
        // Found a protocol header: send back to the host a SensorBus header, version and board Id
        EUSART1_Write(COMMPROTOCOL_PTM_SLAVE_HEADER);
        EUSART1_Write(COMMPROTOCOL_PTM_VERSION);
        EUSART1_Write(COMMPROTOCOL_MYBOARDID_MSB);
        EUSART1_Write(appSensorBusData.rxBoardIdLSB);
        
    } else if (shieldChar == COMMPROTOCOL_SHIELDPAYLOAD_TRAILER) {
        
        // Found a protocol trailer: send back to the host a frame termination character
        EUSART1_Write(COMMPROTOCOL_PTM_SLAVE_TRAILER);
        
        // And signal for debug
        START_TXLED_PULSE;
        
    } else {
        
        // Payload found: send back to the host through the SensorBus
        EUSART1_Write(shieldChar);
    }
}

// Enable LED debugging for several seconds
void APP_SensorBusEnableLEDs(void) {
    
    appSensorBusData.lEDEnabledTimer = appSensorBusData.timerCounter;
    appSensorBusData.flags.DEBUGLEDSEnabled = true;
}
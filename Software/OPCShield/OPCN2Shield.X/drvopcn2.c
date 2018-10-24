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
#include "drvopcn2.h"
#include "drvsystimer.h"

typedef enum drvopcn2commands_ {
    DIGPOT_INVALID_FIRST,
    DIGPOT_FAN_OFF,
    DIGPOT_FAN_ON,
    DIGPOT_LASER_OFF,
    DIGPOT_LASER_ON,
    DIGPOT_FANANDLASER_ON,
    DIGPOT_FANANDLASER_OFF,
    DIGPOT_SET_FAN_POWER,
    DIGPOT_READ_INFORMATION_STRING,
    DIGPOT_READ_SERIALNUMBER,
    DIGPOT_READ_CONFIGURATION,
    DIGPOT_WRITE_CONFIGURATION,
    DIGPOT_READ_HISTOGRAMDATA,
    DIGPOT_READ_PMDATA,
    DIGPOT_INVALID_LAST,
} drv_opcn2commands;

typedef struct drvopcn2commandinfo_ {
    drv_opcn2commands command;
    uint8_t commandByte;
    uint8_t staticPrameterValue;    
    uint8_t useStaticParameter;
    uint8_t dynamicParSize;
    uint8_t readWrite;
} drv_opcn2commandinfo;

#define DRV_OPCN2_INTERNAL_BUFFER_LENGTH     64
#define USE_STATIC_PAR 0x01
#define DONT_USE_STATIC_PAR 0x00
#define READ_OPERATION 0x01
#define WRITE_OPERATION 0x00
#define NUM_OF_HISTOGRAMS_TO_SKIP   3
#define COMMANDLIST_INVALID_ID  (sizeof(commandList)/sizeof(drv_opcn2commandinfo))

const drv_opcn2commandinfo commandList[] = {
    { DIGPOT_READ_HISTOGRAMDATA, 0x30, 0xC0, DONT_USE_STATIC_PAR, sizeof(opcn2_histogram_data), READ_OPERATION },
    { DIGPOT_READ_PMDATA, 0x32, 0xC0, DONT_USE_STATIC_PAR, sizeof(opcn2_pmdata), READ_OPERATION },
    { DIGPOT_FAN_OFF, 0x03, 0x05, USE_STATIC_PAR, 0x00, WRITE_OPERATION },
    { DIGPOT_FAN_ON, 0x03, 0x04, USE_STATIC_PAR, 0x00, WRITE_OPERATION },
    { DIGPOT_LASER_OFF, 0x03, 0x03, USE_STATIC_PAR, 0x00, WRITE_OPERATION },
    { DIGPOT_LASER_ON, 0x03, 0x02, USE_STATIC_PAR, 0x00, WRITE_OPERATION },
    { DIGPOT_FANANDLASER_ON, 0x03, 0x00, USE_STATIC_PAR, 0x00, WRITE_OPERATION },
    { DIGPOT_FANANDLASER_OFF, 0x03, 0x01, USE_STATIC_PAR, 0x00, WRITE_OPERATION },
    { DIGPOT_SET_FAN_POWER, 0x42, 0x00, USE_STATIC_PAR, 1, WRITE_OPERATION },
    { DIGPOT_READ_INFORMATION_STRING, 0x3F, 0xC0, DONT_USE_STATIC_PAR, 60, READ_OPERATION },
    { DIGPOT_READ_SERIALNUMBER, 0x10, 0xC0, DONT_USE_STATIC_PAR, 60, READ_OPERATION },
    { DIGPOT_READ_CONFIGURATION, 0x3D, 0xC0, DONT_USE_STATIC_PAR, sizeof(opcn2_config_variables), READ_OPERATION },
    { DIGPOT_WRITE_CONFIGURATION, 0x3B, 0xC0, DONT_USE_STATIC_PAR, sizeof(opcn2_config_variables), WRITE_OPERATION }
};

typedef enum drvopcn2status_ {
    IDLE,
    SENT_COMMAND,
    SENT_STATIC_PARAMETER,
    WRITING_DYNAMIC_PARAMETERS,
    READING_DATA
} drv_opcn2status;

typedef struct drvopcn2data_ {
    drv_opcn2status status;                                 // Main state machine status
    drv_opcn2commands lastCmdBufferMod;                     // Last command that modified the buffer content
    uint8_t tmpBuffer[DRV_OPCN2_INTERNAL_BUFFER_LENGTH];    // Buffer for in/out data
    uint8_t bufferOffset;                                   // Internal buffer offset
    uint8_t curProcCmdOffset;                               // Offset pointing to currently processed command in the command list
    uint32_t timer;
    uint8_t histGracePeriod;
    bool laserOn;
    bool fanOn;
} drv_opcn2data;

// Our internal data instance
drv_opcn2data drvOpcn2Data;

// Internal functions prototypes
static uint8_t OPCN2_Write(uint8_t data);
static uint8_t getCmdListOffset(drv_opcn2commands cmd);
static uint8_t checkCommandOffsetValidity(void);
static void resetStateMachine(void);
static void* getDataFor(drv_opcn2commands command);
static opcn2_histogram_data* validateHistogramData(opcn2_histogram_data* histogramData);

// Functions implementation
void DRV_OPCN2Init(void) {
        
    resetStateMachine();
    drvOpcn2Data.lastCmdBufferMod = DIGPOT_INVALID_FIRST;
    drvOpcn2Data.bufferOffset = 0;
    drvOpcn2Data.timer = DRV_SysTimerGetTime();
    drvOpcn2Data.laserOn = false;
    drvOpcn2Data.fanOn = false;
    drvOpcn2Data.histGracePeriod = NUM_OF_HISTOGRAMS_TO_SKIP;
}

void DRV_OPCN2Task(void) {
    
    // Check for command offset validity
    checkCommandOffsetValidity();
    
    // Take the proper command info pointer
    const drv_opcn2commandinfo* cmdInfo = commandList + drvOpcn2Data.curProcCmdOffset; 
    
    // When reading flash stored data we need to explicitly enable Table Read. 
    // I don't know if this is for a bug in the compiler... 
    NVMCON1bits.NVMREG = 2;
    
    // Do something based on current status
    switch (drvOpcn2Data.status) {
        
        case IDLE: {
            // Check if a new command has to be processed
            if (drvOpcn2Data.curProcCmdOffset != COMMANDLIST_INVALID_ID) {
                
                // Yes. Send the command byte to the OPC
                OPCN2_Write(cmdInfo->commandByte);
                OPC_SS_SetHigh();
                
                drvOpcn2Data.status = SENT_COMMAND;
                drvOpcn2Data.timer = DRV_SysTimerGetTime();
            }
        }
        break;
        
        case SENT_COMMAND: {
            // 20 ms wait time between the command byte and following read/write ops
            if ( (DRV_SysTimerGetTime() - drvOpcn2Data.timer) > ((uint32_t)2)) {
                
                if (cmdInfo->useStaticParameter == USE_STATIC_PAR) {
                    OPCN2_Write(cmdInfo->staticPrameterValue);
                }
                drvOpcn2Data.status = SENT_STATIC_PARAMETER;                
            }
        }
        break;
        
        case SENT_STATIC_PARAMETER: {

            if (cmdInfo->readWrite == WRITE_OPERATION) {
                if (cmdInfo->dynamicParSize != 0) {
                    drvOpcn2Data.bufferOffset = 0;
                    drvOpcn2Data.status = WRITING_DYNAMIC_PARAMETERS;
                } else {
                    resetStateMachine();
                }
            } else  {
                drvOpcn2Data.bufferOffset = 0;
                drvOpcn2Data.lastCmdBufferMod = cmdInfo->command;
                drvOpcn2Data.status = READING_DATA;
            }
        }
        break;
        
        case WRITING_DYNAMIC_PARAMETERS: {
            if (drvOpcn2Data.bufferOffset < cmdInfo->dynamicParSize) {
                OPCN2_Write(drvOpcn2Data.tmpBuffer[drvOpcn2Data.bufferOffset]);
                drvOpcn2Data.bufferOffset++;
            } else {
                resetStateMachine();
            }
        }
        break;
        
        case READING_DATA: {
            if (drvOpcn2Data.bufferOffset < cmdInfo->dynamicParSize) {
                drvOpcn2Data.tmpBuffer[drvOpcn2Data.bufferOffset] = OPCN2_Write(cmdInfo->staticPrameterValue);
                drvOpcn2Data.bufferOffset++;
            } else {
                resetStateMachine();
            }
        }
        break;
        
        default: {
            // This should never happen
            resetStateMachine();
        }
    }
}

static uint8_t OPCN2_Write(uint8_t data) {
    
    uint8_t result;
    
    OPC_SS_SetLow();
    Nop();
    Nop();
    Nop();
    Nop();
    Nop();
    Nop();
    Nop();
    Nop();
    result = SPI1_Exchange8bit(data);
    
    return result;
}

// Search the specified command in the command list and return the offset
// were the command is located in the list.
// Returns COMMANDLIST_INVALID_ID if the command was not found
static uint8_t getCmdListOffset(drv_opcn2commands cmd) {

    // When reading flash stored data we need to explicitly enable Table Read. 
    // I don't know if this is for a bug in the compiler... 
    NVMCON1bits.NVMREG = 2;
    
    for (uint8_t n = 0; n < COMMANDLIST_INVALID_ID; n++) {

        if (commandList[n].command == cmd) {
            return n;
        }
    }
    
    return COMMANDLIST_INVALID_ID;
}

// Check for command offset validity and restore the IDLE
// status if not satisfied
static uint8_t checkCommandOffsetValidity(void) {
    
    if (drvOpcn2Data.curProcCmdOffset >= COMMANDLIST_INVALID_ID) {
        resetStateMachine();
        return 0x00;
    }
    
    return 0x01;
}

static void resetStateMachine(void) {
    drvOpcn2Data.status = IDLE;
    drvOpcn2Data.curProcCmdOffset = COMMANDLIST_INVALID_ID;
    OPC_SS_SetHigh();    
}

static void* getDataFor(drv_opcn2commands command) {
    if (drvOpcn2Data.lastCmdBufferMod != command) {
        return NULL;
    }
    drvOpcn2Data.lastCmdBufferMod = DIGPOT_INVALID_FIRST;
    return (void*)drvOpcn2Data.tmpBuffer;
}

// Validate read histogram data based on CRC and other factors
static opcn2_histogram_data* validateHistogramData(opcn2_histogram_data* histogramData) {
    
    if (histogramData != NULL) {
        
        uint16_t pivotCrc = 0;
        for (int i = 0; i < HISTOGRAM_BINS_NUM; i++) {
            pivotCrc = pivotCrc + histogramData->bins[i];
        }
        
        if (pivotCrc != histogramData->checksum) {
            return NULL;
        }
        
        // Check for data validity
        if ((histogramData->sampleFlowRate <= 0.0f) || (histogramData->samplingPeriod <= 0.0f)) {
            return NULL;
        }
        
        // This should never happen, but it's always better to check and correct
        if (histogramData->pm.pm1 < 0.0f) {
            histogramData->pm.pm1 = 0.0f;
        }
        if (histogramData->pm.pm25 < 0) {
            histogramData->pm.pm25 = 0;
        }
        if (histogramData->pm.pm10 < 0) {
            histogramData->pm.pm10 = 0;
        }
    }
    
    return histogramData;
}

uint8_t DRV_OPCN2_Ready(void) {
    return drvOpcn2Data.status == IDLE;
}

void DRV_OPCN2_SetLaser(uint8_t on) {
    drvOpcn2Data.curProcCmdOffset = getCmdListOffset((on != 0)?DIGPOT_LASER_ON:DIGPOT_LASER_OFF);
    drvOpcn2Data.laserOn = on;
    drvOpcn2Data.histGracePeriod = NUM_OF_HISTOGRAMS_TO_SKIP;
}

void DRV_OPCN2_SetFan(uint8_t on) {
    drvOpcn2Data.curProcCmdOffset = getCmdListOffset((on != 0)?DIGPOT_FAN_ON:DIGPOT_FAN_OFF);    
    drvOpcn2Data.fanOn = on;
    drvOpcn2Data.histGracePeriod = NUM_OF_HISTOGRAMS_TO_SKIP;
}

void DRV_OPCN2_SetFanAndLaser(uint8_t on) {
    drvOpcn2Data.curProcCmdOffset = getCmdListOffset((on != 0)?DIGPOT_FANANDLASER_ON:DIGPOT_FANANDLASER_OFF);    
    drvOpcn2Data.fanOn = on;
    drvOpcn2Data.laserOn = on;
    drvOpcn2Data.histGracePeriod = NUM_OF_HISTOGRAMS_TO_SKIP;
}

void DRV_OPCN2_SetFanPower(uint8_t power) {
    drvOpcn2Data.tmpBuffer[0] = power;    
    drvOpcn2Data.curProcCmdOffset = getCmdListOffset(DIGPOT_SET_FAN_POWER);
    drvOpcn2Data.lastCmdBufferMod = DIGPOT_SET_FAN_POWER;
}

void DRV_OPCN2_AskInformationString() {
    drvOpcn2Data.lastCmdBufferMod = DIGPOT_INVALID_FIRST;    
    drvOpcn2Data.curProcCmdOffset = getCmdListOffset(DIGPOT_READ_INFORMATION_STRING);
}

char* DRV_OPCN2_GetInformationString() {
    return (char*) getDataFor(DIGPOT_READ_INFORMATION_STRING);
}

void DRV_OPCN2_AskSerialNumberString() {
    drvOpcn2Data.lastCmdBufferMod = DIGPOT_INVALID_FIRST;    
    drvOpcn2Data.curProcCmdOffset = getCmdListOffset(DIGPOT_READ_SERIALNUMBER);
}

char* DRV_OPCN2_GetSerialNumberString() {
    return (char*) getDataFor(DIGPOT_READ_SERIALNUMBER);    
}

void DRV_OPCN2_AskConfigurationVariables() {
    drvOpcn2Data.lastCmdBufferMod = DIGPOT_INVALID_FIRST;    
    drvOpcn2Data.curProcCmdOffset = getCmdListOffset(DIGPOT_READ_CONFIGURATION);
}

opcn2_config_variables* DRV_OPCN2_GetConfigurationVariables() {
    return (opcn2_config_variables*) getDataFor(DIGPOT_READ_CONFIGURATION);    
}

void DRV_OPCN2_WriteConfigurationVariables(opcn2_config_variables *config) {
    
    uint8_t* source = (uint8_t*)config;
    for (uint8_t n = 0; n < sizeof(opcn2_config_variables); n++) {
        drvOpcn2Data.tmpBuffer[n] = *source;
        source++;
    }

    drvOpcn2Data.curProcCmdOffset = getCmdListOffset(DIGPOT_WRITE_CONFIGURATION);
    drvOpcn2Data.lastCmdBufferMod = DIGPOT_WRITE_CONFIGURATION;
}

void DRV_OPCN2_AskHistogramData() {
    if ((!drvOpcn2Data.fanOn) || (!drvOpcn2Data.laserOn) ) {
        return;
    }
    
    // The very first histogram data will be discarded as they does not 
    // provide useful information. They should be asked but discarded when received.
    if (drvOpcn2Data.histGracePeriod != 0) {
        drvOpcn2Data.histGracePeriod--;
    }
    
    drvOpcn2Data.lastCmdBufferMod = DIGPOT_INVALID_FIRST;    
    drvOpcn2Data.curProcCmdOffset = getCmdListOffset(DIGPOT_READ_HISTOGRAMDATA);
}

opcn2_histogram_data* DRV_OPCN2_GetHistogramData() {
    
    // No valid histogram data are available if fan or laser is turned off
    // and if the "grace period" has not been expired (the very first histogram data
    // does not provide useful information so best would be to discard them)
    if ((!drvOpcn2Data.fanOn) || 
        (!drvOpcn2Data.laserOn) || 
        (drvOpcn2Data.histGracePeriod != 0) || 
        (drvOpcn2Data.status != IDLE)) {
        
        return NULL;
    }
    
    return validateHistogramData((opcn2_histogram_data*) getDataFor(DIGPOT_READ_HISTOGRAMDATA));
}

void DRV_OPCN2_AskPMData() {
    drvOpcn2Data.lastCmdBufferMod = DIGPOT_INVALID_FIRST;    
    drvOpcn2Data.curProcCmdOffset = getCmdListOffset(DIGPOT_READ_PMDATA);
}

opcn2_pmdata* DRV_OPCN2_GetPMData() {
    return (opcn2_pmdata*) getDataFor(DIGPOT_READ_PMDATA);
}


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
#include "appcommapi.h"
#include "appsensorbus.h"
#include "appsensorarray.h"

#define APPCOMAPI_TX_BUFFERSIZE   32
#define MAX_INQUIRY_BUFLENGTH     16

typedef enum appcommapi_ids_ {
    ID_ECHO = 'E',
    ID_SENSOR_INQUIRY = 'I',            
    ID_SAMPLE_ENABLE = 'S',
    ID_SAMPLE_DISABLE = 'X',
    ID_SET_SAMPLEPRESC = 'P',
    ID_GET_SAMPLEPRESC = 'Q',
    ID_SET_SAMPLEPOSTS = 'O',
    ID_GET_SAMPLEPOSTS = 'N',
    ID_LASTSAMPLE = 'G',
    ID_LASTSAMPLE_HRES = 'Y',
    ID_FREEMEMORY = 'M',
    ID_LOADPRESET = 'L',
    ID_SAVEPRESET = 'W',
    ID_SET_IIRDENOMVALUES = 'A',
    ID_GET_IIRDENOMVALUES = 'B',
    ID_SET_SAMPLEDECIM = 'D',
    ID_GET_SAMPLEDECIM = 'F',
    ID_ERROR = '*',
} appcommapi_ids;

typedef struct appcomapi_data_ {
    uint8_t currentCmdOffset;
    uint8_t* pBuffer;
} appcomapi_data;

typedef bool (*fpointer)(void);

typedef struct appcommapi_commandinfo_ {
    appcommapi_ids commandId;
    uint8_t parNum;
    fpointer handler;
} appcommapi_commandinfo;


// Internal Function prototypes
static void renderOKAnswer(uint8_t channel);
static void renderKOAnswer();
static uint8_t getParameter(uint8_t parNum);
static void writeValue(uint8_t value);
static void writeShortValue(uint16_t value);
static void writeLongValue(uint32_t value);
static void writeFloatValue(float value);
static void writeString(uint8_t* value);

static bool echo(void);
static bool sensorInquiry(void);
static bool sampleEnable(void);
static bool sampleDisable(void);
static bool setSamplePrescaler(void);
static bool getSamplePrescaler(void);
static bool setSamplePostScaler(void);
static bool getSamplePostScaler(void);
static bool getLastSample(void);
static bool getLastSampleHRes(void);
static bool loadPreset(void);
static bool savePreset(void);

const appcommapi_commandinfo validCommands[] = {
    { ID_ECHO, 0x00, echo },
    { ID_SENSOR_INQUIRY, 0x01, sensorInquiry },
    { ID_SAMPLE_ENABLE, 0x00, sampleEnable },
    { ID_SAMPLE_DISABLE, 0x00, sampleDisable },
    { ID_SET_SAMPLEPRESC, 0x02, setSamplePrescaler },
    { ID_GET_SAMPLEPRESC, 0x01, getSamplePrescaler },
    { ID_SET_SAMPLEPOSTS, 0x02, setSamplePostScaler },
    { ID_GET_SAMPLEPOSTS, 0x01, getSamplePostScaler },
    { ID_LASTSAMPLE, 0x01, getLastSample },
    { ID_LASTSAMPLE_HRES, 0x01, getLastSampleHRes },
    { ID_LOADPRESET, 0x01, loadPreset },
    { ID_SAVEPRESET, 0x02, savePreset },
};

#define APPCOMAPI_INVALID_ID  (sizeof(validCommands)/sizeof(appcommapi_commandinfo))

appcomapi_data appComApiData;

void APP_CommApi_Init() {
    appComApiData.currentCmdOffset = APPCOMAPI_INVALID_ID;
    appComApiData.pBuffer = NULL;
}

bool APP_CommApi_ProcessBuffer(uint8_t* buffer, uint8_t length) {
    
    const appcommapi_commandinfo* cmdInfo = validCommands;
    
    appComApiData.pBuffer = buffer;
    appComApiData.currentCmdOffset = APPCOMAPI_INVALID_ID;
    
    // In the 1st position we expect the command ID, validate it    
    for (uint8_t n = 0; n < APPCOMAPI_INVALID_ID; n++) {
        
        NVMCON1bits.NVMREG = 2;
        if ((*buffer) == cmdInfo->commandId) {
            appComApiData.currentCmdOffset = n;
            break;
        }
        cmdInfo++;
    }
    
    // Execute the action
    bool valid = appComApiData.currentCmdOffset != APPCOMAPI_INVALID_ID;
    NVMCON1bits.NVMREG = 2;
    fpointer handler = cmdInfo->handler;
    uint8_t minLength = (cmdInfo->parNum << 1) + 1;
    valid = valid && (handler != NULL) && (minLength <= length);
    if (valid) {
        valid = (handler)();
    }
    
    // Send back an error if required
    if (!valid) {
        renderKOAnswer();
    }
    
    // Signal an invalid/fault condition to the caller
    return valid;
}


static void renderOKAnswer(uint8_t channel) {
    
    NVMCON1bits.NVMREG = 2;
    *appComApiData.pBuffer++ = validCommands[appComApiData.currentCmdOffset].commandId;
    writeValue(channel);
}

static void renderKOAnswer() {
    
    *appComApiData.pBuffer++ = (uint8_t)ID_ERROR;
    APP_SensorBusCommitBuffer(appComApiData.pBuffer);
}

#define HEXTONIBBLE(a) (((a) <= '9')?((a)-'0'):(((a)-'A') + 0x0A))
static uint8_t getParameter(uint8_t parNum) {
    
    uint8_t* data = appComApiData.pBuffer + 1 + (parNum<<1);
    uint8_t result = HEXTONIBBLE(*data) << 4;
    data++;
    result |= HEXTONIBBLE(*data);
        
    return result;
}

#define NIBBLEBINTOHEX(a) ((a)>0x09)?(((a)-0x0A)+'A'):((a)+'0');
static void writeValue(uint8_t value) {
    
    *appComApiData.pBuffer++ = NIBBLEBINTOHEX(((value>>4) & 0x0F));
    *appComApiData.pBuffer++ = NIBBLEBINTOHEX((value & 0x0F));
}

static void writeShortValue(uint16_t value) {
    writeValue((uint8_t)((value>>8)&0xFF));
    writeValue((uint8_t)(value&0xFF));
}

static void writeLongValue(uint32_t value) {
    writeValue((uint8_t)((value>>24)&0xFF));
    writeValue((uint8_t)((value>>16)&0xFF));
    writeValue((uint8_t)((value>>8)&0xFF));
    writeValue((uint8_t)(value&0xFF));
}

// We suppose here to have 32bit wide floats
static void writeFloatValue(float value) {
    uint32_t iValue = *((uint32_t*)&value);
    writeLongValue(iValue);
}

static void writeString(uint8_t* value) {
    
    do {
        writeValue(*value++);
    }while (*value != NULL);
}

static bool echo(void) {
    
    NVMCON1bits.NVMREG = 2;
    *appComApiData.pBuffer++ = validCommands[appComApiData.currentCmdOffset].commandId;
    APP_SensorBusCommitBuffer(appComApiData.pBuffer);
    
    return true;
}

static bool sensorInquiry(void) {
    
    uint8_t buffer[MAX_INQUIRY_BUFLENGTH];
    uint8_t channel = getParameter(0);
    
    if (APP_SAR_InquirySensor(channel, buffer, MAX_INQUIRY_BUFLENGTH)) {
        buffer[MAX_INQUIRY_BUFLENGTH-1] = '\0';
        renderOKAnswer(channel);
        writeString(buffer);
        APP_SensorBusCommitBuffer(appComApiData.pBuffer);
        
        return true;
    }
    
    return false;
}

static bool sampleEnable(void) {
    if (APP_SAR_EnableSampling(true)) {
        return echo();
    }
    return false;
}

static bool sampleDisable(void) {
    if (APP_SAR_EnableSampling(false)) {
        return echo();
    }
    return false;
}

static bool setSamplePrescaler(void) {
    
    uint8_t channel = getParameter(0);
    uint8_t prescaler = getParameter(1);
    if (APP_SAR_SetSamplePrescaler(channel, prescaler)) {
        renderOKAnswer(channel);
        APP_SensorBusCommitBuffer(appComApiData.pBuffer);
        return true;
    }
    
    return false;
}

static bool getSamplePrescaler(void) {
    uint8_t channel = getParameter(0);
    uint8_t prescaler;
    if (APP_SAR_GetSamplePrescaler(channel, &prescaler)) {
        renderOKAnswer(channel);
        writeValue(prescaler);
        APP_SensorBusCommitBuffer(appComApiData.pBuffer);
        return true;
    }
    
    return false;
}

static bool setSamplePostScaler(void) {
    uint8_t channel = getParameter(0);
    uint8_t postscaler = getParameter(1);
    if (APP_SAR_SetSamplePostscaler(channel, postscaler)) {
        renderOKAnswer(getParameter(0));    // This should be "channel" but seems we have a problem with compiler optimizations when turned on
        APP_SensorBusCommitBuffer(appComApiData.pBuffer);
        return true;
    }
    return false;
}

static bool getSamplePostScaler(void) {
    uint8_t channel = getParameter(0);
    uint8_t postscaler;
    if (APP_SAR_GetSamplePostscaler(channel, &postscaler)) {
        renderOKAnswer(channel);
        writeValue(postscaler);
        APP_SensorBusCommitBuffer(appComApiData.pBuffer);
        return true;
    }
    
    return false;    
}

static bool getLastSample(void) {
    
    float lastSampleHiRes = 0;
    uint32_t lastTimestamp = 0;
    uint8_t channel = getParameter(0);
    if (APP_SAR_GetLastSample(channel, &lastSampleHiRes, &lastTimestamp)) {
        renderOKAnswer(channel);
        uint16_t lastSample = (uint16_t) lastSampleHiRes;
        writeShortValue(lastSample);
        writeLongValue(lastTimestamp);
        APP_SensorBusCommitBuffer(appComApiData.pBuffer);
        return true;
    }
    
    return false;
}

static bool getLastSampleHRes(void) {
    
    float lastSample = 0;
    uint32_t lastTimestamp = 0;
    uint8_t channel = getParameter(0);
    if (APP_SAR_GetLastSample(channel, &lastSample, &lastTimestamp)) {
        renderOKAnswer(channel);
        writeFloatValue(lastSample);
        writeLongValue(lastTimestamp);
        APP_SensorBusCommitBuffer(appComApiData.pBuffer);
        return true;
    }
    
    return false;
}

static bool loadPreset(void) {
    
    uint8_t channel = getParameter(0);
    if (APP_SAR_LoadPreset(channel)) {
        renderOKAnswer(channel);
        APP_SensorBusCommitBuffer(appComApiData.pBuffer);
        return true;
    }
    
    return false;
}

static bool savePreset(void) {
    
    uint8_t channel = getParameter(0);
    if (APP_SAR_SavePreset(channel)) {
        renderOKAnswer(channel);
        APP_SensorBusCommitBuffer(appComApiData.pBuffer);
    }
    
    return false;
}


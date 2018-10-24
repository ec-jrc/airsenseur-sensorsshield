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

#include <pic18f26k40.h>

#include "mcc_generated_files/mcc.h"
#include "appsensorbus.h"
#include "appsensorarray.h"
#include "appsampler.h"
#include "appsampleropcn2.h"
#include "appsamplervz89.h"
#include "appaverager.h"
#include "apppersister.h"
#include "drvsystimer.h"
#include "drvopcn2.h"
#include "drvvz89.h"

#define EMPTY_STEP              (functionP)0x00
#define MAX_RECIPE_STEPS        0x07

#define AVERAGER_FOR_PM1        HISTOGRAM_BINS_NUM
#define AVERAGER_FOR_PM25       (AVERAGER_FOR_PM1+1)
#define AVERAGER_FOR_PM10       (AVERAGER_FOR_PM25+1)
#define AVERAGER_FOR_TEMP       (AVERAGER_FOR_PM10+1)
#define AVERAGER_FOR_VOLUME     (AVERAGER_FOR_TEMP+1)
#define AVERAGER_FOR_MOX        (AVERAGER_FOR_VOLUME+1)

#define PHYSICAL_SENSOR_OPC     0x00
#define PHYSICAL_SENSOR_MOX     0x01
#define NUM_PHYSICAL_SENSORS    0x02

#define SENSOR_SEPARATION_CH    0x15
#define NUM_LOGICAL_CHANNELS    0x16
#define SENSORNAME_SIZE         0x10

#define MAX_AVERAGER_ILOOP_DEEP     10
#define MAX_AVERAGER_OLOOP_DEEP     6

#define GET_PHYSICAL_SENSOR(x) (((x) < SENSOR_SEPARATION_CH)? 0x00 : 0x01)
#define POSTSCALERFACTORS_LISTSIZE  (sizeof(postScalerFactors)/2)

typedef enum appsar_commands {
    IDLE,
    START_SAMPLING,
    STOP_SAMPLING,
    LAST_INVALID,
} appsar_commands;

typedef uint8_t (*functionP)(void);

typedef struct appsar_recipe_ {
    appsar_commands command;
    functionP steps[MAX_RECIPE_STEPS];
} appsar_recipe;

// Internal function prototypes
static uint8_t powerOn(void);
static uint8_t powerOff(void);
static uint8_t initSensors(void);
static uint8_t opcFanOn(void);
static uint8_t opcLaserOn(void);
static uint8_t opcLaserOff(void);
static uint8_t opcFanOff(void);
static uint8_t opcFanAndLaserOn(void);
static uint8_t opcFanAndLaserOff(void);
static uint8_t waitTime(void);
static uint8_t waitStartupTime(void);
static uint8_t startSampling(void);
static uint8_t stopSampling(void);

static bool applyAveragers(uint8_t channel, float sample, uint32_t timestamp);
static uint8_t getPostscalerFactors(uint8_t required);
static void checkAndCorrectPostscalerFactors(app_averagerdata* averagerData);

const appsar_recipe recipeList[] = {
    { IDLE, EMPTY_STEP, EMPTY_STEP, EMPTY_STEP, EMPTY_STEP, EMPTY_STEP, EMPTY_STEP, EMPTY_STEP },
    { START_SAMPLING, powerOn, waitStartupTime, initSensors, waitTime, opcFanAndLaserOn, waitTime, startSampling },
    { STOP_SAMPLING, stopSampling, waitTime, opcFanAndLaserOff, waitTime, powerOff, EMPTY_STEP, EMPTY_STEP },
    { LAST_INVALID, EMPTY_STEP, EMPTY_STEP, EMPTY_STEP, EMPTY_STEP, EMPTY_STEP, EMPTY_STEP, EMPTY_STEP }
};

const uint8_t postScalerFactors[][2] = {
    { 0x00, 0x00 }, { 0x01, 0x00 }, { 0x02, 0x20 }, { 0x03, 0x30 }, { 0x04, 0x40 }, 
    { 0x05, 0x50 }, { 0x06, 0x60 }, { 0x07, 0x70 }, { 0x08, 0x80 }, { 0x09, 0x90 },
    { 0x0A, 0x52 }, { 0x0C, 0x62 }, { 0x0E, 0x72 }, { 0x0F, 0x53 }, { 0x10, 0x82 }, 
    { 0x12, 0x92 }, { 0x14, 0x54 }, { 0x15, 0x73 }, { 0x18, 0x83 }, { 0x19, 0x55 }, 
    { 0x1B, 0x93 }, { 0x1C, 0x74 }, { 0x1E, 0x65 }, { 0x20, 0x84 }, { 0x23, 0x75 }, 
    { 0x24, 0x94 }, { 0x28, 0x85 }, { 0x2D, 0x95 },
};

const char sensorNames[NUM_LOGICAL_CHANNELS][SENSORNAME_SIZE] = {
    "OPCN2Bin0", "OPCN2Bin1", "OPCN2Bin2", "OPCN2Bin3", "OPCN2Bin4", "OPCN2Bin5", "OPCN2Bin6", 
    "OPCN2Bin7", "OPCN2Bin8", "OPCN2Bin9", "OPCN2Bin10", "OPCN2Bin11", "OPCN2Bin12", "OPCN2Bin13", 
    "OPCN2Bin14", "OPCN2Bin15", "OPCN2PM1", "OPCN2PM25", "OPCN2PM10", "OPCN2Temp", "OPCN2Vol", "MOX",
};

typedef struct appsar_data_ {
    
    // Data needed to handle slow operations through state machine
    appsar_commands currentCommand;
    uint8_t currentStep;
    uint32_t stepTimestamp;
    
    // Data associated to samplers
    app_samplerdata samplers[NUM_PHYSICAL_SENSORS];
    
    // Data associated to averagers.
    // Two banks of averagers are connected in series to optimize
    // RAM footprint
    app_averagerdata avgInnerLoop[NUM_LOGICAL_CHANNELS];
    app_averagerdata avgOuterLoop[NUM_LOGICAL_CHANNELS];    
    float avgBufferInnerLoop[NUM_LOGICAL_CHANNELS][MAX_AVERAGER_ILOOP_DEEP];
    float avgBufferOuterLoop[NUM_LOGICAL_CHANNELS][MAX_AVERAGER_OLOOP_DEEP];
    
    // Miscellaneous data
    bool samplingEnabled;
    uint32_t startSamplingTimestamp;
    
} appsar_data;

appsar_data appSarData;

void APP_SAR_Init(void) {
    
    // Power off sensors
    powerOff();

    // Initialize general variables
    appSarData.currentCommand = IDLE;
    appSarData.currentStep = 0;
    appSarData.stepTimestamp = DRV_SysTimerGetTime();
    appSarData.startSamplingTimestamp = DRV_SysTimerGetTime();
    appSarData.samplingEnabled = false;
    
    // Initialize samplers
    APP_Sam_Init(appSarData.samplers);
    APP_Sam_SetTaskFunction(appSarData.samplers, APP_Sam_OPCN2_TaskFunction);
    
    APP_Sam_Init(appSarData.samplers+1);
    APP_Sam_SetTaskFunction(appSarData.samplers+1, APP_Sam_VZ89_TaskFunction);
    
    // Initialize Averagers
    for (uint8_t n = 0; n < NUM_LOGICAL_CHANNELS; n++) {
        APP_Avg_Init(appSarData.avgInnerLoop+n, appSarData.avgBufferInnerLoop[n], MAX_AVERAGER_ILOOP_DEEP);
        APP_Avg_Init(appSarData.avgOuterLoop+n, appSarData.avgBufferOuterLoop[n], MAX_AVERAGER_OLOOP_DEEP);
    }
    
    // Load default preset
    for (uint8_t n = 0; n < NUM_LOGICAL_CHANNELS; n++) {
        APP_SAR_LoadPreset(n);
    }
    
    // Check averager's deep
    checkAndCorrectPostscalerFactors(appSarData.avgInnerLoop);
    checkAndCorrectPostscalerFactors(appSarData.avgOuterLoop);
}

void APP_SAR_Task(void) {
    
    // Evaluate samplers if needed
    if (appSarData.samplingEnabled) {
        
        app_samplerdata* samplerData = appSarData.samplers;

        // Check if new histogram data available from OPC sensor
        // and inject each bin into a specific averager
        if (APP_Sam_Task(samplerData)) {

            bool bSampleAvailable = false;            
            opcn2_histogram_data* histData = APP_Sam_OPCN2_GetCustomData();
            if (histData != NULL) {
                
                // Samples timestamps will be relative to the beginning of the sampling campaign
                uint32_t sampleTimeStamp = samplerData->latchedSampleTimeStamp - appSarData.startSamplingTimestamp;
                
                // Bins
                float airVolume = histData->sampleFlowRate * histData->samplingPeriod;
                for (uint8_t n = 0; n < HISTOGRAM_BINS_NUM; n++) {
                    
                    float particles = ((float)histData->bins[n]) / airVolume;
                    bSampleAvailable |= applyAveragers(n, particles, sampleTimeStamp);
                }
                
                // Temperature
                bSampleAvailable |= applyAveragers(AVERAGER_FOR_TEMP, histData->temperaturePressure & 0xFFFF, sampleTimeStamp);
                
                // Volume
                bSampleAvailable |= applyAveragers(AVERAGER_FOR_VOLUME, airVolume, sampleTimeStamp);
                
                // PMs
                bSampleAvailable |= applyAveragers(AVERAGER_FOR_PM1, histData->pm.pm1, sampleTimeStamp);
                bSampleAvailable |= applyAveragers(AVERAGER_FOR_PM25, histData->pm.pm25, sampleTimeStamp);
                bSampleAvailable |= applyAveragers(AVERAGER_FOR_PM10, histData->pm.pm10, sampleTimeStamp);
                
                if (bSampleAvailable) {
                    TX_LED_Toggle();
                }
            }
        }
        
        // Check if new data is available from MOX sensor
        samplerData++;
        if (APP_Sam_Task(samplerData)) {
            
            bool bSampleAvailable = false;
            vz89_data* vz89Data = APP_Sam_VZ89_GetCustomData();
            if (vz89Data != NULL) {
                
                // Samples timestamps will be relative to the beginning of the sampling campaign
                uint32_t sampleTimeStamp = samplerData->latchedSampleTimeStamp - appSarData.startSamplingTimestamp;
                
                bSampleAvailable = applyAveragers(AVERAGER_FOR_MOX, vz89Data->rawSensorValue, sampleTimeStamp);

                if (bSampleAvailable) {
                    RX_LED_Toggle();
                }
            }
        }
    }
    
    // Handle slow run commands
    if (appSarData.currentCommand > LAST_INVALID) {
        appSarData.currentCommand = IDLE;
    }
    
    // Nothing to do if in IDLE mode
    if (appSarData.currentCommand == IDLE) {
        return;
    }
    
    // When reading flash stored data we need to explicitly enable Table Read. 
    // I don't know if this is for a bug in the compiler... 
    NVMCON1bits.NVMREG = 2;
    const appsar_recipe* recipe = recipeList + (uint16_t)appSarData.currentCommand;
    functionP stepFunction = recipe->steps[appSarData.currentStep];
    if (stepFunction != NULL) {
        if (stepFunction()) {
            appSarData.stepTimestamp = DRV_SysTimerGetTime();            
            appSarData.currentStep++;
            
            if (appSarData.currentStep == MAX_RECIPE_STEPS) {
                appSarData.currentStep = 0;
                appSarData.currentCommand = IDLE;
            }    
        }
    }
}

void APP_SAR_Tick(void) {
    
    // Propagate ticker to the sensor
    APP_Sam_Tick(appSarData.samplers);
    APP_Sam_Tick(appSarData.samplers+1);
}

bool APP_SAR_SetSamplePrescaler(uint8_t channel, uint8_t prescaler) {
    if (channel >= NUM_LOGICAL_CHANNELS) {
        return false;
    }
    
    // MOX sensors minimum sampling period is 1s (see VZ89 datasheet)
    uint8_t physicalSensor = GET_PHYSICAL_SENSOR(channel);
    if ((physicalSensor == PHYSICAL_SENSOR_MOX) && (prescaler < 10)) {
        prescaler = 10;
    }
    
    APP_Sam_SetPrescaler(appSarData.samplers+physicalSensor, prescaler);
    return true;
}

bool APP_SAR_GetSamplePrescaler(uint8_t channel, uint8_t* prescaler) {
    if (channel >= NUM_LOGICAL_CHANNELS) {
        *prescaler = 0;
        return false;
    }
    
    *prescaler = APP_Sam_GetPrescaler(appSarData.samplers+GET_PHYSICAL_SENSOR(channel));
    return true;
}

bool APP_SAR_SetSamplePostscaler(uint8_t channel, uint8_t postscaler){
    if (channel >= NUM_LOGICAL_CHANNELS) {
        return false;
    }
    
    // Calculate deeps for averagers
    uint8_t postScalerListRow = getPostscalerFactors(postscaler);
    NVMCON1bits.NVMREG = 2;
    uint8_t innerPostScaler = (postScalerFactors[postScalerListRow][1]>>4) & 0x0F;
    uint8_t outerPostScaler = postScalerFactors[postScalerListRow][1] & 0x0F;
    
    if (!APP_Avg_SetBufferDeep(appSarData.avgOuterLoop+channel, outerPostScaler)) {
        return false;
    }
    
    return APP_Avg_SetBufferDeep(appSarData.avgInnerLoop+channel, innerPostScaler);
}

bool APP_SAR_GetSamplePostscaler(uint8_t channel, uint8_t* postscaler) {
    if (channel >= NUM_LOGICAL_CHANNELS) {
        *postscaler = 0;
        return false;
    }
    
    uint8_t innerPostScaler = APP_Avg_GetBufferDeep(appSarData.avgInnerLoop+channel);
    uint8_t outerPostScaler = APP_Avg_GetBufferDeep(appSarData.avgOuterLoop+channel);
    
    // Flat averagers counts 1 in the product
    if (innerPostScaler == 0) { innerPostScaler++; }
    if (outerPostScaler == 0) { outerPostScaler++; }
    
    *postscaler = outerPostScaler*innerPostScaler;
    
    return true;
}

bool APP_SAR_EnableSampling(bool enable) {

    // Start sampling operations only if not already in sampling mode
    if (!appSarData.samplingEnabled && enable) {
        
        // Check averager's deep
        checkAndCorrectPostscalerFactors(appSarData.avgInnerLoop);
        checkAndCorrectPostscalerFactors(appSarData.avgOuterLoop);

        // Reset averagers
        for (uint8_t channel = 0; channel < NUM_LOGICAL_CHANNELS; channel++) {
            APP_Avg_SetBufferDeep(appSarData.avgInnerLoop+channel, APP_Avg_GetBufferDeep(appSarData.avgInnerLoop+channel));
            APP_Avg_SetBufferDeep(appSarData.avgOuterLoop+channel, APP_Avg_GetBufferDeep(appSarData.avgOuterLoop+channel));
        }
        
        appSarData.currentStep = 0;
        appSarData.currentCommand = START_SAMPLING;

    } else if (!enable) {
        appSarData.currentStep = 0;
        appSarData.currentCommand = STOP_SAMPLING;
    }
    
    return true;
}

bool APP_SAR_InquirySensor(uint8_t channel, uint8_t* buffer, uint8_t bufSize) {
    
    if (channel >= NUM_LOGICAL_CHANNELS) {
        *buffer = '\0';
        return false;
    }
    
    NVMCON1bits.NVMREG = 2;
    for (uint8_t n = 0; n < bufSize; n++) {
        buffer[n] = sensorNames[channel][n];
        if (sensorNames[channel][n] == '\0') {
            break;
        }
    }
    
    return true;
}

bool APP_SAR_GetLastSample(uint8_t channel, float *lastSample, uint32_t *lastTimestamp) {
    if (channel >= NUM_LOGICAL_CHANNELS) {
        *lastSample = 0;
        *lastTimestamp = 0;
        return false;
    }

    // Retrieve last averaged sample value
    if (channel != AVERAGER_FOR_VOLUME) {
        *lastSample = APP_Avg_LastAveragedValue(appSarData.avgOuterLoop+channel);
    } else {
        *lastSample = APP_Avg_LastCumulatedValue(appSarData.avgOuterLoop+channel);
    }
    
    // Clamp to zero for non temperature samples. 
    // Values less than zero can be result of averaging approximations 
    // due to float finite number representation
    if ((channel != AVERAGER_FOR_TEMP) && ((*lastSample) < 0.0f)) {
        *lastSample = 0.0f;
    }
    
    // Get last timestamp
    *lastTimestamp = APP_Avg_LastTimeStamp(appSarData.avgOuterLoop+channel);
    
    return true;
}

bool APP_SAR_SavePreset(uint8_t channel) {
    if (channel >= NUM_LOGICAL_CHANNELS) {
        return false;
    }
    
    uint8_t physicalSensor = GET_PHYSICAL_SENSOR(channel);
    
    // Save prescaler for this logical channel
    APP_Persister_Save8bit(BASEADDRESS_SAMPLERS+physicalSensor,
                            APP_Sam_GetPrescaler(appSarData.samplers+physicalSensor));
    
    // Saver inner and outer averagers deep for this logical channel
    APP_Persister_Save8bit(BASEADDRESS_AVERAGERS_INNER+channel, 
                            APP_Avg_GetBufferDeep(appSarData.avgInnerLoop+channel));
    
    APP_Persister_Save8bit(BASEADDRESS_AVERAGERS_OUTER+channel, 
                            APP_Avg_GetBufferDeep(appSarData.avgOuterLoop+channel));
    
    return true;
}

bool APP_SAR_LoadPreset(uint8_t channel) {
    if (channel >= NUM_LOGICAL_CHANNELS) {
        return false;
    }
    
    // Load prescalers for this logical channel
    uint8_t physicalSensor = GET_PHYSICAL_SENSOR(channel);
    uint8_t prescaler = APP_Persister_Load8bit(BASEADDRESS_SAMPLERS+physicalSensor);
    if (prescaler == 0xFF) {
        prescaler = 0x20;
    }
    APP_Sam_SetPrescaler(appSarData.samplers+physicalSensor, prescaler);
    
    // Load inner and outer averagers deep for this logical channel
    uint8_t postscaler = APP_Persister_Load8bit(BASEADDRESS_AVERAGERS_INNER+channel);
    APP_Avg_SetBufferDeep(appSarData.avgInnerLoop+channel, postscaler);
    
    uint8_t postscaler = APP_Persister_Load8bit(BASEADDRESS_AVERAGERS_OUTER+channel);
    APP_Avg_SetBufferDeep(appSarData.avgOuterLoop+channel, postscaler);
    
    return true;
}

static uint8_t powerOn(void) {
    
    // Force LEDs blackout to reduce overall inrush current
    APP_SensorBusBlackOut(true);
    
    // Turn on power supply
    OPC_SHDN_SetHigh();
        
    // Setup SPI pins
    OPC_SS_SetDigitalOutput();
    RC0_SetDigitalOutput();
    RC1_SetDigitalInput();
    RC2_SetDigitalOutput();
    SPI1_Initialize();
    
    // Setup I2C pins
    I2CInit();
            
    return 0x01;
}


static uint8_t powerOff(void) {
    
    // Turn off SPI pins
    OPC_SS_SetDigitalInput();
    OPC_SS_ResetPullup();
    RC0_SetDigitalInput();
    RC0_ResetPullup();
    RC1_SetDigitalInput();
    RC1_ResetPullup();
    RC2_SetDigitalInput();
    RC2_ResetPullup();
    
    // Turn off I2C pins
//    IO_SDA2_SetDigitalOutput(); 
//    IO_SCL2_SetDigitalOutput(); 
//    IO_SDA2_SetLow(); 
//    IO_SCL2_SetLow();
    
    // Turn off power supply
    OPC_SHDN_SetLow();
    
    return 0x01;
}

static uint8_t initSensors(void) {

    // Initialize OPC and VZ89
    DRV_OPCN2Init();
    DRV_VZ89Init();

    return 0x01;
}

static uint8_t opcFanOn(void) {
    
    if (!DRV_OPCN2_Ready()) {
        return 0x00;
    }
    
    DRV_OPCN2_SetFan(0x01);
    return 0x01;
}

static uint8_t opcFanAndLaserOn(void) {
    
    if (!DRV_OPCN2_Ready()) {
        return 0x00;
    }

    DRV_OPCN2_SetFanAndLaser(0x01);
    return 0x01;
}

static uint8_t opcFanAndLaserOff(void) {
    if (!DRV_OPCN2_Ready()) {
        return 0x00;
    }

    DRV_OPCN2_SetFanAndLaser(0x00);
    return 0x01;
}

static uint8_t opcLaserOn(void) {
    if (!DRV_OPCN2_Ready()) {
        return 0x00;
    }
    DRV_OPCN2_SetLaser(0x01);
    return 0x01;
}

static uint8_t opcLaserOff(void) {
    if (!DRV_OPCN2_Ready()) {
        return 0x00;
    }
    DRV_OPCN2_SetLaser(0x00);
    return 0x01;
}

static uint8_t opcFanOff(void) {
    if (!DRV_OPCN2_Ready()) {
        return 0x00;
    }
    DRV_OPCN2_SetFan(0x00);
    return 0x01;
}

static uint8_t waitTime(void) {
    if ((DRV_SysTimerGetTime() - appSarData.stepTimestamp) < (uint32_t)50) { // Half a second
        return 0x00;
    }
    
    return 0x01;
}

static uint8_t waitStartupTime() {
    if ((DRV_SysTimerGetTime() - appSarData.stepTimestamp) < (uint32_t)500) { // 5 seconds
        return 0x00;
    }
    
    return 0x01;
}

static uint8_t startSampling() {
    appSarData.samplingEnabled = true;
    appSarData.startSamplingTimestamp = DRV_SysTimerGetTime();
    
    // Remove LEDs blackout
    APP_SensorBusBlackOut(false);
    
    return 0x01;
}

static uint8_t stopSampling() {
    appSarData.samplingEnabled = false;
    return 0x01;
}

static bool applyAveragers(uint8_t channel, float sample, uint32_t timestamp) {
    
    if (APP_Avg_CollectSample(appSarData.avgInnerLoop+channel, sample, timestamp)) {
        float innerSample;
        if (channel != AVERAGER_FOR_VOLUME) {
            innerSample = APP_Avg_LastAveragedValue(appSarData.avgInnerLoop+channel);
        } else {
            innerSample = APP_Avg_LastCumulatedValue(appSarData.avgInnerLoop+channel);
        }
        
        return APP_Avg_CollectSample(appSarData.avgOuterLoop+channel, innerSample, APP_Avg_LastTimeStamp(appSarData.avgInnerLoop+channel));
    };
    
    return false;
}

static uint8_t getPostscalerFactors(uint8_t required) {
    
    uint8_t foundOffset = POSTSCALERFACTORS_LISTSIZE;
    for (uint8_t n = 0; n < POSTSCALERFACTORS_LISTSIZE; n++) {
        NVMCON1bits.NVMREG = 2;
        if (postScalerFactors[n][0] > required) {
            foundOffset = n-1;
            break;
        }
        if (required == postScalerFactors[n][0]) {
            foundOffset = n;
            break;
        }
    }
    
    if (foundOffset >= POSTSCALERFACTORS_LISTSIZE) {
        foundOffset = POSTSCALERFACTORS_LISTSIZE-1;
    }
    
    return foundOffset;
};

// Postscaler factors should be equal for all channels associated
// to the OPC sensor. They are defined in each averager for speed and implementation 
// reasons but it's important to maintain identical.
// This is the reason why they're checked at startup and each time a sampling is started
static void checkAndCorrectPostscalerFactors(app_averagerdata* averagerData) {
    
    bool correct = false;
    uint8_t bufferDeep;
    
    // Check all averagers
    for(uint8_t channel = 0; channel < SENSOR_SEPARATION_CH-1; channel++) {
        if (APP_Avg_GetBufferDeep(averagerData+channel) != APP_Avg_GetBufferDeep(averagerData+channel+1)) {

            correct = true;
            bufferDeep = APP_Avg_GetBufferDeep(averagerData+channel);
            break;
        }
    }
    
    // Correct if needed
    if (correct) {
        for(uint8_t channel = 0; channel < SENSOR_SEPARATION_CH; channel++) {
            APP_Avg_SetBufferDeep(averagerData+channel, bufferDeep);
        }
    }
}
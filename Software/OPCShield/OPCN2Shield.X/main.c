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
#include "drvvz89.h"
#include "drvsystimer.h"
#include "appsensorarray.h"
#include "appsensorbus.h"

// This is called once 10 milliseconds
void TMR0_InterruptHandlerImpl(void) {
    DRV_SysTimerTick();
    APP_SensorBusTimerTick();
    APP_SAR_Tick();
};

uint8_t GetLocalAddress(void) {
    
    return  (ADDR1_GetValue()? 0:1) | 
            (ADDR2_GetValue()? 0:2) | 
            (ADDR4_GetValue()? 0:4) | 
            (ADDR8_GetValue()? 0:8);
};

void main(void)
{
    // Initialize the device
    SYSTEM_Initialize();
    I2CInit();    
    
    // Initialize drivers and application layers
    DRV_OPCN2Init();
    DRV_VZ89Init();    
    DRV_SysTimerInit();
    APP_SensorBusInit(GetLocalAddress());
    APP_SAR_Init();
        
    // Setup the timer tick interrupt handler
    TMR0_SetInterruptHandler(TMR0_InterruptHandlerImpl);

    // Enable high priority global interrupts
    INTERRUPT_GlobalInterruptHighEnable();

    // Enable low priority global interrupts.
    INTERRUPT_GlobalInterruptLowEnable();

    while (1)
    {
        DRV_OPCN2Task();
        DRV_VZ89Task();
        APP_SAR_Task();
        APP_SensorBusRxHandler();
        APP_SensorBusTxHandler();
        
        if (!USER_B_GetValue()) {
            APP_SensorBusEnableLEDs();
        }
    }
}

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

void TMR0_InterruptHandlerImpl(void) {
    APP_SensorBusTimerTick();
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
    
    // Initialize application layers
    APP_SensorBusInit(GetLocalAddress());
    
    // Setup the timer tick interrupt handler
    TMR0_SetInterruptHandler(TMR0_InterruptHandlerImpl);    

    // Enable high priority global interrupts
    INTERRUPT_GlobalInterruptHighEnable();

    // Enable low priority global interrupts.
    INTERRUPT_GlobalInterruptLowEnable();

    // This loop would never end
    while (1)
    {
        APP_SensorBusRxHandler();
        APP_SensorBusTxHandler();
        
        // Enable LEDs if required by user pushbutton
        if (!USERB_GetValue()) {
            APP_SensorBusEnableLEDs();
        }
    }
}

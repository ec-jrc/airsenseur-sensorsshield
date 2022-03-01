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

#include "CommProtocol.h"
#include "EEPROMHelper.h"
#include "ExpShieldTwoBoardImpl.h"
#include "GlobalHalHandlers.h"
#include "GPIOHelper.h"
#include "LEDsHelper.h"
#include "SensorBusWrapper.h"
#include "SerialAHelper.h"
#include "SerialBHelper.h"
#include "SerialCHelper.h"
#include "SerialDHelper.h"
#include "SerialUSBHelper.h"
#include "IntADDevice.h"
#include "PWMHelper.h"
#include "CCDriveEngine.h"
#include "TControlEngine.h"
#include "ADT7470Device.h"

#include "../ASInc/Persistence.h"
#include "SensorsArray.h"

SensorsArray* sensorBoard;
CommProtocol* commProtocol;
SensorBusWrapper *sensorBusProtocol;

void timerInterrupt() {

    if (sensorBoard) {
        sensorBoard->timerTick();
    }
    if (commProtocol) {
        commProtocol->timerTick();
    }
    if (sensorBusProtocol) {
    		sensorBusProtocol->timerTick();
    }

    LEDs.tick();
    EEPROM.tick();
    AS_TCONTROL.tick();
}

void uart1Interrupt(unsigned char halfBuffer) {
	SerialA.onDataRx(halfBuffer!=0);
	LEDs.pulse(LEDsHelper::RXDATA);
}

void uart2Interrupt(unsigned char halfBuffer) {
	SerialB.onDataRx(halfBuffer!=0);
	LEDs.pulse(LEDsHelper::RXDATA);
}

void uart3Interrupt(unsigned char halfBuffer) {
	SerialC.onDataRx(halfBuffer!=0);
}

void uart4Interrupt(unsigned char halfBuffer) {
	SerialD.onDataRx(halfBuffer!=0);
}

void uart1Error() {
	SerialA.onErrorCallback();
}

void uart2Error() {
	SerialB.onErrorCallback();
}

void uart3Error() {
	SerialC.onErrorCallback();
}

void uart4Error() {
	SerialD.onErrorCallback();
}


void usbRxCallback(unsigned char* buffer, long bufferLen) {
	((SerialUSBHelper*)SerialUSBHelper::getInstance())->onDataRx(buffer, bufferLen);
	LEDs.pulse(LEDsHelper::RXDATA);
}

void adcCallback() {
	// Propagate to IntADCDevice and update the constant current diver with
	// measured DC current flowing on the Peltier cell
	unsigned short ccCurrentDrive = IntADDevice::getInstance()->onADCScanTerminated();
	AS_CCDRIVER.tick(ccCurrentDrive);
}

void setup_impl() {

    // Wait for external peripheral hardware stabilization
    HAL_Delay(500);

    // Initialize LEDs
	LEDs.init();

    // Initialize the serial lines
    SerialA.init();
    SerialB.init();

    // Instantiate and initialize the main objects
    sensorBoard = new SensorsArray();
    commProtocol = new CommProtocol(sensorBoard);
    sensorBusProtocol = new SensorBusWrapper(commProtocol);
    sensorBusProtocol->init(AS_GPIO.getBoardId());
    commProtocol->setSensorBusWrapper(sensorBusProtocol);

    // Initialize the PWM Helper
    AS_PWM.init();

    // Initialize the tick timer
    HAL_TIM_Base_Start_IT(&htim7);

    // Initialize the A/D converter timer
    HAL_TIM_Base_Start_IT(&htim3);

    // Initialize the sensor board items
    if (!sensorBoard->init()) {
    	LEDs.pulse(LEDsHelper::HEARTBEAT);
    	LEDs.pulse(LEDsHelper::TXDATA);
    	LEDs.pulse(LEDsHelper::RXDATA);
    }

    // Signal we detected the external temperature/humidity flyboard
    if (sensorBoard->getIsFlyboardReady()) {
    	LEDs.pulse(LEDsHelper::HEARTBEAT);
    }

    // Enable current driver and temperature control PIDs
    AS_CCDRIVER.setEnabled(true);
    AS_TCONTROL.setEnabled(true);

    // Signal we're ready
    LEDs.pulse(LEDsHelper::TXDATA);
}

void loop_impl() {

    // Propagate to the sensor array
    bool newSample = sensorBoard->loop();

    // Heartbeat led
    if (newSample) {
        LEDs.pulse(LEDsHelper::HEARTBEAT);
    }

    // Handle the serial line A (PtP protocol)
    if (SerialA.available()) {
        unsigned char val = SerialA.read();
        commProtocol->onDataReceived(val, CommProtocol::SOURCE_SERIAL);
    }

    // Handle the serial line B (SensorBus)
    if (SerialB.available()) {
    		unsigned char val = SerialB.read();
    		sensorBusProtocol->onDataReceived(val);
    }

    // Handle the serial line through USB
    if (SerialUSB.available()) {
    		unsigned char val = SerialUSB.read();
    		commProtocol->onDataReceived(val, CommProtocol::SOURCE_USB);
    }

    // Handle the EEPROM delayed write operations
    EEPROM.mainLoop();

    // Retrieve current internal chamber setpoint and temperature
    short currentChamberSetpoint, currentChamberTemperature;
    AS_ADT7470.getChamberSetpointAndTemperature(currentChamberSetpoint, currentChamberTemperature);

    // Handle the Temperature controller engine
    AS_TCONTROL.setTemperatureSetpoint(currentChamberSetpoint);
    AS_TCONTROL.loop(currentChamberTemperature);

    // Handle the constant current generator engine
    AS_CCDRIVER.loop();

    // Check for user button status
    if (AS_GPIO.digitalRead(USER_BUTTONPIN) == 0) {
    		LEDs.enable(true);
    }
}


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


#include "TControlEngine.h"
#include "PWMHelper.h"
#include "CCDriveEngine.h"
#include "ADT7470Device.h"

#define TCONTROL_ENGINE_PERIOD				99		/* Once a second */
#define EXT_FAN_BLANK_TIMEOUT				60		/* 60 seconds of consecutive heating */
#define OVERHEAT_BLANK_TIMEOUT				60		/* 60 seconds of consecutive overheat conditions */
#define MAX_EXT_FAN_NOT_ROTATING_PERIOD		120 	/* 120 seconds of not rotating period before forced cooling shutdown (safety) */

#define MAX_T_INT_C							6000	/* in 1/100C */
#define MAX_T_EXT_H							MAX_T_INT_C + 2000
#define MAX_T_INT_H							MAX_T_INT_C + 3000

TControlEngine TControlEngine::instance;

TControlEngine::TControlEngine() : PIDEngine(), enabled(false), prescaler(TCONTROL_ENGINE_PERIOD/3),
									extFanBlankTimer(EXT_FAN_BLANK_TIMEOUT), overheatBlankTimer(OVERHEAT_BLANK_TIMEOUT), currentDrive(0) {

	// Initial PID coefficients are set as default
	// but may be overridden by external calls
    setCoefficients(0.1, 0.02, 0.0);

	// Output min and max are expressed
	// in duty cycle percentage, in 1/100 units
	// Percentace < 0 are reserved for cooling
	// Percentage > 0 are reserved for heating
	setOutMinMax(-10000, 10000);

	// Heating and cooling processes have a default thresholded value
	// that can be overridden by external calls
	setCoolingStartPercentage(100);	// 1% in 1/100 units
	setHeatingStartPercentage(100);	// 1% in 1/100 units
}

TControlEngine::~TControlEngine() {
}

// Temperature setpoint is expressed in 1/100 C
void TControlEngine::setTemperatureSetpoint(short tSetpoint) {
	setSetPoint(tSetpoint);
}

// Minimum percentage at witch the cooling engine starts running in 1/100 %
void TControlEngine::setCoolingStartPercentage(unsigned short _coolingMin) {
	coolingMin = _coolingMin;
}

// Minimum percentage at witch the heating engine starts running in 1/100 %
void TControlEngine::setHeatingStartPercentage(unsigned short _heatingMin) {
	heatingMin = _heatingMin;
}

// The Temperature Control Engine is responsible for deciding
// if and when the heater, the cooler and the fans have to
// run. It's also responsible for defining the power to be
// applied to each of the actors and to shut down cooling/heating if
// unsafe temperatures are found.
// It runs each second.

// Current temperature is expressed in 1/100 C
void TControlEngine::loop(short currentTemperature) {

	if (enabled) {

		// When enabled:
		// The internal heatsink fan is supposed to run at the set point
		// The internal circulation fan is supposed to run at the set point
		// The external heatsink fan is on when the cooler is on
		if (prescaler > TCONTROL_ENGINE_PERIOD) {
			prescaler = 0;

			// Set the internal fans at the set point
			AS_ADT7470.setInternalFanSpeedAtSetpoint();
			AS_ADT7470.setCirculationFanSpeedAtSetpoint();

			// Calculate the next drive value
			currentDrive = (short int) getNextDriveValue(currentTemperature);

			// A drive value greater than zero identifies heating requests.
			bool heating = (currentDrive >= 0);

			// Check if we don't have some unsafe conditions, like temperatures too high
			// internally and/or externally
			short intCT = AS_ADT7470.getTemperatureForChannel(ADT7470_CHANNEL_T_INT_CHAMBER);
			short extHT = AS_ADT7470.getTemperatureForChannel(ADT7470_CHANNEL_T_EXT_HEATSINK);
			short intHT = AS_ADT7470.getTemperatureForChannel(ADT7470_CHANNEL_T_INT_HEATSINK);
			bool overheat = (intCT > MAX_T_INT_C) || (extHT > MAX_T_EXT_H) || (intHT > MAX_T_INT_H);
			if (overheat) {
				overheatBlankTimer = OVERHEAT_BLANK_TIMEOUT;
			} else {
				if (overheatBlankTimer != 0) {
					overheatBlankTimer--;
					overheat = true;
				}
			}

			// When heating,
			// the external fan is supposed to stop (after a grace period)
			if (heating) {

				// Shut down the constant current engine
				AS_CCDRIVER.setCurrentSetPoint(0);

				// Drive the heaters appropriately, when request is over the minimum threshold
				// and not in overheating status
				if (!overheat && (currentDrive >= heatingMin)) {

					AS_PWM.setHeater1DutyCycle(currentDrive);
					AS_PWM.setHeater2DutyCycle(currentDrive);

				} else {

					// Below the minimum threshold. Turn off the heaters.
					AS_PWM.setHeater1DutyCycle(0);
					AS_PWM.setHeater2DutyCycle(0);
				}

			} else {

				// Shut down the heaters
				AS_PWM.setHeater1DutyCycle(0);
				AS_PWM.setHeater2DutyCycle(0);

				// Drive the constant current engine appropriately, if over the minimum threshold
				unsigned short coolingDrive = -currentDrive;
				if (coolingDrive >= coolingMin) {

					// Drive the external fan appropriately
					// The external fan will rotate proportionally to the cooler
					// engine, plus 20% more, clamped by the external set point
					extFanBlankTimer = EXT_FAN_BLANK_TIMEOUT;
					unsigned char fanDrive = coolingDrive/100 + 20;
					unsigned short fanSetpoint = 10000;
					AS_ADT7470.getSetpointForChannel(ADT7470_CHANNEL_F_EXT_HEATSINK, fanSetpoint);
					fanSetpoint = fanSetpoint/100;
					if (fanDrive > fanSetpoint) {
						fanDrive = fanSetpoint;
					}
					AS_ADT7470.setExternalFanSpeed(fanDrive);

					// Drive the cooling appropriately.
					// Avoid to turn on the cooling if in overheating or if the external fan is not properly
					// rotating in the last two minutes. This avoid to self-destroy the
					// peltier cell in case of external fan lock and/or damaged fan
					if (overheat || (AS_ADT7470.getFanLastSeenRotating(ADT7470_CHANNEL_F_EXT_HEATSINK) > MAX_EXT_FAN_NOT_ROTATING_PERIOD)) {
						coolingDrive = 0;
					}

					AS_CCDRIVER.setCurrentSetPoint(coolingDrive);

				} else {

					// Below the minimum threshold. Turn off the cooler.
					AS_CCDRIVER.setCurrentSetPoint(0);
				}
			}

			// Shut down the external fan when the blank timer has expired
			if (extFanBlankTimer > 0) {
				extFanBlankTimer--;
			} else {
				AS_ADT7470.setExternalFanSpeed(0);
			}

		}
	} else {

		// Turn off fans
		AS_ADT7470.setExternalFanSpeed(0);
		AS_ADT7470.setInternalFanSpeed(0);
		AS_ADT7470.setCirculationFanSpeed(0);

		// Turn off heater and cooler
		AS_PWM.setHeater1DutyCycle(0);
		AS_PWM.setHeater2DutyCycle(0);
		AS_CCDRIVER.setCurrentSetPoint(0);
	}
}

void TControlEngine::tick() {
	prescaler++;
}

void TControlEngine::setEnabled(bool on) {
	enabled = on;
}

short TControlEngine::getCurrentDrive() {
	return currentDrive;
}

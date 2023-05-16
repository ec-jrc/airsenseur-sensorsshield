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

#include "CCDriveEngine.h"
#include "PWMHelper.h"

CCDriveEngine CCDriveEngine::instance;

#define CC_MIN_DRIVEVAL	0
#define CC_MAX_DRIVEVAL	10000

CCDriveEngine::CCDriveEngine() : PIDEngine(),
								enabled(false), go(false),
								currentMeasure(0), currentDrive(0) {

	// Set PI(D) tuning coefficients
	AS_CCDRIVER.setCoefficients(0.1, 0.02, 0.0);

	// Output min and max for PID corrections are expressed
	// in duty cycle percentage, in 1/100 units
	setOutMinMax(-CC_MAX_DRIVEVAL, CC_MAX_DRIVEVAL);
}

CCDriveEngine::~CCDriveEngine() {
}

// Current drive setpoint and actual values are expressed in 1/100 %
void CCDriveEngine::setCurrentSetPoint(unsigned short _cSetpoint) {
	setSetPoint(_cSetpoint);

	// When the setpoint is set to 0, is better to turn off the PWM
	// istantaneously instead of delegate to the main loop
	// This is because, during very high transient (more frequently when
	// the user generates a very big temperature setpoint change)
	// it may happens that the TControlEngine starts heating at full power
	// when cooling was still running.
	// This prevents current spikes greater than 7.5A that may blow
	// the protection fuse
	if (_cSetpoint == 0) {
		AS_PWM.setPeltierDutyCycle(0);
	}
}

// Current drive setpoint and actual values are expressed in 1/100 %
void CCDriveEngine::loop() {

	if (enabled) {
		if (go) {
			go = false;

			// Calculate the next value, so the PID coefficients will be updated
			double nextValue = getSetPoint() + getNextDriveValue(currentMeasure);

			// We don't mind (but we need to calculate anyway)
			// about the calculated next value if the setpoint is 0
			if (getSetPoint() == 0) {
				nextValue = 0;
			}

			currentDrive = (nextValue <= CC_MIN_DRIVEVAL)? CC_MIN_DRIVEVAL : ((nextValue > CC_MAX_DRIVEVAL)? CC_MAX_DRIVEVAL : nextValue);

			// Drive the constant current PWM generator
			AS_PWM.setPeltierDutyCycle(currentDrive);
		}
	} else {
		AS_PWM.setPeltierDutyCycle(0);
	}
}

void CCDriveEngine::tick(unsigned short _currentMeasure) {

	// Filter out measurements by averaging them
	currentMeasure = (currentMeasure + _currentMeasure) >> 1;

	go = true;
}

void CCDriveEngine::setEnabled(bool on) {
	enabled = on;
}

unsigned short CCDriveEngine::getCurrentDrive() {
	return currentDrive;
}

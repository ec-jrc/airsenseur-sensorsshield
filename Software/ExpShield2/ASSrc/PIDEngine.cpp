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
 *      European Commission - Joint Research Centre,
 * - Marco Signorini, marco.signorini@liberaintentio.com
 *
 * ===========================================================================
 */

#include "PIDEngine.h"

PIDEngine::PIDEngine() : C{0.0, 0.0, 0.0},
						minVal(0.0), maxVal(0), integralMinVal(0.0),
						integralMaxVal(0.0), setpoint(0.0),
						lastCurrentValue(0.0), integralVal(0.0) {

}

PIDEngine::~PIDEngine() {
}

void PIDEngine::setCoefficients(double p, double i, double d) {
	C.p = p;
	C.i = i;
	C.d = d;
}

double PIDEngine::getP() const {
	return C.p;
}

double PIDEngine::getI() const {
	return C.i;
}

double PIDEngine::getD() const {
	return C.d;
}

void PIDEngine::setSetPoint(double _setpoint) {
	setpoint = _setpoint;
}

double PIDEngine::getSetPoint() const  {
	return setpoint;
}

void PIDEngine::setOutMinMax(double _minVal, double _maxVal) {
	minVal = _minVal;
	maxVal = _maxVal;

	integralMinVal = minVal / C.i;
	integralMaxVal = maxVal / C.i;
}

double PIDEngine::getNextDriveValue(double currentValue) {

	// Calculate the error
	double error = setpoint - currentValue;

	// Proportional term
	double pTerm = C.p * error;

	// Calculate the cumulative value
	integralVal += error;
	if (integralVal > integralMaxVal) {
		integralVal = integralMaxVal;
	} else if (integralVal < integralMinVal) {
		integralVal = integralMinVal;
	}

	// Calculate the integral term
	double iTerm = C.i * integralVal;

	// Calculate the derivative term
	int dTerm = C.d * (lastCurrentValue - currentValue);
	lastCurrentValue = currentValue;

	// Sum P,I,D terms contributions
	double output = pTerm + iTerm + dTerm;

	// Clamp output
	if (output > maxVal) {
		output = maxVal;
	} else if (output < minVal) {
		output = minVal;
	}

	return output;
}


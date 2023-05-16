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

#include "IntChamberTempRef.h"
#include "ADT7470Device.h"

IntChamberTempRef IntChamberTempRef::instance;

#define MAX_TIMEOUT_VALUE_FOR_CHANNEL		  120	/* In seconds */
#define MIN_TEMP_VALID_FOR_CHAMBER			-1000	/* In 1/100 C */
#define MAX_TEMP_VALID_FOR_CHAMBER			 5500	/* In 1/100 C */
#define INTCHAMBER_TEMPREF_ENGINE_PERIOD	 99		/* Once a second */

/*
 * This class is responsible to provide the internal temperature based
 * on the user choice (set via an external command). The default is to
 * use a temperature sensor connected to the ADT7470 IC.
 * When other sensors are selected, it's required for the engine to be
 * in sampling mode. This assures that the other temperature sensors,
 * present in the board and/or external sensors, are polled for the
 * current temperature. For this reason, when sampling mode is disabled,
 * the reference falls back to the temperature sensor connected to the
 * ADT7470 IC.
 */
IntChamberTempRef::IntChamberTempRef() :
	curSource(SOURCE_ADT7470_T_INT_CHAMBER), setpoint(2500), prescaler(0) {

	for (unsigned char n = 0; n < (unsigned char) SOURCE_FIRST_INVALID; n++) {
		curTemperatures[n].timeout = MAX_TIMEOUT_VALUE_FOR_CHANNEL + 1;
	}
}

IntChamberTempRef::~IntChamberTempRef() {
}

void IntChamberTempRef::setSource(source _source) {
	curSource = _source;
}

/* Setpoint is expressed in 1/100 C */
void IntChamberTempRef::setSetpoint(short _setpoint) {
	setpoint = _setpoint;
}


/* Latch the read temperature for a specific channel.
 * Temperature is expressed in 1/100 C */
void IntChamberTempRef::setReadTemperature(source _source, short temperature) {

	if (_source < SOURCE_FIRST_INVALID) {
		curTemperatures[_source].value = temperature;
		curTemperatures[_source].timeout = 0;
	}
}

/* Setpoint and temperature is expressed in 1/100 C */
void IntChamberTempRef::getChamberSetpointAndTemperature(short& _setpoint, short& temperature) {

	// Return set-point
	_setpoint = setpoint;

	// Verify for the last valid sample age and the temperature value.
	// If over the threshold, fall-back to the temperature sensor connected to the ADT7470
	if (curSource >= SOURCE_FIRST_INVALID) {
		curSource = SOURCE_ADT7470_T_INT_CHAMBER;
	}
	bool fallback = (curTemperatures[curSource].timeout > MAX_TIMEOUT_VALUE_FOR_CHANNEL) &&
						checkTemperatureRange(curTemperatures[curSource].value);

	// Fall-back to the ADT7470 connected temperature IC if required
	if ((curSource == SOURCE_ADT7470_T_INT_CHAMBER) || (fallback)) {
		temperature = AS_ADT7470.getChamberTemperature();
		return;
	}

	// Otherwise returns the latest read temperature by the specified sensor
	temperature = curTemperatures[curSource].value;
}

/* Increase the timeout counter for each possible source channel, each second */
void IntChamberTempRef::tick() {

	prescaler++;
	if (prescaler > INTCHAMBER_TEMPREF_ENGINE_PERIOD) {
		prescaler = 0;

		for (unsigned char n = 0; n < (unsigned char) SOURCE_FIRST_INVALID; n++) {
			if (curTemperatures[n].timeout <= MAX_TIMEOUT_VALUE_FOR_CHANNEL) {
				curTemperatures[n].timeout++;
			}
		}
	}
}

/* Check that the value read by the sensor is compatible for the
 * internal chamber temperature
 */
bool IntChamberTempRef::checkTemperatureRange(short temperature) {

	return (temperature >= MIN_TEMP_VALID_FOR_CHAMBER) && (temperature <= MAX_TEMP_VALID_FOR_CHAMBER);
}

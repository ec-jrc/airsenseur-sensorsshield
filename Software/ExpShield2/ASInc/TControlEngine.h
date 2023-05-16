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


#ifndef TCONTROLENGINE_H_
#define TCONTROLENGINE_H_

#include "PIDEngine.h"

class TControlEngine : public PIDEngine {
public:
	virtual ~TControlEngine();

	// Temperatures setpoint and current values are expressed in 1/100 C
	void setTemperatureSetpoint(short tSetpoint);

	// Deadzones, in percentage expressed 1/100, are the min and max limit
	// near zero where the cooler and heater are not working. This generates
	// a thresholded band to avoid heat/cool process goes forth and back
	// when the PID is in the surrounding of zero.
	void setCoolingStartPercentage(unsigned short _coolingMin);
	void setHeatingStartPercentage(unsigned short _heatingMin);

	void tick();
	void loop(short currentTemperature);

	void setEnabled(bool on);
	short getCurrentDrive();

	static inline TControlEngine* getInstance() { return &instance; }

private:
	TControlEngine();

private:
	bool enabled;
	unsigned char prescaler;
	unsigned char extFanBlankTimer;
	unsigned char overheatBlankTimer;
	short currentDrive;
	unsigned short coolingMin;
	unsigned short heatingMin;

private:
	static TControlEngine instance;
};

#define AS_TCONTROL (*(TControlEngine::getInstance()))

#endif /* TCONTROLENGINE_H_ */

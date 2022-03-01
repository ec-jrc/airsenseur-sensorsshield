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

#ifndef CCDRIVEENGINE_H_
#define CCDRIVEENGINE_H_

#include "PIDEngine.h"

class CCDriveEngine : public PIDEngine {
public:
	virtual ~CCDriveEngine();

	// Current drive setpoint and actual values are expressed in 1/100 %
	void setCurrentSetPoint(unsigned short _cSetpoint);

	void tick(unsigned short _currentMeasure);
	void loop();

	void setEnabled(bool on);
	unsigned short getCurrentDrive();

	static inline CCDriveEngine* getInstance() { return &instance; }

private:
	CCDriveEngine();

private:
	bool enabled;
	bool go;
	unsigned short currentMeasure;
	unsigned short currentDrive;

private:
	static CCDriveEngine instance;
};

#define AS_CCDRIVER (*(CCDriveEngine::getInstance()))

#endif /* CCDRIVEENGINE_H_ */

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

#ifndef PWMHELPER_H_
#define PWMHELPER_H_

class PWMHelper {
public:

	virtual ~PWMHelper();

	void init();

	// set Duty cycle in 0.01% (from 0 to 10000)
	void setHeater1DutyCycle(unsigned short duty);
	void setHeater2DutyCycle(unsigned short duty);
	void setPeltierDutyCycle(unsigned short duty);

	static inline PWMHelper* getInstance() { return &instance; }

private:
	PWMHelper();

private:
	static PWMHelper instance;
};

#define AS_PWM (*(PWMHelper::getInstance()))

#endif /* PWMHELPER_H_ */

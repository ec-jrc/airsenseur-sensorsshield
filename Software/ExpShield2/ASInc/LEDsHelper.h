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

#ifndef LEDSHELPER_H_
#define LEDSHELPER_H_

class LEDsHelper {

public:
	typedef enum _leds {
		HEARTBEAT = 0,
		TXDATA,
		RXDATA,
		DUMMYLAST
	} leds;

public:
	LEDsHelper();

private:
	virtual ~LEDsHelper();

public:
	static inline LEDsHelper* getInstance() { return &instance; }
	virtual void init();
	virtual void tick();
	virtual void enable(bool enabled);
	virtual void pulse(leds ledName);

private:
	static LEDsHelper instance;
	volatile unsigned char ledTimers[DUMMYLAST];
	volatile bool ledsEnabled;
	volatile unsigned short ledsEnabledTimer;
};

#define LEDs (*(LEDsHelper::getInstance()))

#endif /* LEDSHELPER_H_ */

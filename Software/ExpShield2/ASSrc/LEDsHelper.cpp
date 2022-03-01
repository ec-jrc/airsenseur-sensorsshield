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

#include "LEDsHelper.h"
#include "GPIOHelper.h"

// Singleton LEDsHelper instance
LEDsHelper LEDsHelper::instance;

#define LED_TMR   20		         /* 200 milliseconds */
#define LEDS_ENABLED_TMR		12000	 /* 120 seconds */

LEDsHelper::LEDsHelper() {
	ledsEnabled = true;
	ledsEnabledTimer = LEDS_ENABLED_TMR;
}

LEDsHelper::~LEDsHelper() {
}

void LEDsHelper::init() {

	for (unsigned char n = 0; n < DUMMYLAST; n++) {
		ledTimers[n] = 0;
	}

    ledsEnabled = true;
    ledsEnabledTimer = LEDS_ENABLED_TMR;
}

void LEDsHelper::tick() {

	if (ledsEnabledTimer != 0) {
		ledsEnabledTimer--;
	} else {
		ledsEnabled = false;
	}

    if (ledTimers[HEARTBEAT] != 0) {
    		ledTimers[HEARTBEAT]--;
        AS_GPIO.digitalWrite(HB_LED_ENPIN, true);
    } else {
    		AS_GPIO.digitalWrite(HB_LED_ENPIN, false);
    }
    if (!ledsEnabled) {
    		AS_GPIO.digitalWrite(TX_LED_ENPIN, false);
    		AS_GPIO.digitalWrite(RX_LED_ENPIN, false);
    } else {
    		if (ledTimers[TXDATA] != 0) {
    			ledTimers[TXDATA]--;
    			AS_GPIO.digitalWrite(TX_LED_ENPIN, true);
    		} else {
    			AS_GPIO.digitalWrite(TX_LED_ENPIN, false);
    		}
    		if (ledTimers[RXDATA] != 0) {
    			ledTimers[RXDATA]--;
    			AS_GPIO.digitalWrite(RX_LED_ENPIN, true);
    		} else {
    			AS_GPIO.digitalWrite(RX_LED_ENPIN, false);
    		}
    }
}

void LEDsHelper::enable(bool enabled) {
	ledsEnabled = enabled;
	ledsEnabledTimer = LEDS_ENABLED_TMR;
}

void LEDsHelper::pulse(leds ledName) {
	if (ledName < DUMMYLAST) {
		ledTimers[ledName] = LED_TMR;
	}
}

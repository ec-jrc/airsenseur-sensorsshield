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

#include "K96SamplesAverager.h"
#include "K96Device.h"

K96SamplesAverager::K96SamplesAverager(const unsigned char _channels)
					: SamplesAverager (_channels-1),
					  lastErrorValue(0), periodTerminated(false) {
}

K96SamplesAverager::~K96SamplesAverager() {
}

bool K96SamplesAverager::collectSample(unsigned char channel, unsigned short sample, unsigned long _timestamp) {

	if (channel < K96_CHANNEL_ERRORSTATUS) {
		periodTerminated = SamplesAverager::collectSample(channel, sample, _timestamp);
		return periodTerminated;
	}

	if (channel == K96_CHANNEL_ERRORSTATUS) {
		lastErrorValue = sample;
		return periodTerminated;
	}

	return false;
}


unsigned short K96SamplesAverager::lastAveragedValue(unsigned char channel) {

	if (channel < K96_CHANNEL_ERRORSTATUS) {
		return SamplesAverager::lastAveragedValue(channel);
	} else if (channel == K96_CHANNEL_ERRORSTATUS) {
		return lastErrorValue;
	}

	return 0;
}


float K96SamplesAverager::lastAveragedFloatValue(unsigned char channel) {
	if (channel < K96_CHANNEL_ERRORSTATUS) {
		return SamplesAverager::lastAveragedFloatValue(channel);
	} else if (channel == K96_CHANNEL_ERRORSTATUS) {
		return (float)lastErrorValue;
	}

	return 0.0f;
}

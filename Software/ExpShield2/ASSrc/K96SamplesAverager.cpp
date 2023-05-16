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
#include <string.h>

K96SamplesAverager::K96SamplesAverager(const unsigned char _channels)
					: SamplesAverager (_channels-K96SAMPLEAVG_ERRORFIELDS),
					  periodTerminated(false) {

	memset(lastErrorValues, 0, sizeof(lastErrorValues));
	memset(clearLastError, 0, sizeof(clearLastError));
}

K96SamplesAverager::~K96SamplesAverager() {
}

bool K96SamplesAverager::collectSample(unsigned char channel, unsigned short sample, unsigned long _timestamp) {

	if (channel < K96_CHANNEL_ERRORSTATUS) {
		periodTerminated = SamplesAverager::collectSample(channel, sample, _timestamp);
		return periodTerminated;
	}

	// For all channels who need to be bitwise or-ed, we need
	// to understand when the averaging period is terminated, so we can
	// clear the or-ed error status on the 1st sample for the next period
	unsigned char errorChannel = channel - K96_CHANNEL_ERRORSTATUS;
	if (errorChannel >= K96SAMPLEAVG_ERRORFIELDS) {
		return periodTerminated;
	}
	if (periodTerminated) {
		clearLastError[errorChannel] = true;
	}

	// This happens only the 1st sample of each averaging period
	// or the averager is not running
	if (!isConsolidated() ||
			(getBufferSize() == 0) ||
			(clearLastError[errorChannel] && !periodTerminated)) {

		lastErrorValues[errorChannel] = 0;
		clearLastError[errorChannel] = false;
	}

	if (errorChannel < K96SAMPLEAVG_ERRORFIELDS) {

		lastErrorValues[errorChannel] |= sample;
		return periodTerminated;
	}

	return false;
}


unsigned short K96SamplesAverager::lastAveragedValue(unsigned char channel) {

	if (channel < K96_CHANNEL_ERRORSTATUS) {
		return SamplesAverager::lastAveragedValue(channel);
	} else if ((channel >= K96_CHANNEL_ERRORSTATUS) && (channel < K96_NUM_OF_CHANNELS)) {
		return lastErrorValues[channel-K96_CHANNEL_ERRORSTATUS];
	}

	return 0;
}


float K96SamplesAverager::lastAveragedFloatValue(unsigned char channel) {
	if (channel < K96_CHANNEL_ERRORSTATUS) {
		return SamplesAverager::lastAveragedFloatValue(channel);
	} else if ((channel >= K96_CHANNEL_ERRORSTATUS) && (channel < K96_NUM_OF_CHANNELS)) {
		return (float)lastErrorValues[channel - K96_CHANNEL_ERRORSTATUS];
	}

	return 0.0f;
}

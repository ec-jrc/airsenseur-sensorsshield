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

#include <SamplesAveragerNextPM.h>
#include "NextPMDevice.h"

SamplesAveragerNextPM::SamplesAveragerNextPM()
: SamplesAverager (NEXTPM_NUM_CHANNELS-1), sampleTime(0), sensorStatus(0), periodTerminated(0) {
}

SamplesAveragerNextPM::~SamplesAveragerNextPM() {
}

bool SamplesAveragerNextPM::collectSample(unsigned char channel, unsigned short sample, unsigned long _timestamp) {

	if (channel < NEXTPM_STATUS) {
		periodTerminated = SamplesAverager::collectSample(channel, sample, _timestamp);
		return periodTerminated;
	}

	if (channel == NEXTPM_STATUS) {
		sensorStatus = sample;
		return periodTerminated;
	}

	return false;
}

unsigned short SamplesAveragerNextPM::lastAveragedValue(unsigned char channel) {

	if (channel < NEXTPM_STATUS) {
		return SamplesAverager::lastAveragedValue(channel);
	} else if (channel == NEXTPM_STATUS) {
		return sensorStatus;
	}

	return 0;
}

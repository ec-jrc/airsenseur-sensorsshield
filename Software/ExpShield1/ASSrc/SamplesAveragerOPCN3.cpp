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

#include "SamplesAveragerOPCN3.h"
#include "OPCN3Device.h"

/* This specific averager for OPCN3 should average all channels except for:
 * - Volume (should be cumulated between each samples in the averager's deep)
 * - Sample time (extracted for debug purposes and set at the latest read value)
 * - Flow rate (extracted for debug purposes and set at the latest read value)
 * - Laser status (extracted for debug purposes and set at the latest read value)
 */
SamplesAveragerOPCN3::SamplesAveragerOPCN3() : SamplesAverager(OPCN3_CHAN_NUMBER-3),
	sampleTime(0), sampleFlowRate(0), laserStatus(0), periodTerminated(false) {
}

SamplesAveragerOPCN3::~SamplesAveragerOPCN3() {
}

bool SamplesAveragerOPCN3::collectSample(unsigned char channel, unsigned short sample, unsigned long _timestamp) {

	if (channel <= OPCN3_VOL) {
		periodTerminated = SamplesAverager::collectSample(channel, sample, _timestamp);
		return periodTerminated;
	} else if (channel == OPCN3_TSA) {
		sampleTime = sample;
		return periodTerminated;
	} else if (channel == OPCN3_FRT) {
		sampleFlowRate = sample;
		return periodTerminated;
	} else if (channel == OPCN3_LSRST) {
		laserStatus = sample;
		return periodTerminated;
	}

	return false;
}

unsigned short SamplesAveragerOPCN3::lastAveragedValue(unsigned char channel) {

	if (channel < OPCN3_VOL) {
		return SamplesAverager::lastAveragedValue(channel);
	} else if (channel == OPCN3_VOL) {
		return *(getAccumulators()+channel);
	} else if (channel == OPCN3_TSA) {
		return sampleTime;
	} else if (channel == OPCN3_FRT) {
		return sampleFlowRate;
	} else if (channel == OPCN3_LSRST) {
		return laserStatus;
	}

	return 0;
}

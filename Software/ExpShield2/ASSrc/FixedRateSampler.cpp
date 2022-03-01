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

#include "FixedRateSampler.h"

const unsigned char FixedRateSampler::DeviceDrivenSampleRate() {
	return 0x00;
}

// Prescaler is set at construction time
FixedRateSampler::FixedRateSampler(SensorDevice* const sensorDevice, unsigned char fixedPrescaler)
																: Sampler(sensorDevice), fixedDecimation(false) {
	Sampler::setPreScaler(fixedPrescaler);
}

// Prescaler and decimation are set at construction time
FixedRateSampler::FixedRateSampler(SensorDevice* const sensorDevice, unsigned char fixedPrescaler, unsigned char fixedDecimation)
																: Sampler(sensorDevice), fixedDecimation(true) {
	Sampler::setPreScaler(fixedPrescaler);
	Sampler::setDecimation(fixedDecimation);
}

FixedRateSampler::~FixedRateSampler() {
}

// Avoid changing the prescaler in lifetime
void FixedRateSampler::setPreScaler(unsigned char value) {
}


// Avoid changing the decimation in lifetime (if required)
void FixedRateSampler::setDecimation(unsigned char value) {
	if (!fixedDecimation) {
		Sampler::setDecimation(value);
	}
}

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

#ifndef SAMPLEAVERAGERNEXTPM_H_
#define SAMPLEAVERAGERNEXTPM_H_

#include "SamplesAverager.h"

class SamplesAveragerNextPM : public SamplesAverager {
public:
	SamplesAveragerNextPM();
	virtual ~SamplesAveragerNextPM();

	virtual bool collectSample(unsigned char channel, unsigned short sample, unsigned long _timestamp);
	virtual unsigned short lastAveragedValue(unsigned char channel);

private:
	unsigned short sampleTime;
	unsigned short sensorStatus;
	bool periodTerminated;
};

#endif /* SAMPLEAVERAGERNEXTPM_H_ */

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

#ifndef LMP91000EVAL_H_
#define LMP91000EVAL_H_

#include "LMP91000.h"

class LMP91000Eval : public LMP91000 {
public:
	LMP91000Eval(const unsigned char menbPin);
	virtual ~LMP91000Eval();

private:
	static const unsigned char loadTable[];
	static const double gainTable[];
	static const unsigned char intZeroTable[];

private:
	unsigned char load;
	unsigned char gain;
	unsigned char intZero;
	bool intSource;

public:
	unsigned char getLoad() const;
	double getGain() const;
	unsigned char getIntZero() const;
	bool getIntSource() const;

protected:
    virtual bool writeRegister(unsigned char address, unsigned char value);
    virtual bool readRegister(unsigned char address, unsigned char* value);

};

#endif /* LMP91000EVAL_H_ */

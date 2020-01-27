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

#include "LMP91000Eval.h"

const unsigned char LMP91000Eval::loadTable[] = { 10, 33, 50, 100 };
const double LMP91000Eval::gainTable[] = { 1e9, 2750, 3500, 7000, 14000, 35000, 120000, 350000 };
const unsigned char LMP91000Eval::intZeroTable[] = { 20, 50, 67, 00 };


LMP91000Eval::LMP91000Eval(const unsigned char menbPin) : LMP91000(menbPin),
		load(0), gain(0), intZero(0), intSource(false) {
}

LMP91000Eval::~LMP91000Eval() {

}

bool LMP91000Eval::writeRegister(unsigned char address, unsigned char value) {

	bool result = LMP91000::writeRegister(address, value);
	if (result) {

		switch (address) {
			case LMP91000_REG_TIACN: {
					load = value & 0x03;
					gain = (value >> 2) & 0x07;
				}
				break;
			case LMP91000_REG_REFCN: {
					intSource = ((value & 0x80) == 0);
					intZero = (value >> 5) & 0x03;
				}
				break;
			default:
				break;
		}
	}

	return result;
}

bool LMP91000Eval::readRegister(unsigned char address, unsigned char* value) {

	bool result = LMP91000::readRegister(address, value);
	if (result) {

		switch (address) {
			case LMP91000_REG_TIACN: {
					load = (*value) & 0x03;
					gain = ((*value) >> 2) & 0x07;
				}
				break;
			case LMP91000_REG_REFCN: {
					intSource = (((*value) & 0x80) == 0);
					intZero = ((*value) >> 5) & 0x03;
				}
				break;
			default:
				break;
		}
	}

	return result;
}

unsigned char LMP91000Eval::getLoad() const {
	return loadTable[load];
}

double LMP91000Eval::getGain() const {
	return gainTable[gain];
}

unsigned char LMP91000Eval::getIntZero() const {
	return intZeroTable[intZero];
}

bool LMP91000Eval::getIntSource() const {
	return intSource;
}


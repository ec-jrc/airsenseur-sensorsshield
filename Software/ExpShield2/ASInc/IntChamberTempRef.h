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

#ifndef INTCHAMBERTEMPREF_H_
#define INTCHAMBERTEMPREF_H_

class IntChamberTempRef {

public:
	typedef enum _source {
		SOURCE_TEMPERATURE_I = 0x00,
		SOURCE_ADT7470_T_INT_CHAMBER = 0x01,
		SOURCE_K96_TEMP_NTC0 = 0x02,
		SOURCE_K96_TEMP_NTC1 = 0x03,
		SOURCE_K96_T_RH0 = 0x04,
		SOURCE_FIRST_INVALID = 0x05,
	} source ;

public:
	IntChamberTempRef();
	virtual ~IntChamberTempRef();

public:
	void setSource(source _source);
	void setSetpoint(short _setpoint);
	void setReadTemperature(source _source, short temperature);
	void getChamberSetpointAndTemperature(short& _setpoint, short& temperature);

public:
	void tick();
	static inline IntChamberTempRef* getInstance() { return &instance; }

private:
	bool checkTemperatureRange(short temperature);

private:
	typedef struct _temperatures {
		short value;
		short timeout;
	} temperatures;

private:
	source curSource;
	short setpoint;
	unsigned short prescaler;
	temperatures curTemperatures[(unsigned short)SOURCE_FIRST_INVALID];

private:
	static IntChamberTempRef instance;

};

#define AS_INTCH_TEMPREF (*(IntChamberTempRef::getInstance()))

#endif /* INTCHAMBERTEMPREF_H_ */

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

#ifndef PIDENGINE_H_
#define PIDENGINE_H_

class PIDEngine {
public:
	PIDEngine();
	virtual ~PIDEngine();

	void setCoefficients(double p, double i, double d);
	void setOutMinMax(double _minVal, double _maxVal);
	void setSetPoint(double _setpoint);

	double getP() const;
	double getI() const;
	double getD() const;
	double getSetPoint() const;

protected:
	double getNextDriveValue(double currentValue);

private:
	typedef struct _coefficients {
		double p;
		double i;
		double d;
	} coefficients;

private:
	coefficients C;
	double minVal;
	double maxVal;
	double integralMinVal;
	double integralMaxVal;
	double setpoint;
	double lastCurrentValue;
	double integralVal;
};

#endif /* PIDENGINE_H_ */

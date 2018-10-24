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

#ifndef _BMP280_H_
#define _BMP280_H_


class BMP280
{
  public:
    BMP280();
    virtual ~BMP280();

    // Initialize the sensor and reads
    // internal calibration coefficients
    // Returns 0 if fails    
    bool begin();

    // Read temperature
    // Returns 0 if fails
    char getTemperature(double &T);

    // Read pressure. 
    // Returns 0 if fails    
    char getPressure(double &P);

  private:
    typedef struct _calibrationdata
    {
      unsigned short dig_T1;
      short dig_T2;
      short dig_T3;
      unsigned short dig_P1;
      short dig_P2;
      short dig_P3;
      short dig_P4;
      short dig_P5;
      short dig_P6;
      short dig_P7;
      short dig_P8;
      short dig_P9;
    } calibrationdata;  

  private:
    char readData(char address, unsigned char &value);
    char readCalibRegister(char address, short *value);
    char writeRegister(unsigned char address, unsigned char value);

    char readTemperatureRegisters(long &adcTemperature);
    char readPressureRegisters(long &adcPressure);

  private:
    calibrationdata calibrationData;
    long t_fine;
    char errorCode;
};

#endif /* _BMP280_H_ */


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

#include <Arduino.h>
#include "BMP280.h"
#include <Wire.h>

// Some constants definitions
#define BMP280ADDRESS 0x76

// Register definitions
#define BMP280_REG_CAL_DIGT1  0x88
#define BMP280_REG_CAL_DIGT2  0x8A
#define BMP280_REG_CAL_DIGT3  0x8C
#define BMP280_REG_CAL_DIGP1  0x8E
#define BMP280_REG_CAL_DIGP2  0x90
#define BMP280_REG_CAL_DIGP3  0x92
#define BMP280_REG_CAL_DIGP4  0x94
#define BMP280_REG_CAL_DIGP5  0x96
#define BMP280_REG_CAL_DIGP6  0x98
#define BMP280_REG_CAL_DIGP7  0x9A
#define BMP280_REG_CAL_DIGP8  0x9C
#define BMP280_REG_CAL_DIGP9  0x9E
#define BMP280_REG_ID         0xD0
#define BMP280_REG_RESET      0xE0
#define BMP280_REG_STATUS     0xF3
#define BMP280_REG_CTRL_MEAS  0xF4
#define BMP280_REG_CONFIG     0xF5
#define BMP280_REG_PRESS_MSB  0xF7
#define BMP280_REG_PRESS_LSB  0xF8
#define BMP280_REG_PRESS_XLSB 0xF9
#define BMP280_REG_TEMP_MSB   0xFA
#define BMP280_REG_TEMP_LSB   0xFB
#define BMP280_REG_TEMP_XLSB  0xFC


BMP280::BMP280() : t_fine(0), errorCode(0) {
}


BMP280::~BMP280() {
}

bool BMP280::begin() {

  Wire.begin();

  // Read chipsed ID to check for the IC presence on the bus
  unsigned char value = 0;
  if (!readData(BMP280_REG_ID, value) || (value != 0x58)) {
    return false;
  }

  // Read all calibration data
  if (!readCalibRegister(BMP280_REG_CAL_DIGT1, (short*)&(calibrationData.dig_T1)) ||
      !readCalibRegister(BMP280_REG_CAL_DIGT2, &(calibrationData.dig_T2)) ||
      !readCalibRegister(BMP280_REG_CAL_DIGT3, &(calibrationData.dig_T3)) ||
      !readCalibRegister(BMP280_REG_CAL_DIGP1, (short*)&(calibrationData.dig_P1)) ||
      !readCalibRegister(BMP280_REG_CAL_DIGP2, &(calibrationData.dig_P2)) ||
      !readCalibRegister(BMP280_REG_CAL_DIGP3, &(calibrationData.dig_P3)) ||
      !readCalibRegister(BMP280_REG_CAL_DIGP4, &(calibrationData.dig_P4)) ||
      !readCalibRegister(BMP280_REG_CAL_DIGP5, &(calibrationData.dig_P5)) ||
      !readCalibRegister(BMP280_REG_CAL_DIGP6, &(calibrationData.dig_P6)) ||
      !readCalibRegister(BMP280_REG_CAL_DIGP7, &(calibrationData.dig_P7)) ||
      !readCalibRegister(BMP280_REG_CAL_DIGP8, &(calibrationData.dig_P8)) ||
      !readCalibRegister(BMP280_REG_CAL_DIGP9, &(calibrationData.dig_P9)) ) {
    return false;
  }

  // Setup the sensor running in:
  // - Normal mode
  // - x16 pressure oversampling (20bit/sample)
  // - x16 temperature oversampling (20bit/sample)
  // - IIR disabled
  // - Standby = 0.5ms
  if (!writeRegister(BMP280_REG_CTRL_MEAS, 0xB7) ||
      !writeRegister(BMP280_REG_CONFIG, 0x00) ) {
    return false;
  }

  return true;    
}

char BMP280::getTemperature(double &T) {

  long adc_T;
  if (!readTemperatureRegisters(adc_T)) {
    return 0;
  }

  // Apply fixed math compensation (see sensor datasheet, 3.11.3 Compensation formula chapter)
  int32_t var1  = ((((adc_T>>3) - ((int32_t)calibrationData.dig_T1 <<1))) * ((int32_t)calibrationData.dig_T2)) >> 11;
  int32_t var2  = (((((adc_T>>4) - ((int32_t)calibrationData.dig_T1)) * ((adc_T>>4) - ((int32_t)calibrationData.dig_T1))) >> 12) * ((int32_t)calibrationData.dig_T3)) >> 14;  

  t_fine = (int32_t)(var1 + var2);
  T = ((float)(((var1 + var2) * 5 + 128) >> 8))/100;
  return 1;
}

char BMP280::getPressure(double &P) {

  long adc_P;
  if (!readPressureRegisters(adc_P)) {
    return 0;
  }

  // Apply fixed math compensation (see sensor datasheet, 3.11.3 Compensation formula chapter)
  int64_t var1 = ((int64_t)t_fine) - 128000;
  int64_t var2 = var1 * var1 * (int64_t)calibrationData.dig_P6;
  var2 = var2 + ((var1*(int64_t)calibrationData.dig_P5)<<17);
  var2 = var2 + (((int64_t)calibrationData.dig_P4)<<35);
  var1 = ((var1 * var1 * (int64_t)calibrationData.dig_P3)>>8) + ((var1 * (int64_t)calibrationData.dig_P2)<<12);
  var1 = (((((int64_t)1)<<47)+var1))*((int64_t)calibrationData.dig_P1)>>33;
  if (var1 == 0) {
    return 0;  // avoid exception caused by division by zero
  }
  
  int64_t p = (int64_t)1048576 - (int64_t)adc_P;
  p = (((p<<31) - var2)*3125) / var1;

  var1 = (((int64_t)calibrationData.dig_P9) * (p>>13) * (p>>13)) >> 25;
  var2 = (((int64_t)calibrationData.dig_P8) * p) >> 19;

  p = ((p + var1 + var2) >> 8) + (((int64_t)calibrationData.dig_P7)<<4);
  
  P = ((double)p)/25600;  

  return 1;
}


char BMP280::readTemperatureRegisters(long &adcTemperature) {
  unsigned char data[3];

  *data = BMP280_REG_TEMP_MSB;
  if (readBuffer(data,3)) {
    adcTemperature = ((long)(((long)data[0]<<16) | ((long)data[1]<<8) | (long)data[2]))>>4;
    return 1;
  }

  return 0;
}

char BMP280::readPressureRegisters(long &adcPressure) {
  unsigned char data[3];

  *data = BMP280_REG_PRESS_MSB;
  if (readBuffer(data,3)) {
    adcPressure = ((long)(((long)data[0]<<16) | ((long)data[1]<<8) | (long)data[2]))>>4;
    return 1;
  }

  return 0;
}

char BMP280::readData(char address, unsigned char &value) {

  if (readBuffer((unsigned char*)&address,1)) {
    value = address;
    return 1;
  }

  value = 0;
  return 0;
}

char BMP280::readCalibRegister(char address, short *value) {
  unsigned char data[2];

  *data = address;
  if (readBuffer(data,2)) {
    *value = ( ( (short)data[1]<<8) | (short)data[0] );
    return 1;
  }
  
  *value = 0;
  return 0;  
}

char BMP280::readBuffer(unsigned char *buffer, char bufferSize) {
  Wire.beginTransmission(BMP280ADDRESS);
  Wire.write(buffer[0]);
  errorCode = Wire.endTransmission();
  if (errorCode == 0) {
    
    Wire.requestFrom(BMP280ADDRESS,bufferSize);
    while(Wire.available() != bufferSize) ; // wait until bytes are ready
    for(unsigned char n = 0; n < bufferSize; n++, buffer++) {
      
      *buffer = Wire.read();
    }
    return 1;
  }
  return 0;
}

char BMP280::writeRegister(unsigned char address, unsigned char value) {

  unsigned char data[2];
  
  data[0] = address;
  data[1] = value;
  
  unsigned char result = writeBuffer(data, 2);
  return (result)? 1:0;
}

char BMP280::writeBuffer(unsigned char *buffer, char bufferSize) {
  Wire.beginTransmission(BMP280ADDRESS);
  Wire.write(buffer,bufferSize);
  errorCode = Wire.endTransmission();
  if (errorCode == 0) {
    return 1;
  }
  
  return 0;
}



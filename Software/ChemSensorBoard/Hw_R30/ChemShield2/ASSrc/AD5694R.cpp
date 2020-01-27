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

#include "AD5694R.h"
#include "GPIOHelper.h"
#include "EEPROMHelper.h"
#include "I2CBHelper.h"

#define AD5694_VAL_GAINNOR      LOW
#define AD5694_VAL_GAINDOUBLE   HIGH

/* I2C device general address */
#define AD5694_ADDRESS          0x18

AD5694R::AD5694R(const unsigned char gainPin, const unsigned char address) 
        : m_gainPin(gainPin), m_address(AD5694_ADDRESS|address) {

	AS_GPIO.digitalWrite(gainPin, AD5694_VAL_GAINNOR);
}

AD5694R::~AD5694R() {
    
}

bool AD5694R::setGain(bool doubleGain) {

	AS_GPIO.digitalWrite(m_gainPin, (doubleGain == true)? AD5694_VAL_GAINDOUBLE:AD5694_VAL_GAINNOR);
    
    return true;
}

bool AD5694R::setValue(unsigned char channelId, unsigned short value) {
    
    unsigned char lsb = ((value & 0x0F) << 4);
    unsigned char msb = (value >> 4) & 0xFF;
    
    return writeRegister(AD5694_CMD_WANDUPD|channelId, msb, lsb);
}

bool AD5694R::init() {

    // Turn On DAC A, B, C; 
    // DAC D is tied to ground with 1kohm
    unsigned char data = 0x40;
    return writeRegister(AD5694_CMD_PWRUD, 0, data);
}

bool AD5694R::writeRegisters(unsigned char channelId, unsigned short value, bool gain) {
    
    bool result = true;

    channelId = (1<<(channelId))&0x0F;
    
    result &= setValue(channelId, value);
    result &= setGain(gain);
       
    return result;
}

bool AD5694R::readRegisters(unsigned char channelId, unsigned short* value, bool* gain) {

    unsigned char msb;
    unsigned char lsb;
    
    channelId = (1<<(channelId))&0x0F;
    
    bool result = readRegister(AD5694_CMD_WANDUPD|channelId, &msb, &lsb);
    *value = ((((unsigned short)msb)<<4) & 0x0FF0) | ((lsb>>4) & 0x0F);

    lsb = (AS_GPIO.digitalRead(m_gainPin) == AD5694_VAL_GAINNOR)? 0x00 : 0x01;
    *gain = (lsb != 0);
        
    return result;
}

bool AD5694R::writeRegister(unsigned char address, unsigned char msb, unsigned char lsb) {

    bool result;

    unsigned char pData[] = { msb, lsb };
    result = I2CB.write(m_address, address, 0x01, pData, 2);
    
    return result;
}

bool AD5694R::readRegister(unsigned char address, unsigned char* msb, unsigned char* lsb) {

    bool result = false;

    unsigned char pData[2];
    result = I2CB.read(m_address, address, 0x01, pData, 2);
    *msb = pData[0];
    *lsb = pData[1];
        
    return result;
}

bool AD5694R::storePreset() {
    
    unsigned char lsb;
    unsigned char msb;
    
    // Channel A
    bool result = readRegister(AD5694_CMD_WANDUPD|AD5694_DAC_A, &msb, &lsb);
    EEPROM.write(AD5694R_PRESET_VALA_LSB(m_gainPin), lsb);
    EEPROM.write(AD5694R_PRESET_VALA_MSB(m_gainPin), msb);

    // Channel B
    result &= readRegister(AD5694_CMD_WANDUPD|AD5694_DAC_B, &msb, &lsb);
    EEPROM.write(AD5694R_PRESET_VALB_LSB(m_gainPin), lsb);
    EEPROM.write(AD5694R_PRESET_VALB_MSB(m_gainPin), msb);
    
    // Channel C
    result &= readRegister(AD5694_CMD_WANDUPD|AD5694_DAC_C, &msb, &lsb);
    EEPROM.write(AD5694R_PRESET_VALC_LSB(m_gainPin), lsb);
    EEPROM.write(AD5694R_PRESET_VALC_MSB(m_gainPin), msb);
    
    // Channel D
    result &= readRegister(AD5694_CMD_WANDUPD|AD5694_DAC_D, &msb, &lsb);
    EEPROM.write(AD5694R_PRESET_VALD_LSB(m_gainPin), lsb);
    EEPROM.write(AD5694R_PRESET_VALD_MSB(m_gainPin), msb);
    
    // Gain
    bool gain = AS_GPIO.digitalRead(m_gainPin);
    EEPROM.write(AD5694R_PRESET_GAIN(m_gainPin), (gain)? 0xFF : 0x00);
    
    return result;
}

bool AD5694R::loadPreset() {
    
    unsigned char msb;
    unsigned char lsb;
    
    // Channel A
    lsb = EEPROM.read(AD5694R_PRESET_VALA_LSB(m_gainPin));
    msb = EEPROM.read(AD5694R_PRESET_VALA_MSB(m_gainPin));
    bool result = writeRegister(AD5694_CMD_WANDUPD|AD5694_DAC_A, msb, lsb);
    
    // Channel B
    lsb = EEPROM.read(AD5694R_PRESET_VALB_LSB(m_gainPin));
    msb = EEPROM.read(AD5694R_PRESET_VALB_MSB(m_gainPin));
    result &= writeRegister(AD5694_CMD_WANDUPD|AD5694_DAC_B, msb, lsb);
    
    // Channel C
    lsb = EEPROM.read(AD5694R_PRESET_VALC_LSB(m_gainPin));
    msb = EEPROM.read(AD5694R_PRESET_VALC_MSB(m_gainPin));
    result &= writeRegister(AD5694_CMD_WANDUPD|AD5694_DAC_C, msb, lsb);
    
    // Channel D
    lsb = EEPROM.read(AD5694R_PRESET_VALD_LSB(m_gainPin));
    msb = EEPROM.read(AD5694R_PRESET_VALD_MSB(m_gainPin));
    result &= writeRegister(AD5694_CMD_WANDUPD|AD5694_DAC_D, msb, lsb);
        
    // Gain
    lsb = EEPROM.read(AD5694R_PRESET_GAIN(m_gainPin));
    setGain(lsb != 0x00);

    return result;
}

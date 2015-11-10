/* ========================================================================
 * Copyright 2015 EUROPEAN UNION
 *
 * Licensed under the EUPL, Version 1.1 or - as soon they will be approved by 
 * the European Commission - subsequent versions of the EUPL (the "Licence"); 
 * You may not use this work except in compliance with the Licence. 
 * You may obtain a copy of the Licence at: http://ec.europa.eu/idabc/eupl
 * Unless required by applicable law or agreed to in writing, software distributed 
 * under the Licence is distributed on an "AS IS" basis, WITHOUT WARRANTIES OR 
 * CONDITIONS OF ANY KIND, either express or implied. See the Licence for the 
 * specific language governing permissions and limitations under the Licence.
 * Date: 02/04/2015
 * Authors
 * - Michel Gerboles  - michel.gerboles@jrc.ec.europa.eu,  
 *                     European Commission - Joint Research Centre, 
 * - Laurent Spinelle - laurent.spinelle@jrc.ec.europa.eu,
 *                     European Commission - Joint Research Centre, 
 * - Marco Signorini  - marco.signorini@liberaintentio.com
 * 
 * ======================================================================== 
 */

#ifndef AD5694R_H_
#define	AD5694R_H_

#define AD5694_SLAVE_1  0x00
#define AD5694_SLAVE_2  0x02
#define AD5694_SLAVE_3  0x04
#define AD5694_SLAVE_4  0x06

#define AD5694_DAC_A   0x01
#define AD5694_DAC_B   0x02
#define AD5694_DAC_C   0x04
#define AD5694_DAC_D   0x08
#define AD5694_DAC_ALL 0x0F

#define AD5694_MODE_NORMAL      0x00
#define AD5694_MODE_PD_1K       0x01
#define AD5694_MODE_PD_100K     0x02
#define AD5694_MODE_PD_HIGZ     0x03

#define AD5694_CMD_NOP          0x00
#define AD5694_CMD_WANDUPD      0x30
#define AD5694_CMD_PWRUD        0x40

class AD5694R {
    public:
        AD5694R(const unsigned char gainPin, const unsigned char address);
        virtual ~AD5694R();
        
        bool init() const;

        bool setGain(bool doubleGain) const;
        bool setValue(unsigned char channelId, unsigned short value) const;
        
        bool writeRegisters(unsigned char channelId, unsigned short value, bool gain) const;
        bool readRegisters(unsigned char channelId, unsigned short* value, bool* gain) const;
        
        bool storePreset() const;
        bool loadPreset() const;

    private:
        bool writeRegister(unsigned char address, unsigned char msb, unsigned char lsb) const;
        bool readRegister(unsigned char address, unsigned char* msb, unsigned char* lsb) const;

    private:
        const unsigned char m_gainPin;
        const unsigned char m_address;
};

#endif	/* AD5694R_H_ */


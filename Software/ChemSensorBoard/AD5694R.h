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
 *   Laurent Spinelle – laurent.spinelle@jrc.ec.europa.eu and 
 *   Alexander Kotsev - alexander.kotsev@jrc.ec.europa.eu,
 *   European Commission - Joint Research Centre, 
 * - Marco Signorini, marco.signorini@liberaintentio.com
 *
 * ===========================================================================
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


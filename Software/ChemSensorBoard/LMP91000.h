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


#ifndef LMP91000_H
#define	LMP91000_H

/* Constant definitions for setGainAndLoad function */
#define GAIN_EXTERNAL   0x00
#define GAIN_2K75       0x04
#define GAIN_3K5        0x08
#define GAIN_7K0        0x0C
#define GAIN_14K0       0x10
#define GAIN_35K0       0x14
#define GAIN_120K0      0x18
#define GAIN_350K0      0x1C

#define LOAD_10         0x00
#define LOAD_33         0x01
#define LOAD_50         0x02
#define LOAD_100        0x03

/* Constants definitions for setReferenceZeroAndBias */
#define REFSOURCE_INT   0x00
#define REFSOURCE_EXT   0x80

#define INTZ_20P        0x00
#define INTZ_50P        0x20
#define INTZ_67P        0x40
#define INTZ_BYPASS     0x60

#define BIAS_SIGN_NEG   0x00
#define BIAS_SIGN_POS   0x10

#define BIAS_0P         0x00
#define BIAS_1P         0x01
#define BIAS_2P         0x02
#define BIAS_4P         0x03
#define BIAS_6P         0x04
#define BIAS_8P         0x05
#define BIAS_10P        0x06
#define BIAS_12P        0x07
#define BIAS_14P        0x08
#define BIAS_16P        0x09
#define BIAS_18P        0x0A
#define BIAS_20P        0x0B
#define BIAS_22P        0x0C
#define BIAS_24P        0x0D

/* Constants definitions for setModeControl */
#define FET_SHORT_DISABLED      0x00
#define FET_SHORT_ENABLED       0x80

#define MODE_DEEPSLEEP          0x00
#define MODE_2LEAD_GND_REF      0x01
#define MODE_STANDBY            0x02
#define MODE_3LEAD_AMP_CELL     0x03
#define MODE_TEMPERATURE_TIAOFF 0x04
#define MODE_TEMPERATURE_TIAON  0x05

class LMP91000 {
    
    public:
        LMP91000(const unsigned char menbPin);
        ~LMP91000();
        
    public:
        bool lock() const;
        bool unLock() const;

        bool setGainAndLoad(unsigned char gain, unsigned char load) const;
        bool setReferenceZeroAndBias(unsigned char refSource, unsigned char intZero, unsigned char biasSign, unsigned char bias) const;
        bool setModeControl(unsigned char fetShort, unsigned char opMode) const;
        bool writeRegisters(unsigned char tia, unsigned char ref, unsigned char mode) const;
        bool readRegisters(unsigned char *tia, unsigned char *ref, unsigned char *mode) const;
        
        bool storePreset(unsigned char* name) const;
        bool loadPreset() const;
        void getPresetName(unsigned char* name) const;
                
    private:
        bool writeRegister(unsigned char address, unsigned char value) const;
        bool readRegister(unsigned char address, unsigned char* value) const;
        
    private:
        const unsigned char m_menbPin;
};


#endif	/* LMP91000_H */


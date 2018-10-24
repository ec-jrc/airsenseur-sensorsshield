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

#ifndef PERSISTENCE_H
#define	PERSISTENCE_H

#ifdef	__cplusplus
extern "C" {
#endif

// LEDS pins
#define HB_LED_ENPIN      0
#define TX_LED_ENPIN      1
#define RX_LED_ENPIN      2

// AFE Enable pins    
#define AFE_1_ENPIN       3
#define AFE_2_ENPIN       4
#define AFE_3_ENPIN       5
#define AFE_4_ENPIN       6

// ADC Enable pins
#define ADC_1_CSPIN       7
#define ADC_2_CSPIN       8
#define ADC_3_CSPIN       9
#define ADC_4_CSPIN       10
    
// DAC Gain pins
#define DAC_1_GAINPIN     11
#define DAC_2_GAINPIN     12
#define DAC_3_GAINPIN     13
#define DAC_4_GAINPIN     14    

// User I/O lines
#define USER_BUTTONPIN    15

#define LAST_UNUSEDPIN    16
    
// EEPROM memory map persistence    
    
// 00 - 03 -> AFE1 Registers
// 04 - 07 -> AFE2 Registers
// 08 - 0B -> AFE3 Registers
// 0C - 0F -> AFE4 Registers
#define LMP9100_BASE(a)     (((a) - AFE_1_ENPIN) << 2)
    
#define LMP9100_TIA(a)      LMP9100_BASE(a)
#define LMP9100_REF(a)      (LMP9100_BASE(a) + 1)
#define LMP9100_MODE(a)     (LMP9100_BASE(a) + 2)
    
// 10 - 17 -> Preset name 1    
// 20 - 27 -> Preset name 2
// 30 - 37 -> Preset name 3
// 40 - 47 -> Preset name 4
#define LMP9100_PRESETNAME(a)       (((a) - AFE_1_ENPIN + 1) << 4)
#define LMP9100_PRESETNAME_LENGTH   8
    
// 60 - 61 -> Sampler preset 1
// 70 - 71 -> Sampler preset 2
// 80 - 81 -> Sampler preset 3
// 90 - 91 -> Sampler preset 4
// A0 - A1 -> Sampler preset 5
// B0 - B1 -> Sampler preset 6
// C0 - C1 -> Sampler preset 7
#define SAMPLER_PRESET_BASE(a)     ((((a) + 1) << 4) + 0x50)
    
#define SAMPLER_PRESET_PRESCALER(a)     SAMPLER_PRESET_BASE(a)
#define SAMPLER_PRESET_DECIMATION(a)    (SAMPLER_PRESET_BASE(a) + 1)
#define SAMPLER_PRESET_IIR1DENOM(a)     (SAMPLER_PRESET_BASE(a) + 2)
#define SAMPLER_PRESET_IIR2DENOM(a)     (SAMPLER_PRESET_BASE(a) + 3)
    
// D0 - D7 -> Averager presets 
#define AVERAGER_PRESET_BUFSIZE(a)      (0xD0 + (a))

// 100 - 10A -> AD5694 preset 1
// 110 - 11A -> AD5694 preset 2    
// 120 - 12A -> AD5694 preset 3
// 130 - 13A -> AD5694 preset 4    
#define AD5694R_PRESET_BASE(a)          ((((a) - DAC_1_GAINPIN) << 4) + 0x100)
#define AD5694R_PRESET_VALA_LSB(a)      (AD5694R_PRESET_BASE(a))
#define AD5694R_PRESET_VALA_MSB(a)      (AD5694R_PRESET_BASE(a) + 0x01)
#define AD5694R_PRESET_VALB_LSB(a)      (AD5694R_PRESET_BASE(a) + 0x02)
#define AD5694R_PRESET_VALB_MSB(a)      (AD5694R_PRESET_BASE(a) + 0x03)
#define AD5694R_PRESET_VALC_LSB(a)      (AD5694R_PRESET_BASE(a) + 0x04)
#define AD5694R_PRESET_VALC_MSB(a)      (AD5694R_PRESET_BASE(a) + 0x05)
#define AD5694R_PRESET_VALD_LSB(a)      (AD5694R_PRESET_BASE(a) + 0x06)
#define AD5694R_PRESET_VALD_MSB(a)      (AD5694R_PRESET_BASE(a) + 0x07)
#define AD5694R_PRESET_GAIN(a)          (AD5694R_PRESET_BASE(a) + 0x08)

// 1000 - 100F -> Sensor 1 serial number
// 1010 - 101F -> Sensor 2 serial number
// 1020 - 102F -> Sensor 3 serial number
// 1030 - 103F -> Sensor 4 serial number
#define SERIAL_NUMBER_MAXLENGTH         0x10 /* See also in CommProtocol for MAX_SERIAL_BUFLENGTH */
#define SENSOR_SERIAL_NUMBER(a)         (0x1000 + ((a) * SERIAL_NUMBER_MAXLENGTH))



// 7FF0 - Board serial number
#define BOARD_SERIAL_NUMBER             0x7FF0
        
#ifdef	__cplusplus
}
#endif

#endif	/* PERSISTENCE_H */


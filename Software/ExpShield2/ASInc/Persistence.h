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

#include "main.h"

#ifndef PERSISTENCE_H
#define	PERSISTENCE_H

#ifdef	__cplusplus
extern "C" {
#endif

// LEDS pins
#define HB_LED_ENPIN      0
#define TX_LED_ENPIN      1
#define RX_LED_ENPIN      2

// User I/O lines
#define USER_BUTTONPIN    3
#define USER_ADDR0		  4
#define USER_ADDR1		  5
#define USER_ADDR2		  6
#define USER_ADDR3		  7

// SensorBus Lines
#define SBUS_TXE		  8
#define SBUS_CS			  9

// D300 sensor Lines
#define D300_CAL1		 10
#define D300_CAL2		 11
#define D300_RESET		 12

// K96 sensor lines
#define K96_EN			 13
#define K96_RDY			 14

// FAN control lines
#define FAN_ALERT		 15


#define SPARE_1			 16
#define SPARE_2			 17

#define LAST_UNUSEDPIN    18
    
// EEPROM memory map persistence    
    
// 0000 - 0004 -> Sampler preset channel 0
// 0010 - 0014 -> Sampler preset channel 1
// 0020 - 0024 -> Sampler preset channel 2
// ...
// 00D0 - 00D4 -> Sampler preset channel D
// ...
#define SAMPLER_PRESET_BASE(a) ((a)<<4)

#define SAMPLER_PRESET_PRESCALER(a)     SAMPLER_PRESET_BASE(a)
#define SAMPLER_PRESET_DECIMATION(a)    (SAMPLER_PRESET_BASE(a) + 1)
#define SAMPLER_PRESET_IIR1DENOM(a)     (SAMPLER_PRESET_BASE(a) + 2)
#define SAMPLER_PRESET_IIR2DENOM(a)     (SAMPLER_PRESET_BASE(a) + 3)
    
// 0008 - 0008 -> Sampler averager buffer size channel 0
// 0018 - 0018 -> Sampler averager buffer size channel 1
// 0028 - 0028 -> Sampler averager buffer size channel 2
// ...
// 00D8 - 00D8 -> Sampler averager buffer size channel D
// ...
#define AVERAGER_PRESET_BUFSIZE(a)      (SAMPLER_PRESET_BASE(a) + 8)

// 1000 - 100F -> Sensor 1 serial number (D300)
// 1010 - 101F -> Sensor 2 serial number (N/A)
// 1020 - 102F -> Sensor 3 serial number (N/A)
// 1030 - 103F -> Sensor 4 serial number (N/A)
// 1040 - 104F -> Sensor 5 serial number (N/A)
#define SERIAL_NUMBER_MAXLENGTH         0x10 /* See also in CommProtocol for MAX_SERIAL_BUFLENGTH */
#define SENSOR_SERIAL_NUMBER(a)         (0x1000 + ((a) * SERIAL_NUMBER_MAXLENGTH))

// 1100 - 1100 -> Channel 0 enabled, Sampler 0
// 1101 - 1101 -> Channel 1 enabled, Sampler 0
// 1102 - 1102 -> Channel 2 enabled, Sampler 0
//...
// 1200 - 1200 -> Channel 0 enabled, Sampler 1
// a: sampler
// b: relative channel
#define SAMPLER_CHANNEL_ENABLED_PRESET(a,b)	((0x1100 + (((unsigned short)(a))<<8)) + (b))

// 2000 - 2000 -> Channel 0 setpoint, Sampler 0
// 2002 - 2002 -> Channel 1 setpoint, Sampler 0
// 2004 - 2004 -> Channel 2 setpoint, Sampler 0
// ...
// 2100 - 2100 -> Channel 0 setpoint Sampler 1
// ...
// a: sampler
// b: relative channel
#define SAMPLER_CHANNEL_SETPOINT(a,b)		((0x2000 + (((unsigned short)(a))<<8)) + ((b)<<1))

// Other constants to be persisted
#define PID_COEFFICIENTS				0x7000	/* to 0x700F */

// 7FF0 - Board serial number
#define BOARD_SERIAL_NUMBER             0x7FF0
        
#ifdef	__cplusplus
}
#endif

#endif	/* PERSISTENCE_H */


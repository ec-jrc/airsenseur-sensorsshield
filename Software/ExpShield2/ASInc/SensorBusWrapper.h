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

#ifndef SENSORBUSWRAPPER_H_
#define SENSORBUSWRAPPER_H_

#define COMMPROTOCOL_PTM_HOST_HEADER    '['
#define COMMPROTOCOL_PTM_HOST_TRAILER   ']'

#define COMMPROTOCOL_PTM_SLAVE_HEADER   '('
#define COMMPROTOCOL_PTM_SLAVE_TRAILER  ')'

#define COMMPROTOCOL_PTM_VERSION_ZERO   '0'
#define COMMPROTOCOL_PTM_VERSION_ONE	'1'

#define COMMPROTOCOL_BOARDID_BROADCAST  0xFF
#define COMMPROTOCOL_MYBOARDID_MSB      '0'

#define COMMPROTOCOL_SHIELDPAYLOAD_HEADER   '{'
#define COMMPROTOCOL_SHIELDPAYLOAD_TRAILER  '}'

#define SENSORBUS_FRAME_TIMEOUT         500	/* in 10ms steps -> 5seconds */

class CommProtocol;

class SensorBusWrapper {
public:
	SensorBusWrapper(CommProtocol* lowLayerProtocol);
	virtual ~SensorBusWrapper();

public:
	void init(unsigned char boardID);
	void timerTick(void);	// to be called each 10th seconds
	void onDataReceived(unsigned char rxChar);
	unsigned short write(char* buffer) const;

private:
	typedef enum _rxstatuses {
	    IDLE, HEADER_FOUND, VERSION_FOUND, BOARDID1_FOUND, BOARDID_FOUND,
	} rxstatuses;

    rxstatuses rxStatus;

    unsigned char myBoardId;
    unsigned char rxBoardId;
    unsigned char rxProtocolVersion;
    static char txHeader[];

    unsigned short timerCounter;
    unsigned short rxProtocolTimeout;

private:
	CommProtocol* commProtocol;
};

#endif /* SENSORBUSWRAPPER_H_ */

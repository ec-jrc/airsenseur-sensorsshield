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

#ifndef _CHEMSENSOR_BOARD_IMPL_H_
#define _CHEMSENSOR_BOARD_IMPL_H_

#ifdef __cplusplus
 extern "C" {
#endif

 	void uart1Interrupt();
 	void uart2Interrupt();
 	void uart3Interrupt();
 	void uart4Interrupt();
 	void uart1Error();
 	void uart2Error();
 	void uart3Error();
 	void uart4Error();
 	void usbRxCallback(unsigned char* buffer, long bufferLen);
	void timerInterrupt();
	void setup_impl();
	void loop_impl();

#ifdef __cplusplus
}
#endif


#endif /* _CHEMSENSOR_BOARD_IMPL_H_ */

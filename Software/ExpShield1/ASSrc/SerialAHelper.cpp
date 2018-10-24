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

#include <GlobalHalHandlers.h>
#include <string.h>

#include <SerialAHelper.h>

// Singleton SerialAHelper instance
SerialAHelper SerialAHelper::instance;
uint8_t SerialAHelper::txBuffer[SERIALABUFFERSIZE];
uint8_t SerialAHelper::rxBuffer = '\000';

SerialAHelper::SerialAHelper() {
}

SerialAHelper::~SerialAHelper() {
}

void SerialAHelper::init() const {
	HAL_UART_Receive_DMA(&huart1, &rxBuffer, 1);
}

void SerialAHelper::onDataRx() {
	putByte(rxBuffer);
}

uint16_t SerialAHelper::write(char* buffer) const {

	uint8_t len = strlen(buffer);
	return write(buffer, len);
}

uint16_t SerialAHelper::write(char* buffer, uint16_t len) const {

	len = (len > SERIALABUFFERSIZE)? SERIALABUFFERSIZE : len;
	uint32_t timeout = HAL_GetTick();
	while((huart1.gState != HAL_UART_STATE_READY) &&
			(HAL_GetTick() - timeout) < 1000) { };

	memcpy(txBuffer, buffer, len);
	HAL_UART_Transmit_DMA(&huart1, (uint8_t*)txBuffer, len);
	return len;
}


bool SerialAHelper::available() const {
	return dataReady();
}

uint8_t SerialAHelper::read() {
	return getByte();
}

void SerialAHelper::onErrorCallback() {
	HAL_UART_Receive_DMA(&huart1, &rxBuffer, 1);
}

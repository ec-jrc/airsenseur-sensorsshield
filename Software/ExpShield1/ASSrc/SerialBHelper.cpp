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

#include <SerialBHelper.h>

// Singleton SerialBHelper instance
SerialBHelper SerialBHelper::instance;
uint8_t SerialBHelper::txBuffer[SERIALBBUFFERSIZE];
uint8_t SerialBHelper::rxBuffer = '\000';

SerialBHelper::SerialBHelper() {
}

SerialBHelper::~SerialBHelper() {
}

void SerialBHelper::init() const {
	HAL_UART_Receive_DMA(&huart2, &rxBuffer, 1);
}

void SerialBHelper::onDataRx() {
	putByte(rxBuffer);
}

uint16_t SerialBHelper::write(char* buffer) const {
	uint8_t len = strlen(buffer);
	return write(buffer, len);
}

uint16_t SerialBHelper::write(char* buffer, uint16_t len) const {

	len = (len > SERIALBBUFFERSIZE)? SERIALBBUFFERSIZE : len;
	uint32_t timeout = HAL_GetTick();
	while((huart2.gState != HAL_UART_STATE_READY) &&
			(HAL_GetTick() - timeout) < 1000) { };

	memcpy(txBuffer, buffer, len);
	HAL_UART_Transmit_DMA(&huart2, (uint8_t*)txBuffer, len);
	return len;
}

bool SerialBHelper::available() const {
	return dataReady();
}

uint8_t SerialBHelper::read() {
	return getByte();
}

void SerialBHelper::onErrorCallback() {
	HAL_UART_Receive_DMA(&huart2, &rxBuffer, 1);
}


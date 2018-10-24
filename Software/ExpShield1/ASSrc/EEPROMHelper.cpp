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

#include <EEPROMHelper.h>
#include <GlobalHalHandlers.h>
#include <string.h>


#define MEM24AA256_ADDRESS 0xA0

// Singleton EEPROMHelper instance
EEPROMHelper EEPROMHelper::instance;

EEPROMHelper::EEPROMHelper() : writingItem(NULL), lastItem(NULL) , timer(0), writingTimestamp(0) {
}

EEPROMHelper::~EEPROMHelper() {
}


bool EEPROMHelper::write(unsigned short address, unsigned char value) {

	// EEPROM writes require 5ms. It's mandatory to enqueue all requests
	// and process them later
	return pushRequest(address, &value, 1);
}


bool EEPROMHelper::write(unsigned short address, unsigned char* pData, unsigned char size) {

	// No more than 64 bytes at time can be stored in a single transaction (see 24AA256 datasheet)
	if (size > 64) {
		return false;
	}

	// EEPROM writes require 5ms. It's mandatory to enqueue all requests
	// and process them later
	return pushRequest(address, pData, size);
}

unsigned char EEPROMHelper::read(unsigned short address) {

	unsigned char result;
	HAL_StatusTypeDef res = HAL_I2C_Mem_Read(&hi2c2, MEM24AA256_ADDRESS, address, 0x02, &result, 0x01, 500);

	return (res == HAL_OK)? result : 0xFF;
}

void EEPROMHelper::tick() {
	timer++;
}

void EEPROMHelper::mainLoop() {

	// Check if there is something to write on the queue
	if ((writingItem != NULL) && (((unsigned char)(timer - writingTimestamp)) > MIN_TIME_BETWEEN_WRITING)) {
		writingTimestamp = timer;

		// Start writing the new item
		datapacket* curWriting = writingItem;
		HAL_StatusTypeDef res = HAL_I2C_Mem_Write(&hi2c2, MEM24AA256_ADDRESS, curWriting->address, 0x02, curWriting->pData, curWriting->size, 500);

		// Link to next item in the queue
		writingItem = curWriting->nextItem;

		// Unlink head-tail for the very last item
		if (writingItem == NULL) {
			lastItem = NULL;
		}

		// Deallocate memory
		delete[] curWriting->pData;
		delete curWriting;
	}
}

bool EEPROMHelper::pushRequest(unsigned short address, unsigned char* pData, unsigned char size) {

	datapacket* newItem = new datapacket();
	if (newItem == NULL) {
		return false;
	}

	newItem->address = address;
	newItem->size = size;
	newItem->pData = new unsigned char[size];
	newItem->nextItem = NULL;
	if (newItem->pData == NULL) {
		delete newItem;
		return false;
	}
	memcpy(newItem->pData, pData, size);

	// enqueue the new item
	if (lastItem != NULL) {
		lastItem->nextItem = newItem;
	}
	lastItem = newItem;

	// link head-tail for the very first time
	if (writingItem == NULL) {
		writingItem = lastItem;
	}

	return true;
}

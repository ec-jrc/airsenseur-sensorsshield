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
 *      European Commission - Joint Research Centre,
 * - Marco Signorini, marco.signorini@liberaintentio.com
 *
 * ===========================================================================
 */

#include <stddef.h>
#include "IntADDevice.h"
#include "GlobalHalHandlers.h"


#define TEMP30_CAL_ADDR  ((uint16_t*) ((uint32_t)0x1FFFF7B8))
#define VREFINT_CAL      ((uint16_t*) ((uint32_t)0x1FFFF7BA))
#define VREFINT_DATA	 (adcDmaBuffer[INTAD_CHANNEL_VREFINT])
#define VDD_CALIB		 ((uint32_t) 3300)
#define AVG_SLOPE		 ((uint32_t) 5336) 		/* See A.7.16 in RM0360 STM document */

#define MAX_V_FOR_CC	 1.8
#define MAX_RAWADC_FOR_CC 	(MAX_V_FOR_CC * 4095 / VDDA) // 2233

#define VCHANNEL(ADC_DATA) ((VREFINT_DATA != 0)? ((3.3f * (*VREFINT_CAL) * (ADC_DATA)) / (VREFINT_DATA * 4095)) : 0.0f)
#define VDDA 			   ((VREFINT_DATA != 0)? (3.3f * (((float)(*VREFINT_CAL)) / VREFINT_DATA)) : 0)

#define TEMPERATURE_FROM_ADC(adcval) ((  ((float)(((uint32_t) *TEMP30_CAL_ADDR) - ((uint32_t)(adcval) * (VDDA * 1000) / VDD_CALIB)) * 1000)) / AVG_SLOPE) + 30

const char* const IntADDevice::channelNames[] {
		"PELTV", "PELTC", "PSINV", "TUCBRD"
};

const char* const IntADDevice::channelMeasurementUnits[] = {
		"mV", "mA", "mV", "C"
};

IntADDevice* const IntADDevice::instance = new IntADDevice();


IntADDevice* const IntADDevice::getInstance() {
	return IntADDevice::instance;
}

const unsigned char IntADDevice::defaultSampleRate() {
	return INTAD_SAMPLING_PERIOD;
}

IntADDevice::IntADDevice() : SensorDevice(INTAD_NUM_OF_CHANNELS),
							calibrationTimer(0),
							samplesAvailable(0),
							go(false),
							error(false) {
}

IntADDevice::~IntADDevice() {

}

void IntADDevice::onStartSampling() {
	SensorDevice::onStartSampling();

	samplesAvailable = false;
}

void IntADDevice::onStopSampling() {
}

void IntADDevice::setLowPowerMode(bool lowPower) {
}

bool IntADDevice::init() {

	// Initialize the DMA buffer with something valid, to avoid
	// wrong measurements at the very first sample
	adcDmaBuffer[INTAD_CHANNEL_PLT_VFBK] = 0;
	adcDmaBuffer[INTAD_CHANNEL_PLT_CFBK] = 0;
	adcDmaBuffer[INTAD_CHANNEL_VIN_FBK] = 0;
	adcDmaBuffer[INTAD_CHANNEL_VREFINT] = *VREFINT_CAL;
	adcDmaBuffer[INTAD_CHANNEL_TEMPERATURE] = *TEMP30_CAL_ADDR;

	if (HAL_ADCEx_Calibration_Start(&hadc) == HAL_OK) {
		return (HAL_ADC_Start_DMA(&hadc, (uint32_t*)adcDmaBuffer, ADC_DMA_BUFFER_SIZE) == HAL_OK);
	}

	return false;
}

void IntADDevice::loop() {

	// Retrieve samples from the DMA buffer and place in the base device
	if (go && !error && samplesAvailable) {
		samplesAvailable = false;
		go = false;

		setSample(INTAD_CHANNEL_PLT_VFBK, adcDmaBuffer[INTAD_CHANNEL_PLT_VFBK]);
		setSample(INTAD_CHANNEL_PLT_CFBK, adcDmaBuffer[INTAD_CHANNEL_PLT_CFBK]);
		setSample(INTAD_CHANNEL_VIN_FBK, adcDmaBuffer[INTAD_CHANNEL_VIN_FBK]);
		setSample(INTAD_CHANNEL_TEMPERATURE, adcDmaBuffer[INTAD_CHANNEL_TEMPERATURE]);
	}

	// This is true for the very first time the loop enters after power on.
	// Then it will be true each AUTOCALIBRATION_PERIOD ticks
	if (calibrationTimer >= AUTOCALIBRATION_PERIOD) {

		calibrationTimer = 0;
		error = false;

		// Stop DMA because ADC should be stop here
		HAL_ADC_Stop_DMA(&hadc);

		// Start ADC Calibration
		if (HAL_ADCEx_Calibration_Start(&hadc) == HAL_OK) {

			// Retrieve the ADC calibration value
			uint32_t cVal = HAL_ADC_GetValue(&hadc);
			UNUSED(cVal);

		} else {
			error = true;
		}

		// ADC is used by the external PID to check for power and
		// maintain the temperature.
		// It may be stopped for preventing power usage, when no
		// thermal compensation is needed, but we decided
		// to maintain always on for simplicity
		if (HAL_ADC_Start_DMA(&hadc, (uint32_t*)adcDmaBuffer, ADC_DMA_BUFFER_SIZE) != HAL_OK) {
			error = true;
		}
	}
}

void IntADDevice::tick() {

	calibrationTimer++;
}

const char* IntADDevice::getSerial() const {
	return "NA";
}

const char* IntADDevice::getChannelName(unsigned char channel) const {
	if (channel < INTAD_NUM_OF_CHANNELS) {
		return channelNames[channel];
	}

	return "";
}

bool IntADDevice::setChannelName(unsigned char channel, const char* name) {
	return false;
}

const char* IntADDevice::getMeasurementUnit(unsigned char channel) const {
	if (channel < INTAD_NUM_OF_CHANNELS) {
		return channelMeasurementUnits[channel];
	}

	return "";
}

float IntADDevice::evaluateMeasurement(unsigned char channel, float value) const {

	float result = 0.0f;

	switch (channel) {

		case INTAD_CHANNEL_PLT_VFBK:
		case INTAD_CHANNEL_VIN_FBK:
			result = VCHANNEL(value) * (13.6f / 3.6f) * 1000;
			break;

		case INTAD_CHANNEL_PLT_CFBK:
			result = VCHANNEL(value) * (1/(100 * 0.003f)) * 1000;
			break;

		case INTAD_CHANNEL_TEMPERATURE:
			result = TEMPERATURE_FROM_ADC(value);
		break;

		default:
			break;
	}

	return result;
}

void IntADDevice::triggerSample() {
	SensorDevice::triggerSample();
	go = true;
}

// Called each time DMA terminates a transaction.
// This is done 10 times each second (A/D conversion is triggered by the TMR3 timer)
// NOTE: the A/D converter sampling frequency is higher than what is shown in the
// reporting channels. This is because we use the "DC current" feedback channel to properly
// tune the PWM duty cycle in the constant current power supply for the Peltier cell.
// This function returns the strength percentage (in 1/100 units) of the
// current flowing in the constant current generator. 100% is supposed for 6A
unsigned short IntADDevice::onADCScanTerminated() {

	samplesAvailable = true;

	unsigned short acRaw = adcDmaBuffer[INTAD_CHANNEL_PLT_CFBK];
	unsigned short maxRaw = MAX_RAWADC_FOR_CC;
	unsigned short result = ((float)acRaw / maxRaw) * 10000;

	return result;
}

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

#include "PWMHelper.h"
#include "GlobalHalHandlers.h"

#define PERIOD_HTR1			60000		/* 800 Hz when no prescaler is set */
#define PERIOD_MAX_HTR1		PERIOD_HTR1

#define PERIOD_HTR2			60000		/* 800 Hz when no prescaler is set */
#define PERIOD_MAX_HTR2		PERIOD_HTR2

#define PERIOD_PLT1			(1375 - 1)	/* 35kHz when no prescaler is set */
#define PERIOD_MAX_PLT1 	PERIOD_PLT1

#define MAX_PWM_DUTY		10000


PWMHelper PWMHelper::instance;

PWMHelper::PWMHelper() {
}

PWMHelper::~PWMHelper() {
}

void PWMHelper::init() {

	// Start with duty cycle 0
	setHeater1DutyCycle(0);
	setHeater2DutyCycle(0);
	setPeltierDutyCycle(0);

	// Initialize period for each channel
	__HAL_TIM_SET_AUTORELOAD(&htim14, PERIOD_HTR1);
	HAL_TIM_PWM_Start(&htim14, TIM_CHANNEL_1);

	__HAL_TIM_SET_AUTORELOAD(&htim16, PERIOD_HTR2);
	HAL_TIM_PWM_Start(&htim16, TIM_CHANNEL_1);

	__HAL_TIM_SET_AUTORELOAD(&htim17, PERIOD_PLT1);
	HAL_TIM_PWM_Start(&htim17, TIM_CHANNEL_1);
	HAL_TIMEx_PWMN_Start(&htim17, TIM_CHANNEL_1);

	// Start timers
	HAL_TIM_Base_Start_IT(&htim14);
	HAL_TIM_Base_Start_IT(&htim16);
	HAL_TIM_Base_Start_IT(&htim17);
}


void PWMHelper::setHeater1DutyCycle(unsigned short duty) {

	duty = (duty > MAX_PWM_DUTY)? MAX_PWM_DUTY : duty;
	uint16_t pwm_value = PERIOD_MAX_HTR1 * ((float)duty/MAX_PWM_DUTY);
	__HAL_TIM_SET_COMPARE(&htim14, TIM_CHANNEL_1, pwm_value);
}

void PWMHelper::setHeater2DutyCycle(unsigned short duty) {

	duty = (duty > MAX_PWM_DUTY)? MAX_PWM_DUTY : duty;
	uint16_t pwm_value = PERIOD_MAX_HTR2 - (PERIOD_MAX_HTR2 * ((float)duty/MAX_PWM_DUTY));
	__HAL_TIM_SET_COMPARE(&htim16, TIM_CHANNEL_1, pwm_value);
}

void PWMHelper::setPeltierDutyCycle(unsigned short duty) {

	duty = (duty > MAX_PWM_DUTY)? MAX_PWM_DUTY : duty;
	uint16_t pwm_value = PERIOD_MAX_PLT1 * ((float)duty/MAX_PWM_DUTY);
	__HAL_TIM_SET_COMPARE(&htim17, TIM_CHANNEL_1, pwm_value);
}


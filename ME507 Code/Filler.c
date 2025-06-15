/*
 * Filler.c
 *
 *  Created on: Jun 5, 2025
 *      Author: longe
 */

#include "Filler.h"

const int SCREW_AR = 60000; /**< autoreload for PWM signal */
const int FLAP_AR = 60000; /**< autoreload for PWM signal */

const uint8_t LC1_SIG = 0b01111000; /**< register value to consider adc signal for load cell 1 */
const uint8_t LC2_SIG = 0b01010110; /**< register value to consider adc signal for load cell 2 */

const uint8_t LC1_REF = 0b00001010; /**< register value to consider adc reference for load cell 1 */
const uint8_t LC2_REF = 0b00001111; /**< register value to consider adc reference for load cell 1 */

const int LEFT_OFFSET = 1600; /**< a starting value of the left load cell */
const int RIGHT_OFFSET = 650; /**< a starting value of the right load cell */

const int THRESHOLD = 100; /**< maximum step change in load cell weight to filter out extraneous readings */

adc_struct_t adcStruct;

fill_struct_t fillStruct;

void init_filler(TIM_HandleTypeDef* encTim, TIM_HandleTypeDef* screwTim, TIM_HandleTypeDef* flapTim, SPI_HandleTypeDef* lcSpi) {
	fillStruct.encTim = encTim;
	fillStruct.screwTim = screwTim;
	fillStruct.flapTim = flapTim;

	adcStruct.lcSpi = lcSpi;

	fillStruct.fillState = S1_DELAY;
	fillStruct.prevFillState = S0_MAN_ADV;
	fillStruct.fillStamp = 0;
	fillStruct.enaState = 0;
	fillStruct.enaStamp = 0;
	fillStruct.runOnce = false;
	fillStruct.flapTarget = 0;
	fillStruct.flapStamp = -1;
	fillStruct.timeOutStamp = HAL_GetTick();
	fillStruct.lcErrorStamp = -1;
	fillStruct.lcErrorStamp2 = -1;

	adcStruct.adcState = S3_READ_LC2;
	adcStruct.targetWeight = 0;
	adcStruct.adcStamp = 0;
	adcStruct.lWeight = 0;
	adcStruct.rWeight = 0;
	adcStruct.prevIndex = 0;
	adcStruct.startWeight = -1;

	// turn on motors
	HAL_TIM_PWM_Start(fillStruct.screwTim, TIM_CHANNEL_1);
	HAL_TIM_PWM_Start_IT(fillStruct.flapTim, TIM_CHANNEL_1);

	// start the encoder
	HAL_TIM_Encoder_Start(fillStruct.encTim, TIM_CHANNEL_ALL);
}

void start_adc() {
	// reset the device
	  adcStruct.adcOut[0] = 0x06;
	  adcStruct.adcOut[1] = 0x00;
	  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_5, GPIO_PIN_RESET);
	  HAL_SPI_TransmitReceive(adcStruct.lcSpi, adcStruct.adcOut, adcStruct.adcIn, 2, 1000);
	  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_5, GPIO_PIN_SET);
	  HAL_Delay(100);

	  // set the status register
	  adcStruct.adcOut[0] = 0x41;
	  adcStruct.adcOut[1] = 0b00000000;
	  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_5, GPIO_PIN_RESET);
	  HAL_SPI_TransmitReceive(adcStruct.lcSpi, adcStruct.adcOut, adcStruct.adcIn, 2, 1000);
	  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_5, GPIO_PIN_SET);
	  HAL_Delay(100);

	  // set the signal lines for LC1
	  adcStruct.adcOut[0] = 0x51;
	  adcStruct.adcOut[1] = LC1_SIG;
	  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_5, GPIO_PIN_RESET);
	  HAL_SPI_TransmitReceive(adcStruct.lcSpi, adcStruct.adcOut, adcStruct.adcIn, 2, 1000);
	  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_5, GPIO_PIN_SET);
	  HAL_Delay(100);

	  // set the reference lines for LC1
	  adcStruct.adcOut[0] = 0x46;
	  adcStruct.adcOut[1] = LC1_REF;
	  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_5, GPIO_PIN_RESET);
	  HAL_SPI_TransmitReceive(adcStruct.lcSpi, adcStruct.adcOut, adcStruct.adcIn, 2, 1000);
	  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_5, GPIO_PIN_SET);
	  HAL_Delay(100);

	  // start the conversion
	  adcStruct.adcOut[0] = 0x08;
	  adcStruct.adcOut[1] = 0x00;
	  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_5, GPIO_PIN_RESET);
	  HAL_SPI_TransmitReceive(adcStruct.lcSpi, adcStruct.adcOut, adcStruct.adcIn, 2, 1000);
	  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_5, GPIO_PIN_SET);
	  HAL_Delay(100);

	  // read LC1 weight
	  adcStruct.adcOut[0] = 0x12;
	  adcStruct.adcOut[1] = 0x00;
	  adcStruct.adcOut[2] = 0x00;
	  adcStruct.adcOut[3] = 0x00;
	  adcStruct.adcOut[4] = 0x00;
	  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_5, GPIO_PIN_RESET);
	  HAL_SPI_TransmitReceive(adcStruct.lcSpi, adcStruct.adcOut, adcStruct.adcIn, 5, 1000);
	  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_5, GPIO_PIN_SET);
	  HAL_Delay(100);

	  // discard the first value and read again
	  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_5, GPIO_PIN_RESET);
	  HAL_SPI_TransmitReceive(adcStruct.lcSpi, adcStruct.adcOut, adcStruct.adcIn, 5, 1000);
	  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_5, GPIO_PIN_SET);
	  // parse weight
	  int nextWeight = ((adcStruct.adcIn[2] << 16) | (adcStruct.adcIn[3] << 8) | (adcStruct.adcIn[4]));
	  if (nextWeight > 8388608) {
		  nextWeight = nextWeight - 16777216;
	  }
	  // apply polarity and offset
	  nextWeight = -1*nextWeight - LEFT_OFFSET;
	  adcStruct.lWeight = nextWeight;
	  HAL_Delay(100);

	  // set signal line for LC2
	  adcStruct.adcOut[0] = 0x51;
	  adcStruct.adcOut[1] = LC2_SIG;
	  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_5, GPIO_PIN_RESET);
	  HAL_SPI_TransmitReceive(adcStruct.lcSpi, adcStruct.adcOut, adcStruct.adcIn, 2, 1000);
	  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_5, GPIO_PIN_SET);
	  HAL_Delay(100);

	  // set reference line for LC2
	  adcStruct.adcOut[0] = 0x46;
	  adcStruct.adcOut[1] = LC2_REF;
	  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_5, GPIO_PIN_RESET);
	  HAL_SPI_TransmitReceive(adcStruct.lcSpi, adcStruct.adcOut, adcStruct.adcIn, 2, 1000);
	  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_5, GPIO_PIN_SET);
	  HAL_Delay(100);

	  // start conversion
	  adcStruct.adcOut[0] = 0x08;
	  adcStruct.adcOut[1] = 0x00;
	  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_5, GPIO_PIN_RESET);
	  HAL_SPI_TransmitReceive(adcStruct.lcSpi, adcStruct.adcOut, adcStruct.adcIn, 2, 1000);
	  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_5, GPIO_PIN_SET);
	  HAL_Delay(100);

	  // read LC2 weight
	  adcStruct.adcOut[0] = 0x12;
	  adcStruct.adcOut[1] = 0x00;
	  adcStruct.adcOut[2] = 0x00;
	  adcStruct.adcOut[3] = 0x00;
	  adcStruct.adcOut[4] = 0x00;
	  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_5, GPIO_PIN_RESET);
	  HAL_SPI_TransmitReceive(adcStruct.lcSpi, adcStruct.adcOut, adcStruct.adcIn, 5, 1000);
	  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_5, GPIO_PIN_SET);
	  HAL_Delay(100);

	  // discard the first value and read again
	  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_5, GPIO_PIN_RESET);
	  HAL_SPI_TransmitReceive(adcStruct.lcSpi, adcStruct.adcOut, adcStruct.adcIn, 5, 1000);
	  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_5, GPIO_PIN_SET);
	  // parse weight
	  nextWeight = ((adcStruct.adcIn[2] << 16) | (adcStruct.adcIn[3] << 8) | (adcStruct.adcIn[4]));
	  if (nextWeight > 8388608) {
		  nextWeight = nextWeight - 16777216;
	  }
	  nextWeight = -1*nextWeight - RIGHT_OFFSET;
	  adcStruct.rWeight = nextWeight;


}

void run_filler() {

	if (adcStruct.adcState == S0_SET_LC1) {
		if (adcStruct.prevAdcState != adcStruct.adcState) {

			  // set signal line
			  adcStruct.adcOut[0] = 0x51;
			  adcStruct.adcOut[1] = LC1_SIG;
			  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_5, GPIO_PIN_RESET);
			  HAL_SPI_TransmitReceive(adcStruct.lcSpi, adcStruct.adcOut, adcStruct.adcIn, 2, 1000);
			  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_5, GPIO_PIN_SET);

			  // set reference line
			  adcStruct.adcOut[0] = 0x46;
			  adcStruct.adcOut[1] = LC1_REF;
			  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_5, GPIO_PIN_RESET);
			  HAL_SPI_TransmitReceive(adcStruct.lcSpi, adcStruct.adcOut, adcStruct.adcIn, 2, 1000);
			  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_5, GPIO_PIN_SET);

			  // start conversion
			  adcStruct.adcOut[0] = 0x08;
			  adcStruct.adcOut[1] = 0x00;
			  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_5, GPIO_PIN_RESET);
			  HAL_SPI_TransmitReceive(adcStruct.lcSpi, adcStruct.adcOut, adcStruct.adcIn, 2, 1000);
			  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_5, GPIO_PIN_SET);

			  adcStruct.prevAdcState = adcStruct.adcState;
			  adcStruct.adcStamp = HAL_GetTick();
		}

		if ((HAL_GetTick() - adcStruct.adcStamp) > 100) {
			adcStruct.adcState = S1_READ_LC1;
		}

	} else if (adcStruct.adcState == S1_READ_LC1) {

		if (adcStruct.prevAdcState != adcStruct.adcState) {

			  // read weight
			  adcStruct.adcOut[0] = 0x12;
			  adcStruct.adcOut[1] = 0x00;
			  adcStruct.adcOut[2] = 0x00;
			  adcStruct.adcOut[3] = 0x00;
			  adcStruct.adcOut[4] = 0x00;
			  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_5, GPIO_PIN_RESET);
			  HAL_SPI_TransmitReceive(adcStruct.lcSpi, adcStruct.adcOut, adcStruct.adcIn, 5, 1000);
			  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_5, GPIO_PIN_SET);

			  // parse weight
			  int nextWeight = ((adcStruct.adcIn[2] << 16) | (adcStruct.adcIn[3] << 8) | (adcStruct.adcIn[4]));
			  if (nextWeight > 8388608) {
				  nextWeight = nextWeight - 16777216;
			  }
			  // apply polarity and offset
			  nextWeight = -1*nextWeight - LEFT_OFFSET;
			  // apply threshold filter
			  if ((adcStruct.lWeight == 0) || (((nextWeight - adcStruct.lWeight) < THRESHOLD) && ((nextWeight - adcStruct.lWeight) > -1*THRESHOLD))) {
				  adcStruct.lWeight = nextWeight;
				  adcStruct.prevL[adcStruct.prevIndex] = nextWeight;

			  } else {
				  adcStruct.prevL[adcStruct.prevIndex] = adcStruct.lWeight;
			  }

			  adcStruct.prevAdcState = adcStruct.adcState;
			  adcStruct.adcStamp = HAL_GetTick();
		}

		if ((HAL_GetTick() - adcStruct.adcStamp) > 100) {
			adcStruct.adcState = S2_SET_LC2;
		}

	} else if (adcStruct.adcState == S2_SET_LC2) {

		if (adcStruct.prevAdcState != adcStruct.adcState) {

			  // set signal line
			  adcStruct.adcOut[0] = 0x51;
			  adcStruct.adcOut[1] = LC2_SIG;
			  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_5, GPIO_PIN_RESET);
			  HAL_SPI_TransmitReceive(adcStruct.lcSpi, adcStruct.adcOut, adcStruct.adcIn, 2, 1000);
			  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_5, GPIO_PIN_SET);

			  // set reference line
			  adcStruct.adcOut[0] = 0x46;
			  adcStruct.adcOut[1] = LC2_REF;
			  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_5, GPIO_PIN_RESET);
			  HAL_SPI_TransmitReceive(adcStruct.lcSpi, adcStruct.adcOut, adcStruct.adcIn, 2, 1000);
			  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_5, GPIO_PIN_SET);

			  // start conversion
			  adcStruct.adcOut[0] = 0x08;
			  adcStruct.adcOut[1] = 0x00;
			  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_5, GPIO_PIN_RESET);
			  HAL_SPI_TransmitReceive(adcStruct.lcSpi, adcStruct.adcOut, adcStruct.adcIn, 2, 1000);
			  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_5, GPIO_PIN_SET);

			  adcStruct.prevAdcState = adcStruct.adcState;
			  adcStruct.adcStamp = HAL_GetTick();
		}

		if ((HAL_GetTick() - adcStruct.adcStamp) > 100) {
			adcStruct.adcState = S3_READ_LC2;
		}

	} else if (adcStruct.adcState == S3_READ_LC2) {

		if (adcStruct.prevAdcState != adcStruct.adcState) {

			// read load cell 2
			  // read weight
			  adcStruct.adcOut[0] = 0x12;
			  adcStruct.adcOut[1] = 0x00;
			  adcStruct.adcOut[2] = 0x00;
			  adcStruct.adcOut[3] = 0x00;
			  adcStruct.adcOut[4] = 0x00;
			  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_5, GPIO_PIN_RESET);
			  HAL_SPI_TransmitReceive(adcStruct.lcSpi, adcStruct.adcOut, adcStruct.adcIn, 5, 1000);
			  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_5, GPIO_PIN_SET);

			  // parse weight
			  int nextWeight = ((adcStruct.adcIn[2] << 16) | (adcStruct.adcIn[3] << 8) | (adcStruct.adcIn[4]));
			  if (nextWeight > 8388608) {
				  nextWeight = nextWeight - 16777216;
			  }
			  nextWeight = -1*nextWeight - RIGHT_OFFSET;
			  // apply threshold filter
			  if ((adcStruct.rWeight == 0) || (((nextWeight - adcStruct.rWeight) < THRESHOLD) && ((nextWeight - adcStruct.rWeight) > -1*THRESHOLD))) {
				  adcStruct.rWeight = nextWeight;
				  adcStruct.prevR[adcStruct.prevIndex] = nextWeight;

			  } else {
				  adcStruct.prevR[adcStruct.prevIndex] = adcStruct.rWeight;
			  }

			  // increment weight history index
			  if (adcStruct.prevIndex >= 9) {
				  adcStruct.prevIndex = 0;
			  } else {
				  adcStruct.prevIndex++;
			  }

			  adcStruct.prevAdcState = adcStruct.adcState;
			  adcStruct.adcStamp = HAL_GetTick();
		}

		if ((HAL_GetTick() - adcStruct.adcStamp) > 100) {
			adcStruct.adcState = S0_SET_LC1;
		}
	}

	//int aveWeight = (adcStruct.lWeight + adcStruct.rWeight) / 2;
	int aveWeight = get_weight();

	// if the enable state has been toggled, record the start weight
	if (fillStruct.enaState == 0) {
		adcStruct.startWeight = -1;
	} else {
		if (adcStruct.startWeight == -1) {
			adcStruct.startWeight = aveWeight;
		}
	}

	// check if the load cell is reading a bad value
	if (fillStruct.enaState == 1) {

		// first check if the target weight is too far away for too long
		if ((adcStruct.targetWeight - aveWeight > 100) || (adcStruct.targetWeight - aveWeight < -100)) {
		  if (fillStruct.lcErrorStamp == -1) {
			  fillStruct.lcErrorStamp = HAL_GetTick();
		  } else if ((HAL_GetTick() - fillStruct.lcErrorStamp) > 5000) {
			  fillStruct.enaState = -2;
			  fillStruct.lcErrorStamp = -1;
		  }
		} else {
		  fillStruct.lcErrorStamp = -1;
		}

		// next check if the current weight is dropping too quickly
		int lastIndex = adcStruct.prevIndex+1;
		if (lastIndex > 9) {
		  lastIndex = 0;
		}
		int lastWeight = (adcStruct.prevL[lastIndex] + adcStruct.prevR[lastIndex]) / 2;
		if ((lastWeight - get_weight()) > 20) {
		  if (fillStruct.lcErrorStamp2 == -1) {
			  fillStruct.lastSafeWeight = lastWeight;
			  fillStruct.lcErrorStamp2 = HAL_GetTick();
		  }
		}

		if ((fillStruct.lcErrorStamp2 > 0) && ((HAL_GetTick() - fillStruct.lcErrorStamp2) > 2000)) {
		  if ((fillStruct.lastSafeWeight - get_weight()) > 20) {
			  fillStruct.enaState = -2;
		  }
		  fillStruct.lcErrorStamp2 = -1;
		}

	}

	// if the filling isn't working, stop and error out
	if ((HAL_GetTick() - fillStruct.timeOutStamp) > 60000) {
	  fillStruct.enaState = -1;
	  fillStruct.timeOutStamp = HAL_GetTick();
	}

	if (fillStruct.fillState == S0_MAN_ADV) { // manual screw advance
	  if (fillStruct.prevFillState != fillStruct.fillState) {
		  HAL_GPIO_WritePin(GPIOD, GPIO_PIN_0, GPIO_PIN_SET);
		  __HAL_TIM_SET_COMPARE(fillStruct.screwTim, TIM_CHANNEL_1, SCREW_AR/2);
		  fillStruct.fillStamp = HAL_GetTick();
		  fillStruct.prevFillState = fillStruct.fillState;
	  }
	  if ((HAL_GetTick() - fillStruct.fillStamp) > 100) {
		  fillStruct.fillState = S1_DELAY;
	  }
	} else if (fillStruct.fillState == S1_DELAY) { // delay 1 second
	  if (fillStruct.prevFillState != fillStruct.fillState) {
		  fillStruct.fillStamp = HAL_GetTick();
		  fillStruct.prevFillState = fillStruct.fillState;
	  }
	  if ((HAL_GetTick() - fillStruct.fillStamp) > 1000) {
		  fillStruct.fillState = S2_MEASURE;
	  }
	} else if (fillStruct.fillState == S2_MEASURE) { // check the weight
	  __HAL_TIM_SET_COMPARE(fillStruct.screwTim, TIM_CHANNEL_1, 0);
	  fillStruct.prevFillState = fillStruct.fillState;


	  if (adcStruct.targetWeight > aveWeight + 5) {
		  if (fillStruct.enaState == 1) {
			  fillStruct.fillState = S3_ADVANCE;
		  } else {
			  fillStruct.timeOutStamp = HAL_GetTick();
		  }
	  } else {

		  fillStruct.timeOutStamp = HAL_GetTick();

		  if (fillStruct.runOnce) { // if we are at weight and set to run once, disable
			  fillStruct.enaState = 0;
			  fillStruct.runOnce = false;
		  }
	  }

	} else if (fillStruct.fillState == S3_ADVANCE) { // advance the screw
	  if (fillStruct.prevFillState != fillStruct.fillState) {
		  HAL_GPIO_WritePin(GPIOD, GPIO_PIN_0, GPIO_PIN_SET);
		  __HAL_TIM_SET_COMPARE(fillStruct.screwTim, TIM_CHANNEL_1, SCREW_AR/2);
		  fillStruct.fillStamp = HAL_GetTick();
		  fillStruct.prevFillState = fillStruct.fillState;
	  }

	  if (fillStruct.enaState != 1) {
		  fillStruct.fillState = S2_MEASURE;
	  } else {

		  int delay = 4000;
		  if ((adcStruct.targetWeight - adcStruct.startWeight) > 70) {
			  delay = 2000;
		  } else if ((adcStruct.targetWeight - adcStruct.startWeight) > 30) {
			  delay = 4000;
		  }
		  if ((HAL_GetTick() - fillStruct.fillStamp) > delay) {
			  fillStruct.fillState = S4_RETRACT;
		  }

	  }
	} else if (fillStruct.fillState == S4_RETRACT) { // retract the screw
	  if (fillStruct.prevFillState != fillStruct.fillState) {
		  HAL_GPIO_WritePin(GPIOD, GPIO_PIN_0, GPIO_PIN_RESET);
		  __HAL_TIM_SET_COMPARE(fillStruct.screwTim, TIM_CHANNEL_1, SCREW_AR/2);
		  fillStruct.fillStamp = HAL_GetTick();
		  fillStruct.prevFillState = fillStruct.fillState;
	  }

	  if ((HAL_GetTick() - fillStruct.fillStamp) > 200) {
		  fillStruct.fillState = S5_FLAP;
	  }
	} else if (fillStruct.fillState == S5_FLAP) { // stop the screw, run the flap
	  if (fillStruct.prevFillState != fillStruct.fillState) {
		  HAL_GPIO_WritePin(GPIOD, GPIO_PIN_0, GPIO_PIN_SET);
		  __HAL_TIM_SET_COMPARE(fillStruct.screwTim, TIM_CHANNEL_1, 0);
		  if (fillStruct.enaState == 1) {
			  fillStruct.flapTarget = fillStruct.flapTarget + 2000;
		  }
		  fillStruct.fillStamp = HAL_GetTick();
		  fillStruct.prevFillState = fillStruct.fillState;
	  }

	  if ((HAL_GetTick() - fillStruct.fillStamp) > 1500) {
		  fillStruct.fillState = S2_MEASURE;
	  }
	}

	int flapNext = __HAL_TIM_GET_COUNTER(fillStruct.encTim); // update the flap encoder
	if (flapNext > 32768) {
	  flapNext = flapNext - 65536;
	}
	if ((flapNext - fillStruct.flapCurrent) > 32768) { // overflow detection
	  fillStruct.flapCurrent = flapNext;
	  fillStruct.flapTarget = fillStruct.flapTarget + 65536;
	} else if ((flapNext - fillStruct.flapCurrent) < -32768) { // underflow detection
	  fillStruct.flapCurrent = flapNext;
	  fillStruct.flapTarget = fillStruct.flapTarget - 65536;
	} else {
	  fillStruct.flapCurrent = flapNext;
	}

	// calculate flap speed
	if (fillStruct.flapTarget > fillStruct.flapCurrent) {
	  if (((HAL_GetTick() - fillStruct.flapStamp) < 5000) || (fillStruct.flapStamp <= 0)) {
		  HAL_GPIO_WritePin(GPIOD, GPIO_PIN_10, GPIO_PIN_RESET);
		  __HAL_TIM_SET_COMPARE(fillStruct.flapTim, TIM_CHANNEL_1, FLAP_AR/2);
	  } else {
		  while (fillStruct.flapTarget > fillStruct.flapCurrent) {
			  fillStruct.flapTarget = fillStruct.flapTarget - 2000;
		  }
		  fillStruct.flapStamp = HAL_GetTick();
	  }

	} else if (fillStruct.flapCurrent > fillStruct.flapTarget + 20) {
	  if (((HAL_GetTick() - fillStruct.flapStamp) < 5000) || (fillStruct.flapStamp <= 0)) {
		  HAL_GPIO_WritePin(GPIOD, GPIO_PIN_10, GPIO_PIN_SET);
		  __HAL_TIM_SET_COMPARE(fillStruct.flapTim, TIM_CHANNEL_1, FLAP_AR/2);
	  } else {
		  __HAL_TIM_SET_COMPARE(fillStruct.flapTim, TIM_CHANNEL_1, 0);
		  fillStruct.flapStamp = HAL_GetTick();
	  }
	} else {
	  __HAL_TIM_SET_COMPARE(fillStruct.flapTim, TIM_CHANNEL_1, 0);
	  fillStruct.flapStamp = HAL_GetTick();
	}

}


void enable_filler() {
	if ((HAL_GetTick() - fillStruct.enaStamp) > 1000) {
		if (fillStruct.enaState != 0) {
			fillStruct.enaState = 0;
		  } else {
			  fillStruct.enaState = 1;
		  }
		fillStruct.enaStamp = HAL_GetTick();
		fillStruct.timeOutStamp = HAL_GetTick();
	}
}

int get_enable_state() {
	return fillStruct.enaState;
}

void advance_screw() {
	fillStruct.fillState = S0_MAN_ADV;
}

void cycle_flap() {
	fillStruct.flapTarget = fillStruct.flapTarget + 2000;
}

int get_weight() {
	return (adcStruct.lWeight + adcStruct.rWeight) / 2;
}

int get_weight_filtered() {
	int sum = 0;
	for (int i = 0; i < 10; i++) {
		sum = sum + (adcStruct.prevL[i] + adcStruct.prevR[i]) / 2;
	}
	return (int) (sum / 10);
}

int get_target() {
	return adcStruct.targetWeight;
}

void run_once() {
	if (fillStruct.enaState == 1) {
		fillStruct.runOnce = true;
	}
}

void set_target(int newTarget) {
	adcStruct.targetWeight = newTarget;
}

fill_state_t get_fill_state() {
	return fillStruct.fillState;
}

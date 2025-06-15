/*
 * Filler.h
 *
 *  Created on: Jun 5, 2025
 *      Author: longe
 */

#include <stdbool.h>
#include <string.h>
#include <stdint.h>

#include "stm32f4xx_hal.h"
#include "stm32f4xx_hal_def.h"

#ifndef SRC_FILLER_H_
#define SRC_FILLER_H_

/**
 * @brief Initialize filler variables, start the encoder, and start the PWM generation
 * @param encTim: pointer to encoder pulse count timer handler
 * @param screwTim: pointer to the screw motor PWM control timer handler
 * @param flapTim: pointer to the flap motor PWM control timer handler
 * @param lcSpi: pointer to the load cell ADC SPI handler
 * @retval: None
 */
void init_filler(TIM_HandleTypeDef* encTim, TIM_HandleTypeDef* screwTim, TIM_HandleTypeDef* flapTim, SPI_HandleTypeDef* lcSpi);

/**
 * @brief Configures the ADC for weight measurement
 * @retval: None
 */
void start_adc();

/**
 * @brief Runs ADC FSM and the filling FSM
 * @retval: None
 */
void run_filler();

/**
 * @brief Sets the enable flag in response to user input
 * @retval: None
 */
void enable_filler();

/**
 * @brief Returns the current value of the enable flag
 * @retval: Current value of enaState in the fillStruct
 */
int get_enable_state();

/**
 * @brief Sets the filling FSM to manual screw advance state in response to user input
 * @retval: None
 */
void advance_screw();

/**
 * @brief Increments the flap target in response to user input
 * @retval: None
 */
void cycle_flap();

/**
 * @brief Returns the current measured weight by the ADC
 * @retval: Measured weight in ADC LSB's
 */
int get_weight();

/**
 * @brief Returns the current target weight in ADC LSB's
 * @retval: Current target in ADC LSB's
 */
int get_target();

/**
 * @brief Sets the next target weight in ADC LSB's
 * @param newTarget: the next target weight, in ADC LSB's with no external offset
 * @retval: None
 */
void set_target(int newTarget);

/**
 * @brief Gets the last five weight measurements averaged, in ADC LSB's
 * @retval: Filtered weight in ADC LSB's
 */
int get_weight_filtered();

/**
 * @brief Sets the flag to disable the FSM after the weight target has been reached
 * @retval: None
 */
void run_once();

/**
 * @brief List of valid states for the ADC FSM
 */
typedef enum {
	S0_SET_LC1,
	S1_READ_LC1,
	S2_SET_LC2,
	S3_READ_LC2
} adc_state_t;

/**
 * @brief Struct of variables for ADC reading
 */
typedef struct {
	SPI_HandleTypeDef* lcSpi; /**< spi connection to adc */

	// buffers for spi communication with adc
	uint8_t adcIn[100]; /**< input buffer for SPI communication with the ADC */
	uint8_t adcChar; /**< input char for SPI communication with the ADC */
	uint8_t adcOut[100]; /**< output buffer for SPI communication with the ADC */
	uint8_t adcOut2[100]; /**< secondary output buffer for SPI communication with the ADC */

	int lWeight; /**< current measured weight of left load cell */
	int rWeight; /**< current measured weight of right load cell */

	int prevL[10]; /**< time history of left load cell weight for filtering */
	int prevR[10]; /**< time history of right load cell weight for filtering */
	int prevIndex; /**< current index of the time history buffers */

	adc_state_t adcState; /**< next state of the adc reading FSM to run */
	adc_state_t prevAdcState; /**< the last state of the adc reading FSM to run */
	int adcStamp; /**< time stamp for adc FSM */

	int targetWeight; /**< target weight set by user */
	int startWeight; /**< weight at last enable */

} adc_struct_t;

/**
 * @brief List of valid states for the filling FSM
 */
typedef enum {
	S0_MAN_ADV,
	S1_DELAY,
	S2_MEASURE,
	S3_ADVANCE,
	S4_RETRACT,
	S5_FLAP
} fill_state_t;

/**
 * @brief Struct of variables for filling control
 */
typedef struct {
	TIM_HandleTypeDef* encTim; /**< timer for flap encoder */
    TIM_HandleTypeDef* screwTim; /**< timer for screw motor PWM */
    TIM_HandleTypeDef* flapTim; /**< timer for flap motor PWM */

    fill_state_t fillState; /**< the next state of the filling FSM to run */
    fill_state_t prevFillState; /**< the last state of the filling FSM to run */
	int fillStamp; /**< a time stamp for the filling FSM */

	int flapTarget; /**< target encoder position for the flap */
	int flapCurrent; /**< current encoder position of the flap */
	int flapStamp; /**< a time stamp for flap timeout */

	int enaState; /**< 0 = disabled, 1 = enabled */
	int enaStamp; /**< time stamp for debouncing enable commands */

	bool runOnce; /**< flag to indicate that device should disable after weight is reached */

	int timeOutStamp; /**< a time stamp to track if the filling action has timed out */

	int lcErrorStamp; /**< a time stamp to track if the load cell is consistently erroring */
	int lcErrorStamp2;

	int lastSafeWeight;

} fill_struct_t;

#endif /* SRC_FILLER_H_ */

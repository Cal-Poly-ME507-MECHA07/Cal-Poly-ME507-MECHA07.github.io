/*
 * Interface.h
 *
 *  Created on: Jun 5, 2025
 *      Author: longe
 */

#include <stdbool.h>
#include <string.h>
#include <stdint.h>

#include "stm32f4xx_hal.h"
#include "stm32f4xx_hal_def.h"

#include "Filler.h"


#ifndef SRC_INTERFACE_H_
#define SRC_INTERFACE_H_

/**
 * @brief Initialize all interface variables and start the callback
 * @param uart1: pointer to the ST-Link uart handler
 * @param wifi: pointer to the wifi module uart handler
 * @retval: None
 */
void init_interface(UART_HandleTypeDef* uart1, UART_HandleTypeDef* wifi, I2C_HandleTypeDef* calendar);

/**
 * @brief Runs the startup FSM to configure the wifi module
 * @retval: 0 if still running, 1 if complete
 */
int start_interface();

/**
 * @brief Checks all interface connections for commands and forwards data
 * @retval: None
 */
void run_interface();

/**
 * @brief Stores the most recent wifi data in the buffer
 * @param huart: pointer to the wifi module uart handler
 * @retval: None
 */
void wifi_interrupt(UART_HandleTypeDef *huart);

/**
 * @brief Prints a string to the ST-Link uart
 * @retval: None
 */
void print_msg(char* msg);

void set_target_time(int minutes, int hours);

int get_offset();

void set_offset(int offset);

/** @brief List of valid states for the interface initialization FSM*/
typedef enum {
	S0_DELAY,
	S1_RESET,
	S2_MODE,
	S3_NETWORK,
	S4_MUX,
	S5_SERVER,
	S6_DONE
} init_state_t;

/** @brief Struct of variables needed for the communication interfaces*/
typedef struct {
	UART_HandleTypeDef* uart1; /**< uart through the stlink*/
    UART_HandleTypeDef* wifi; /**< uart connection to esp wifi module*/
    I2C_HandleTypeDef* calendar; /**< i2c connection to calendar ic */

	int TS; /**< time stamp for the wifi initialization FSM*/
	init_state_t initState; /**< the next state of the wifi initialization FSM to run*/
	init_state_t prevInitState; /**< the last state of the wifi initialization FSM to run*/

	int alarm_minutes;
	int alarm_hours;

	// buffers and char vars for coms with uart1:
	uint8_t uartIn[100]; /**< input buffer for ST-Link uart */
	uint8_t uartChar; /**< input char for ST-Link uart */
	uint8_t uartOut[100]; /**< output buffer for ST-Link uart */

	// buffer and char vars for coms with wifi:
	uint8_t wifiIn[100]; /**< input buffer for wifi module uart */
	uint8_t wifiChar; /**< input char for wifi module uart */
	// wifiOut is larger to hold website
	uint8_t wifiOut[2048]; /**< output buffer for wifi module uart.  Size is increased to hold website data */
	uint8_t wifiOut2[2048]; /**< secondary output buffer for wifi module uart.  Size is increased to hold website data */

	int wifiIndex; /**< the index of the last char to go in the wifiIn buffer*/
	int wifiIndex2; /**< the index of the last char that has been printed out of the wifiIn buffer */

	int offset; /**< the current offset by which to modify the display weights */

	bool alarm_flag;

	bool resetFlag;

	int rel_minutes;
	int rel_hours;

} interface_struct_t;

#endif /* SRC_INTERFACE_H_ */

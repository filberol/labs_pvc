/*
 * matrix_keyboard.c
 *
 *  Created on: Dec 23, 2024
 *      Author: filberol
 */

#include "matrix_keyboard.h"

bool keyboard_is_set(uint16_t* state, uint16_t bit) {
	return (*state & bit) > 0;
}

void keyboard_set(uint16_t* state, uint16_t bit) {
	*state = (*state | bit);
}

void keyboard_reset(uint16_t* state, uint16_t bit) {
	*state = (*state & (~bit));
}

void keyboard_reset_all(uint16_t* state) {
	*state = 0;
}

HAL_StatusTypeDef keyboard_init(void) {
	HAL_StatusTypeDef status = set_default_regs(KEYBOARD_I2C_ADDR);
	if (status != HAL_OK) {
		return status;
	}
	uint8_t out_def = 0x00;
	status = write_reg(KEYBOARD_I2C_ADDR, REG_OUT, &out_def);
	return status;
}

HAL_StatusTypeDef keyboard_get_state(uint16_t* state) {
	static uint32_t pull_delay = 3;

	static uint32_t last_time = 0;
	static uint32_t minimum_delay = 20;

	static uint16_t prev_state = 0;

	keyboard_reset_all(state);

	uint16_t cur_state;
	keyboard_reset_all(&cur_state);

	while (HAL_GetTick() < last_time + minimum_delay) { }
	last_time = HAL_GetTick();

	HAL_StatusTypeDef status;
	uint8_t input_port;

	for (uint8_t i = 0; i < 4; i++) {
		uint8_t conf = ~(0x01 << i);

		status = write_reg(KEYBOARD_I2C_ADDR, REG_OUT, &conf);
		if (status != HAL_OK) {
			return status;
		}

		HAL_Delay(pull_delay);

		status = read_reg_in(KEYBOARD_I2C_ADDR, &input_port);
		if (status != HAL_OK) {
			return status;
		}

		input_port = (input_port >> 4) & 0x7;
		if (input_port != 0) {
			if ((input_port & 0x4) == 0) {
				keyboard_set(&cur_state, BUTTON_9 << i);
			}
			if ((input_port & 0x2) == 0) {
				keyboard_set(&cur_state, BUTTON_5 << i);
			}
			if ((input_port & 0x1) == 0) {
				keyboard_set(&cur_state, BUTTON_1 << i);
			}
		}

		HAL_Delay(pull_delay);
	}

	*state = cur_state & (~prev_state);
	prev_state = cur_state;

	return HAL_OK;
}

/*
 * pca9538.c
 *
 *  Created on: Dec 23, 2024
 *      Author: filberol
 */

#include "pca9538.h"

static uint8_t REG_OUT_DEFAULT = 0xFF;
static uint8_t REG_INV_DEFAULT = 0x00;
static uint8_t REG_CONF_DEFAULT = 0xF0;

HAL_StatusTypeDef read_reg(uint16_t addr, reg_t reg, uint8_t* buf) {
	HAL_StatusTypeDef status = HAL_I2C_Mem_Read(&hi2c1, addr | 1, reg, 1, buf, 1, 100);
	return status;
}

HAL_StatusTypeDef read_reg_in(uint16_t addr, uint8_t* buf) {
	HAL_StatusTypeDef status = read_reg(addr, REG_IN, buf);
	return status;
}

HAL_StatusTypeDef read_reg_conf(uint16_t addr, uint8_t* buf) {
	HAL_StatusTypeDef status = read_reg(addr, REG_CONF, buf);
	return status;
}

HAL_StatusTypeDef write_reg(uint16_t addr, reg_t reg, uint8_t* buf) {
	HAL_StatusTypeDef status = HAL_I2C_Mem_Write(&hi2c1, addr & 0xFFFE, reg, 1, buf, 1, 100);
	return status;
}

HAL_StatusTypeDef set_default_regs(uint16_t addr) {
	HAL_StatusTypeDef status;

	status = write_reg(addr, REG_OUT, &REG_OUT_DEFAULT);
	if (status != HAL_OK) {
		return status;
	}

	status = write_reg(addr, REG_INV, &REG_INV_DEFAULT);
	if (status != HAL_OK) {
		return status;
	}

	status = write_reg(addr, REG_CONF, &REG_CONF_DEFAULT);

	return status;
}

/*
 * i2c.h
 *
 *  Created on: Jan 29, 2025
 *      Author: tannerberman
 */
#ifndef I2C_H_
#define I2C_H_
#include <stm32l475xx.h>

void i2c_init();

uint8_t i2c_transaction(uint8_t address, uint8_t dir, uint8_t* data, uint8_t len);
uint16_t get_fs_xl_scale();

#endif /* I2C_H_ */


/*
 * lsm6dsl.c
 *
 *  Created on: Feb 5, 2025
 *      Author: tannerberman
 */

#include "lsm6dsl.h"

#define LSM6DSL_ADDR 0x6A  // 7-bit I2C address
#define WHO_AM_I_REG 0x0F
#define CTRL1_XL 0x10      // Control register for accelerometer
#define INT1_CTRL     0x0D  // Interrupt control register
#define OUTX_L 0x28        // X-axis low byte
#define OUTX_H 0x29        // X-axis high byte
#define OUTY_L 0x2A
#define OUTY_H 0x2B
#define OUTZ_L 0x2C
#define OUTZ_H 0x2D

void lsm6dsl_init() {
	uint8_t data[2];

	// Step 1 --> Enable accelerometer
	data[0] = CTRL1_XL;
	data[1] = 0x60;

//    if (i2c_transaction(LSM6DSL_ADDR, 0, data, 2)) {
//        printf("Error: Failed to configure CTRL1_XL\n");
//    }
    i2c_transaction(LSM6DSL_ADDR, 0, data, 2);

    // Step 2: Enable accelerometer data-ready interrupt on INT1
    data[0] = INT1_CTRL;
    data[1] = 0x01;  // 00000001 (Enable INT1_DRDY_XL)
//    if (i2c_transaction(LSM6DSL_ADDR, 0, data, 2)) {
//        printf("Error: Failed to configure INT1_CTRL\n");
//    }
    i2c_transaction(LSM6DSL_ADDR, 0, data, 2);
}

void lsm6dsl_read_xyz(int16_t* x, int16_t* y, int16_t* z) {
    uint8_t reg = OUTX_L;
    uint8_t data[6];

//
//    do {
//        if (i2c_transaction(LSM6DSL_ADDR, 0, &reg, 1)) return;
//        if (i2c_transaction(LSM6DSL_ADDR, 1, &status, 1)) return;
//    } while (!(status & 0x01));  // Keep polling until XLDA bit is set
//
    // Write register address, then read 6 bytes in one transaction
//    if (i2c_transaction(LSM6DSL_ADDR, 0, &reg, 1)) {
//        printf("Error: Failed to set register address for reading\n");
//        return;
//    }
    i2c_transaction(LSM6DSL_ADDR, 0, &reg, 1);
//    if (i2c_transaction(LSM6DSL_ADDR, 1, data, 6)) {
//        printf("Error: Failed to read accelerometer data\n");
//        return;
//    }
    i2c_transaction(LSM6DSL_ADDR, 1, data, 6);

    // Convert raw data
    *x = (int16_t)(data[1] << 8 | data[0]);
    *y = (int16_t)(data[3] << 8 | data[2]);
    *z = (int16_t)(data[5] << 8 | data[4]);


}


/*
 * i2c.c
 *
 *  Created on: Jan 29, 2025
 *      Author: tannerberman
 */

#include "i2c.h"

void i2c_init() {
	//I2C peripheral clock enabled in clock controller (4MHz)
	RCC->APB1ENR1 |= RCC_APB1ENR1_I2C2EN;

	//Now, we must configure the I2C peripheral itself before enabling it

	//1. STM32’s default pin mode is GPIO, but I2C needs to use Alternate Function Mode.
	//So we must Configure GPIO Pins (PB10 & PB11) for I2C
	RCC->AHB2ENR |= RCC_AHB2ENR_GPIOBEN; // Enable GPIOB peripheral clock

	//Now we must put our pins in alternate function mode
	GPIOB->MODER &= ~GPIO_MODER_MODE10_Msk; //Clear mode state of GPIOB pin 10
	GPIOB->MODER &= ~GPIO_MODER_MODE11_Msk; //Clear mode state of GPIOB pin 11
	GPIOB->MODER |= GPIO_MODER_MODE10_1; 	//Set 2nd bit position to 1 for pin 10 (which sets it to alternate function (which is 0b10))
	GPIOB->MODER |= GPIO_MODER_MODE11_1; 	//same as above but for pin 11

	//Now we must actually set our pins to alternate function mode AF4 (I2C2's), bits need to be 0100 for this
	GPIOB->AFR[1] &= ~GPIO_AFRH_AFSEL10_Msk; 	// Clear Alternate function State for pin 10
	GPIOB->AFR[1] &= ~GPIO_AFRH_AFSEL11_Msk; 	// Clear Alternate Function state for pin 10
	GPIOB->AFR[1] |= GPIO_AFRH_AFSEL10_2; 		//Setting AFSEL area to 0b0100, for alternate function mode 4 (AF4)
	GPIOB->AFR[1] |= GPIO_AFRH_AFSEL11_2; 		//same as above but for pin 11

	//Must make our output type Open-Drain, (good for multiple devices)
	GPIOB->OTYPER |= GPIO_OTYPER_OT10;
	GPIOB->OTYPER |= GPIO_OTYPER_OT11;
	//Looking at the schematics, their already exists external resistors, so we do not need //TODO: *ASK MOHOK*//
	// to manually enable internal pull-ups in software


	//GPIO setup is complete, now we must configure the I2C2 peripheral itself

	// "Clear PE bit in I2C_CR1"
	I2C2->CR1 &= ~I2C_CR1_PE;

	//For noise filters, default settings (analog enabled) is sufficient for standard mode.

	// We assume f_I2CCLK = 4 MHz. With no additional division, each tick = 1/4MHz = 250 ns.

	//BAUD RATE TIME EXPLANATION
	// This is SCL frequency formula for our setup(default noise filters):
	//   f_SCL = f_I2CCLK / ((PRESC + 1) * (SCLL + SCLH + 2))
	// and:
	//    PRESC is the prescaler (we set this to 0 for no division, so PRESC+1 = 1)
	//    SCLL is the software SCL low period (actual low time = (SCLL + 1) * tick)
	//    SCLH is the software SCL high period (actual high time = (SCLH + 1) * tick)
	//    We think the extra “+2” accounts for internal timing offsets in the peripheral.

	// For a target f_SCL of 30 kHz (which it seems we need), we need the total number of ticks per SCL cycle to be:
	//   Total ticks = f_I2CCLK / f_SCL = 4,000,000 / 30,000 ≈ 133.33 ticks.
	// We must choose an integer value of 133 ticks for the SCL period.

	// So we need:
	//   (SCLL + SCLH + 2) = 133  -->  SCLL + SCLH = 131
	//
	// We're choosing:
	//   SCLL = 65 (0x41 hex) and SCLH = 66 (0x42 hex)
	// then:
	//   SCLL + 1 = 66 ticks (low time)
	//   SCLH + 1 = 67 ticks (high time)
	//   Total ticks = 66 + 67 = 133
	//
	// Therefore, the actual SCL period is:
	//   133 ticks * 250 ns/tick = 33.25 µs,
	// which gives an SCL frequency of around 30.08 kHz.
	// TODO: discuss DELAYS!

//	I2C2->TIMINGR &= ~I2C_TIMINGR_PRESC_Msk;
//	I2C2->TIMINGR |= (0 << I2C_TIMINGR_PRESC_Pos);  // PRESC = 0 (no division)
//
//	I2C2->TIMINGR &= ~I2C_TIMINGR_SCLL_Msk;
//	I2C2->TIMINGR |= (0x62 << I2C_TIMINGR_SCLL_Pos); // SCLL = 98
//
//	I2C2->TIMINGR &= ~I2C_TIMINGR_SCLH_Msk;
//	I2C2->TIMINGR |= (0x63 << I2C_TIMINGR_SCLH_Pos); // SCLH = 99
//
//	I2C2->TIMINGR &= ~I2C_TIMINGR_SDADEL_Msk;
//	I2C2->TIMINGR |= (0x2 << I2C_TIMINGR_SDADEL_Pos); // SDADEL = 2 (500 ns)
//
//	I2C2->TIMINGR &= ~I2C_TIMINGR_SCLDEL_Msk;
//	I2C2->TIMINGR |= (0x4 << I2C_TIMINGR_SCLDEL_Pos); // SCLDEL = 4 (1 µs)

    // Set Timing Register for ~26 kHz
    I2C2->TIMINGR = 0; // Clear TIMINGR
    I2C2->TIMINGR |= (0x00 << I2C_TIMINGR_PRESC_Pos);  // PRESC = 0
    I2C2->TIMINGR |= (0x4B << I2C_TIMINGR_SCLL_Pos);   // SCLL = 75
    I2C2->TIMINGR |= (0x4B << I2C_TIMINGR_SCLH_Pos);   // SCLH = 75
    I2C2->TIMINGR |= (0x02 << I2C_TIMINGR_SDADEL_Pos); // SDADEL = 2 (500 ns)
    I2C2->TIMINGR |= (0x04 << I2C_TIMINGR_SCLDEL_Pos); // SCLDEL = 4 (1 µs)

	//Finally, Enable I2C2 (check if off first) //TODO: MOHAK NOTE-->should we do this?
	if ((I2C2->CR1 & I2C_CR1_PE) == 0) {
		I2C2->CR1 |= I2C_CR1_PE;
	}
}

uint8_t i2c_transaction(uint8_t address, uint8_t dir, uint8_t* data, uint8_t len) {

	// Clear all previous settings in CR2 (***ChatGPT suggested this for general organization! To me, it seems like a great change for readability!)
	// (Slave Address, Number of bytes, Write/Read Mode, Start bit, Stop bit) (Page 1294 in reference manual helps)
	I2C2->CR2 &= ~(I2C_CR2_SADD_Msk | I2C_CR2_NBYTES_Msk | I2C_CR2_RD_WRN | I2C_CR2_START | I2C_CR2_STOP);

	I2C2->CR2 |= (address << 0x1);					//THIS WAS A HUGE ISSUE, WAS SHIFTING BY 0 BEFORE!

    I2C2->CR2 |= (len << I2C_CR2_NBYTES_Pos);		// Set the number of bytes to transfer

	if (dir == 0) { // WRITING MODE!
		I2C2->CR2 &= ~I2C_CR2_RD_WRN;       		// Set to Write, which is 0
	}
	else {			// READING MODE!
		I2C2->CR2 |= I2C_CR2_RD_WRN;				// Set to Read, which is 1
	}

	// Generate Start condition
	I2C2->CR2 |= I2C_CR2_START;

	// Now we must actually send or receive data to or from the slave device (Reference Manual --> Page 1297 (master transmit) and 1301 (master reception))
	// - If we're writing, each byte is sent to the slave, ensuring the data register is ready before each write.
	// - If we're reading, each byte is read from the slave, ensuring data is available in the receive register before each read.
	// After all data is transmitted or received, a stop condition is generated to complete transaction.


	// WRITING MODE!
	if (dir == 0) {
		for (uint8_t i = 0; i < len; i++) { 				// for each bit 0 to len...
			while (1) { 		// Wait until TXIS (Transmit Interrupt Status) is set
				// NACK Check!
				if (I2C2->ISR & I2C_ISR_NACKF) {
					I2C2->ICR |= I2C_ICR_NACKCF;
					return 1;
				}
				if (I2C2->ISR & I2C_ISR_TXIS) {
					break;
				}
			}
			// Write data to the TXDR (Transmit Data Register)
			//TODO: Learn: How is it actually receiving this data?
			I2C2->TXDR = data[i];
			//do i need a while loop?
		}
		        // Wait for TC then generate STOP
		        while (!(I2C2->ISR & I2C_ISR_TC));
		        I2C2->CR2 |= I2C_CR2_STOP;
		        while (!(I2C2->ISR & I2C_ISR_STOPF));
		        I2C2->ICR |= I2C_ICR_STOPCF;

		        while (I2C2->ISR & I2C_ISR_BUSY); //TRY
		return 0;
	}
	// READING MODE!
	if (dir == 1) {
		for (uint8_t i = 0; i < len; i++) { 				// for each bit 0 to len...
			while (!(I2C2->ISR & I2C_ISR_RXNE)); 		// Wait until RXNE (Receive Data Register Not Empty) is set
            // Read data from the RXDR (Receive Data Register)
			//TODO: Learn: How are we actually receiving this data?
            data[i] = I2C2->RXDR;


		}
		// Wait for Transfer Complete (TC)
		            while (!(I2C2->ISR & I2C_ISR_TC));

		            // Generate STOP condition
		            I2C2->CR2 |= I2C_CR2_STOP;

		            //TODO: ask mohak about these bottom two
		            // do we need RESTART for multi-segment communication (e.g., register write + read)
		            // are we gonna need multi-segnment communication?

		            // Wait until STOPF (Stop Flag) is set
		            while (!(I2C2->ISR & I2C_ISR_STOPF));

		            // Clear STOPF flag
		            I2C2->ICR |= I2C_ICR_STOPCF;

		            while (I2C2->ISR & I2C_ISR_BUSY)
		                ; //TRY
		return 0;
	}

}



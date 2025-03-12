/*
 * timer.c
 *
 *  Created on: Oct 5, 2023
 *      Author: schulman
 */

#include "timer.h"

const int MAX_PSC_VALUE = 65536;

const int CHOSEN_PSC_VALUE = 7999;

void timer_init(TIM_TypeDef* timer)
{

	//Give the clock to the timer, basically for power
	RCC->APB1ENR1 |= RCC_APB1ENR1_TIM2EN;

	//Stop the timer
	timer->CR1 &= ~TIM_CR1_CEN;

	//Clear out any timer state
	timer->SR &= ~TIM_SR_UIF;

	//Reset all counter
	timer->CNT = 0;

	//Enable the timer interrupt internally
	NVIC_EnableIRQ(TIM2_IRQn);
	NVIC_SetPriority(TIM2_IRQn, 1);

	//Enable the timer DMA interrupt event register
	timer->DIER |= TIM_DIER_UIE;

	//Setting clock prescaler to a safe initial value (max)
	timer->PSC = MAX_PSC_VALUE;

	//Enable the timer
	timer->CR1 |= TIM_CR1_CEN;

}

void timer_reset(TIM_TypeDef* timer)
{
	//Reset the counter of the timer to be 0
	timer->CNT = 0;

}

void timer_set_ms(TIM_TypeDef* timer, uint16_t period_ms)
{
	// Stopping the timer before making changes
    timer->CR1 &= ~TIM_CR1_CEN;

    // Reset the counter of the timer to be 0
    timer->CNT = 0;

	//We know that the clock is 4MHz, so we can use the prescaler to divide it down to just 1KHz so that it is easier to count
	timer->PSC = CHOSEN_PSC_VALUE;

	//Setting our Auto Reload Register to the period_ms which aligns with our chosen PSC Value
	timer->ARR = period_ms - 1;

	// Restart the timer
    timer->CR1 |= TIM_CR1_CEN;

}

void low_timer_init(LPTIM_TypeDef* timer)
{
    printf("Starting low_timer_init...\n");

    // Enable the LSI clock (low-speed internal clock - 32 kHz)
    printf("Enabling LSI clock...\n");
    RCC->CSR |= RCC_CSR_LSION;

    // Wait for LSI clock to be ready
    printf("Waiting for LSI clock to be ready...\n");
    uint32_t timeout = 10000; // Prevent infinite loop
    while(!(RCC->CSR & RCC_CSR_LSIRDY) && timeout > 0) {
        timeout--;
    }

    if (timeout == 0) {
        printf("ERROR: LSI clock failed to become ready!\n");
        printf("RCC->CSR value: 0x%08X\n", RCC->CSR);
        return; // Exit if clock doesn't become ready
    }
    printf("LSI clock is ready. RCC->CSR: 0x%08X\n", RCC->CSR);

    // Enable the LPTIM1 peripheral clock
    printf("Enabling LPTIM1 peripheral clock...\n");
    RCC->APB1ENR1 |= RCC_APB1ENR1_LPTIM1EN;
    printf("RCC->APB1ENR1 value: 0x%08X\n", RCC->APB1ENR1);

    // Select LSI as the clock source for LPTIM1
    printf("Configuring LPTIM1 clock source...\n");
    RCC->CCIPR &= ~RCC_CCIPR_LPTIM1SEL; // Clear bits
    RCC->CCIPR |= RCC_CCIPR_LPTIM1SEL_1; // Set to 10 for LSI
    printf("RCC->CCIPR value: 0x%08X\n", RCC->CCIPR);

    // Disable the LPTIM before configuration
    printf("Disabling LPTIM before configuration...\n");
    timer->CR &= ~LPTIM_CR_ENABLE;
    printf("LPTIM->CR after disable: 0x%08X\n", timer->CR);

    // Configure the LPTIM
    // Set prescaler to divide by 32 (32kHz/32 = 1kHz)
    printf("Configuring LPTIM prescaler...\n");
    timer->CFGR &= ~LPTIM_CFGR_PRESC;
    timer->CFGR |= (5 << LPTIM_CFGR_PRESC_Pos); // 101 binary = div by 32
    printf("LPTIM->CFGR value: 0x%08X\n", timer->CFGR);

    // Clear any pending interrupts
    printf("Clearing pending interrupts...\n");
    timer->ICR = 0x1F; // Clear all flags
    printf("LPTIM->ICR value: 0x%08X\n", timer->ICR);

    // Set up LPTIM interrupt
    printf("Setting up LPTIM interrupt...\n");
    timer->IER |= LPTIM_IER_ARRMIE; // Enable Auto-reload match interrupt
    printf("LPTIM->IER value: 0x%08X\n", timer->IER);

    // Detailed NVIC Configuration
	printf("Configuring NVIC for LPTIM1...\n");

	// Clear any pending interrupt
	NVIC_ClearPendingIRQ(LPTIM1_IRQn);

	// Set priority (lower number = higher priority)
	NVIC_SetPriority(LPTIM1_IRQn, 0);

	// Enable the interrupt in NVIC
	NVIC_EnableIRQ(LPTIM1_IRQn);

	// Verify interrupt is enabled
	if (NVIC_GetEnableIRQ(LPTIM1_IRQn) == 1) {
		printf("LPTIM1 Interrupt is ENABLED in NVIC\n");
	} else {
		printf("ERROR: LPTIM1 Interrupt is NOT ENABLED in NVIC\n");
	}
    // Enable the LPTIM
    printf("Enabling LPTIM...\n");
    timer->CR |= LPTIM_CR_ENABLE;
    printf("LPTIM->CR after enable: 0x%08X\n", timer->CR);

    printf("low_timer_init completed successfully.\n");
}

void low_timer_reset(LPTIM_TypeDef* timer)
{
    printf("Starting low_timer_reset...\n");

    // Stop the timer first
    printf("Stopping the timer...\n");
    timer->CR &= ~LPTIM_CR_CNTSTRT;
    printf("LPTIM->CR after stop: 0x%08X\n", timer->CR);

    // Disable the timer
    printf("Disabling the timer...\n");
    timer->CR &= ~LPTIM_CR_ENABLE;
    printf("LPTIM->CR after disable: 0x%08X\n", timer->CR);

    // Clear any pending interrupts
    printf("Clearing pending interrupts...\n");
    timer->ICR = 0x1F; // Clear all flags
    printf("LPTIM->ICR value: 0x%08X\n", timer->ICR);

    // Re-enable the timer
    printf("Re-enabling the timer...\n");
    timer->CR |= LPTIM_CR_ENABLE;
    printf("LPTIM->CR after re-enable: 0x%08X\n", timer->CR);

    // Start the counter again
    printf("Starting the counter...\n");
    timer->CR |= LPTIM_CR_CNTSTRT;
    printf("LPTIM->CR after counter start: 0x%08X\n", timer->CR);

    printf("low_timer_reset completed successfully.\n");
}

void low_timer_set_ms(LPTIM_TypeDef* timer, uint16_t period_ms)
{
    printf("Starting low_timer_set_ms with period %d ms...\n", period_ms);

    // Check if timer is NULL
    if (timer == 0) {
        printf("ERROR: Timer pointer is NULL!\n");
        return;
    }

    // Disable the timer before changing settings
    printf("Disabling the timer...\n");
    timer->CR &= ~LPTIM_CR_ENABLE;
    printf("LPTIM->CR after disable: 0x%08X\n", timer->CR);

    // Clear any pending interrupts
    printf("Clearing pending interrupts...\n");
    timer->ICR = 0x1F;
    printf("LPTIM->ICR value: 0x%08X\n", timer->ICR);

    // Re-enable the timer (must be done before setting ARR)
    printf("Re-enabling the timer...\n");
    timer->CR |= LPTIM_CR_ENABLE;
    printf("LPTIM->CR after re-enable: 0x%08X\n", timer->CR);

    // Set the auto-reload value
    printf("Setting auto-reload value...\n");
    timer->ARR = period_ms;
    printf("LPTIM->ARR value: 0x%08X\n", timer->ARR);

    // Start the counter
    printf("Starting the counter...\n");
    timer->CR |= LPTIM_CR_CNTSTRT;
    printf("LPTIM->CR after counter start: 0x%08X\n", timer->CR);

    // Add a small delay to ensure configuration is complete
    for(volatile int i = 0; i < 1000; i++) {
        __NOP(); // No operation, just a small delay
    }

    printf("Timer configuration complete. CR: 0x%08X, ARR: 0x%08X, IER: 0x%08X\n",
           timer->CR, timer->ARR, timer->IER);

    printf("DETAILED REGISTER CHECK:\n");
        printf("RCC->CCIPR (Clock Source): 0x%08X\n", RCC->CCIPR);
        printf("RCC->APB1ENR1 (Peripheral Clock): 0x%08X\n", RCC->APB1ENR1);
        printf("LPTIM1->CR (Control Register): 0x%08X\n", timer->CR);
        printf("LPTIM1->IER (Interrupt Enable): 0x%08X\n", timer->IER);
        printf("LPTIM1->CFGR (Configuration): 0x%08X\n", timer->CFGR);
        printf("LPTIM1->ARR (Auto-Reload): 0x%08X\n", timer->ARR);
        printf("NVIC ISER Register: 0x%08X\n", NVIC->ISER[0]);

    printf("low_timer_set_ms completed successfully.\n");
}

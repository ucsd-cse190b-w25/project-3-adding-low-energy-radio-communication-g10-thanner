/*
 * timer.h
 *
 *  Created on: Oct 5, 2023
 *      Author: schulman
 */

#ifndef TIMER_H_
#define TIMER_H_

/* Include the type definitions for the timer peripheral */
#include <stm32l475xx.h>

void timer_init(TIM_TypeDef* timer);
void timer_reset(TIM_TypeDef* timer);
void timer_set_ms(TIM_TypeDef* timer, uint16_t period_ms);

// LPTIM functions for low-power operation
void low_timer_init(LPTIM_TypeDef* timer);
void low_timer_reset(LPTIM_TypeDef* timer);
void low_timer_set_ms(LPTIM_TypeDef* timer, uint16_t period_ms);

#endif /* TIMER_H_ */

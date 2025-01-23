/*
 * Copyright (C) OpenTX
 *
 * Based on code named
 *   th9x - http://code.google.com/p/th9x
 *   er9x - http://code.google.com/p/er9x
 *   gruvin9x - http://code.google.com/p/gruvin9x
 *
 * License GPLv2: http://www.gnu.org/licenses/gpl-2.0.html
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include "iface_a7105.h"
#include "mixer_scheduler.h"
#include "opentx.h"

void intmoduleStop() {
  TRACE("intmoduleStop");
  CLEAR_BIT(INTMODULE_TIMER->CR1, TIM_CR1_CEN);

  if (moduleState[INTERNAL_MODULE].protocol == PROTOCOL_CHANNELS_AFHDS2A_SPI)
    A7105_Sleep();
}

void intmoduleAfhds2aPulsesStart(uint16_t periodUs) {
  CLEAR_BIT(INTMODULE_TIMER->CR1, TIM_CR1_ARPE);   // Disable ARR Preload
  CLEAR_BIT(INTMODULE_TIMER->SMCR, TIM_SMCR_MSM);  // Disable Master Slave Mode
  WRITE_REG(INTMODULE_TIMER->PSC, 2);              // Prescaler
  WRITE_REG(INTMODULE_TIMER->ARR, 65535 - periodUs); // Preload, was: 61759 => 3776
  SET_BIT(INTMODULE_TIMER->DIER, TIM_DIER_UIE);    // Enable update interrupt (UIE)

  NVIC_SetPriority(INTMODULE_TIMER_IRQn, 2);
  NVIC_EnableIRQ(INTMODULE_TIMER_IRQn);
}

void initSPI1()
{
	SPI1->CR1 &= ~SPI_CR1_SPE; // SPI_DISABLE(); // SPI_2.end();

	// SPI1->CR1 &= ~SPI_CR1_DFF_8_BIT;		//8 bits format, this bit should be written only when SPI is disabled (SPE = ?0?) for correct operation.
	// SPI1->CR1 &= ~SPI_CR1_LSBFIRST;		// Set the SPI_1 bit order MSB first
	// SPI1->CR1 &= ~(SPI_CR1_CPOL|SPI_CR1_CPHA);	// Set the SPI_1 data mode 0: Clock idles low, data captured on rising edge (first transition)
	// SPI1->CR1 &= ~(SPI_CR1_BR);
	// SPI1->CR1 |= SPI_CR1_BR_1;	/*!< BaudRate control equal to fPCLK/8   */
	// SPI1->CR1 = (SPI_CR1_BIDIMODE | SPI_CR1_BIDIOE); 
  SPI1->CR1 |= ((SPI_CR1_MSTR | SPI_CR1_SSI) | SPI_CR1_SSM | SPI_CR1_BR_1);	/*!< BaudRate control equal to fPCLK/8   */

  SPI1->CR2 |= (SPI_CR2_DS_2 | SPI_CR2_DS_1 | SPI_CR2_DS_0 | SPI_CR2_FRXTH); // Data length: 8-bit + FIFO reception threshold 1/4 (8-bit)

//  CLEAR_BIT(SPI1->CR2, SPI_CR2_NSSP); /* Disable NSS pulse management */

  SPI1->CR1 |= SPI_CR1_SPE; // SPI_ENABLE(); //SPI_2.begin();								//Initialize the SPI_1 port.
}

void intmoduleAfhds2aStart() {
  TRACE("intmoduleAfhds2aStart");
  /**SPI1 GPIO Configuration
  PE13   ------> SPI1_SCK
  PE14   ------> SPI1_MISO
  PE15   ------> SPI1_MOSI
  */
  GPIOE->OSPEEDR |= (GPIO_OSPEEDR_OSPEEDR13 | GPIO_OSPEEDR_OSPEEDR14 | GPIO_OSPEEDR_OSPEEDR15);  // high output speed
  // GPIOE->OTYPER |= 0x00000000U;              // Output PUSHPULL
  // GPIOE->PUPDR |= 0x00000000U;               // PULL_NO
  GPIOE->MODER |= (GPIO_MODER_MODER13_1 | GPIO_MODER_MODER14_1 | GPIO_MODER_MODER15_1);      // Select alternate function mode
  GPIOE->AFR[1] |= ((0x0000001U << (5 * 4)) | (0x0000001U << (6 * 4)) | (0x0000001U << (7 * 4)));  // Select alternate function 1

  initSPI1();

  //RF_SCN output
  RF_SCN_GPIO_PORT->MODER |= GPIO_MODER_MODER12_0;
  //RF_RxTx output
  RF_RxTx_GPIO_PORT->MODER |= GPIO_MODER_MODER8_0 | GPIO_MODER_MODER9_0;
  //RF_RF0 output
  RF_RF0_GPIO_PORT->MODER |= GPIO_MODER_MODER10_0;
  //RF_RF1 output
  RF_RF1_GPIO_PORT->MODER |= GPIO_MODER_MODER11_0;

  // RF_GIO1
  SYSCFG->EXTICR[0] |= SYSCFG_EXTICR1_EXTI2_PB;  // Set EXTI Source
  EXTI->FTSR |= EXTI_FTSR_TR2;                   // Falling edge

  NVIC_SetPriority(EXTI2_3_IRQn, 2);
  NVIC_EnableIRQ(EXTI2_3_IRQn);

  intmoduleAfhds2aPulsesStart(3850); // was: 3776 us
  initAFHDS2A();
  SET_BIT(INTMODULE_TIMER->CR1, TIM_CR1_CEN);
}

// handler for RADIO GIO2 (FALLING AGE)
extern "C" void EXTI2_3_IRQHandler() 
{
  if (EXTI->PR & RF_GIO2_PIN) {
    WRITE_REG(EXTI->PR, RF_GIO2_PIN);
    DisableGIO();
    SETBIT(RadioState, CALLER, GPIO_CALL);
    ActionAFHDS2A();
  }
}

extern "C" void INTMODULE_TIMER_IRQHandler() 
{
  WRITE_REG(INTMODULE_TIMER->SR, ~(TIM_SR_UIF));  // Clear the update interrupt flag (UIF)
  if (setupPulsesInternalModule()) {
    SETBIT(RadioState, CALLER, TIM_CALL);
    ActionAFHDS2A();
  }
}

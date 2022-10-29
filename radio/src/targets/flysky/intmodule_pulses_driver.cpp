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

static bool internalRFEnabled = false;
/*----------------------PRT Timer----------------------------------------------*/
inline void EnablePRTTim(void) {
  SET_BIT(TIM16->CR1, TIM_CR1_CEN);
}
inline void DisablePRTTim(void) {
  CLEAR_BIT(TIM16->CR1, TIM_CR1_CEN);
}

void SetPRTTimPeriod(uint8_t prot) {
  TIM16->CNT = 0;
  switch (prot) {
    case PROTO_AFHDS2A_SPI:
      WRITE_REG(TIM16->ARR, 61759);
      break;
#if defined(AFHDS)      
    case PROTO_AFHDS_SPI:
      WRITE_REG(TIM16->ARR, 23999 /*35999*/);
      break;
#endif      
    default:
      break;
  }
};

void intmoduleStop() {
  TRACE("intmoduleStop: Stopping internal RF");
  DisablePRTTim();
  if (internalRFEnabled) {
    A7105_Sleep();
    internalRFEnabled = false;
  }
}

void intmoduleNoneStart() {
  TRACE("intmoduleNoneStart: Init internal timer no pulses");
  __IO uint32_t tmpreg;
  SET_BIT(RCC->APB2ENR, RCC_APB2ENR_TIM16EN);

  /* Delay after an RCC peripheral clock enabling */
  tmpreg = READ_BIT(RCC->APB2ENR, RCC_APB2ENR_TIM16EN);
  CLEAR_BIT(TIM16->CR1, TIM_CR1_ARPE);   // Disable ARR Preload
  CLEAR_BIT(TIM16->SMCR, TIM_SMCR_MSM);  // Disable Master Slave Mode
  WRITE_REG(TIM16->PSC, 2);              // Prescaler
  WRITE_REG(TIM16->ARR, 61759);          // Preload

  /* TIM16 interrupt Init */
  SET_BIT(TIM16->DIER, TIM_DIER_UIE);  // Enable update interrupt (UIE)
  NVIC_SetPriority(TIM16_IRQn, 2);
  NVIC_EnableIRQ(TIM16_IRQn);

  (void)tmpreg;
  internalRFEnabled = false;

  EnablePRTTim();
}

void intRFmoduleStart() {
  TRACE("intRFmoduleStart");
  // RF
  __IO uint32_t tmpreg;
  /* SPI1 clock enable */
  SET_BIT(RCC->APB2ENR, RCC_APB2ENR_SPI1EN);
  /* Delay after an RCC peripheral clock enabling */
  tmpreg = READ_BIT(RCC->APB2ENR, RCC_APB2ENR_SPI1EN);
  /**SPI1 GPIO Configuration
  PE13   ------> SPI1_SCK
  PE14   ------> SPI1_MISO
  PE15   ------> SPI1_MOSI
  */
  //PE13
  GPIOE->OSPEEDR |= GPIO_OSPEEDR_OSPEEDR13;  // high output speed
  GPIOE->OTYPER |= 0x00000000U;              // Output PUSHPULL
  GPIOE->PUPDR |= 0x00000000U;               // PULL_NO
  GPIOE->MODER |= GPIO_MODER_MODER13_1;      // Select alternate function mode
  GPIOE->AFR[1] |= (0x0000001U << (5 * 4));  // Select alternate function 1
  //PE14
  GPIOE->OSPEEDR |= GPIO_OSPEEDR_OSPEEDR14;  // high output speed
  GPIOE->OTYPER |= 0x00000000U;              // Output PUSHPULL
  GPIOE->PUPDR |= 0x00000000U;               // PULL_NO
  GPIOE->MODER |= GPIO_MODER_MODER14_1;      // Select alternate function mode
  GPIOE->AFR[1] |= (0x0000001U << (6 * 4));  // Select alternate function 1
  //PE15
  GPIOE->OSPEEDR |= GPIO_OSPEEDR_OSPEEDR15;  // high output speed
  GPIOE->OTYPER |= 0x00000000U;              // Output PUSHPULL
  GPIOE->PUPDR |= 0x00000000U;               //GPIO_PUPDR_PUPDR15_0;//                  // PULL_NO
  GPIOE->MODER |= GPIO_MODER_MODER15_1;      // Select alternate function mode
  GPIOE->AFR[1] |= (0x0000001U << (7 * 4));  // Select alternate function 1

  //  SPI1->CR1 = (SPI_CR1_BIDIMODE | SPI_CR1_BIDIOE);          /*!< Half-Duplex Tx mode. Tx transfer on 1 line */
  SPI1->CR1 |= (SPI_CR1_MSTR | SPI_CR1_SSI); /*!< Master configuration  */
  SPI1->CR1 |= 0x00000000U;                  /*!< SPI_POLARITY_LOW */
  SPI1->CR1 |= 0x00000000U;                  /*!< First clock transition is the first data capture edge  */
  SPI1->CR1 |= SPI_CR1_SSM;                  /*!< NSS managed internally. NSS pin not used and free */
  SPI1->CR1 |= SPI_CR1_BR_1;                 /*!< BaudRate control equal to fPCLK/8   */
  SPI1->CR1 |= 0x00000000U;                  /*!< Data is transmitted/received with the MSB first */
  SPI1->CR1 |= 0x00000000U;                  /*!< CRC calculation disabled */

  SPI1->CR2 |= (SPI_CR2_DS_2 | SPI_CR2_DS_1 | SPI_CR2_DS_0); /*!< Data length for SPI transfer:  8 bits */
  SPI1->CR2 |= 0x00000000U;                                  /*!< Motorola mode. Used as default value */
  SPI1->CR2 |= SPI_CR2_FRXTH;

  CLEAR_BIT(SPI1->CR2, SPI_CR2_NSSP); /* Disable NSS pulse management */
  SET_BIT(SPI1->CR1, SPI_CR1_SPE);    // SPI_ENABLE

  //RF_SCN output
  RF_SCN_GPIO_PORT->MODER |= GPIO_MODER_MODER12_0;
  //RF_RxTx output
  RF_RxTx_GPIO_PORT->MODER |= GPIO_MODER_MODER8_0 | GPIO_MODER_MODER9_0;
  //RF_RF0 output
  RF_RF0_GPIO_PORT->MODER |= GPIO_MODER_MODER10_0;
  //RF_RF1 output
  RF_RF1_GPIO_PORT->MODER |= GPIO_MODER_MODER11_0;

  /* SYSCFG clock enable */
  SET_BIT(RCC->APB2ENR, RCC_APB2ENR_SYSCFGCOMPEN);
  /* Delay after an RCC peripheral clock enabling */
  tmpreg = READ_BIT(RCC->APB2ENR, RCC_APB2ENR_SYSCFGCOMPEN);
  // RF_GIO1
  SYSCFG->EXTICR[0] |= SYSCFG_EXTICR1_EXTI2_PB;  // Set EXTI Source
  EXTI->FTSR |= EXTI_FTSR_TR2;                   // Falling edge

  NVIC_SetPriority(EXTI2_3_IRQn, 2);
  NVIC_EnableIRQ(EXTI2_3_IRQn);
  /*------------------Radio_Protocol_Timer_Init(3860 uS TIM16)------------------*/
  /* TIM16 clock enable */
  SET_BIT(RCC->APB2ENR, RCC_APB2ENR_TIM16EN);

  /* Delay after an RCC peripheral clock enabling */
  tmpreg = READ_BIT(RCC->APB2ENR, RCC_APB2ENR_TIM16EN);
  CLEAR_BIT(TIM16->CR1, TIM_CR1_ARPE);   // Disable ARR Preload
  CLEAR_BIT(TIM16->SMCR, TIM_SMCR_MSM);  // Disable Master Slave Mode
  WRITE_REG(TIM16->PSC, 2);              // Prescaler
  WRITE_REG(TIM16->ARR, 61759);          // Preload

  /* TIM16 interrupt Init */
  SET_BIT(TIM16->DIER, TIM_DIER_UIE);  // Enable update interrupt (UIE)
  NVIC_SetPriority(TIM16_IRQn, 2);
  NVIC_EnableIRQ(TIM16_IRQn);

  (void)tmpreg;
  internalRFEnabled = true;
}

void intmoduleAfhds2aStart() {
  intRFmoduleStart();
  initAFHDS2A();
  EnablePRTTim();
}
#if defined(AFHDS)
void intmoduleAfhdsStart() {
  intRFmoduleStart();
  initAFHDS();
  EnablePRTTim();
}
#endif
/*-------------handler for RADIO GIO2 (FALLING AGE)---------------------------*/
void EXTI2_3_IRQHandler(void) {
  if (EXTI->PR & RF_GIO2_PIN) {
    WRITE_REG(EXTI->PR, RF_GIO2_PIN);
    DisableGIO();
    SETBIT(RadioState, CALLER, GPIO_CALL);
    ActionAFHDS2A();
  }
}
/*------------handler for Radio_Protocol_Timer 3860 uS------------------------*/
void TIM16_IRQHandler(void) {
  WRITE_REG(TIM16->SR, ~(TIM_SR_UIF));  // Clear the update interrupt flag (UIF)
  setupPulses(INTERNAL_MODULE);
  if (internalRFEnabled) {
    //  if(g_model.moduleData[INTERNAL_MODULE].type = MODULE_TYPE_AFHDS2A_SPI)
    if (s_current_protocol[INTERNAL_MODULE] == PROTO_AFHDS2A_SPI) {
      SETBIT(RadioState, CALLER, TIM_CALL);
      ActionAFHDS2A();
    } 
#if defined(AFHDS)    
    else if (s_current_protocol[INTERNAL_MODULE] == PROTO_AFHDS_SPI) {
      ActionAFHDS();
    }
#endif    
  }
}

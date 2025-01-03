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

#include "opentx.h"

void extmoduleSendNextFrame();

//inline void EnablePPMTim(void) {
//  TRACE("EnablePPMTim");
//  SET_BIT(EXTMODULE_TIMER->CR1, TIM_CR1_CEN);
//}
//inline void DisablePPMTim(void) {
//  TRACE("DisablePPMTim");
//  CLEAR_BIT(EXTMODULE_TIMER->CR1, TIM_CR1_CEN);
//}
// inline void EnablePPMOut(void) {
//   TRACE("EnablePPMOut");
//   SET_BIT(EXTMODULE_TIMER->CCER, TIM_CCER_CC2E);
// }

inline void DisablePPMOut(void) {
  TRACE("DisablePPMOut");
  CLEAR_BIT(EXTMODULE_TIMER->CCER, TIM_CCER_CC2E);
}

void extmoduleStop() {
  TRACE("extmoduleStop");
  DisablePPMOut();
  // DisablePPMTim();
  // NVIC_DisableIRQ(EXTMODULE_TIMER_IRQn);

  EXTERNAL_MODULE_OFF();
}

void extmoduleTimerStart(uint32_t period, uint8_t state) {
  TRACE("extmoduleTimerStart");

  if (state)
    EXTERNAL_MODULE_ON();
  else
    EXTERNAL_MODULE_OFF();

  // GPIO_PinAFConfig(EXTMODULE_TX_GPIO, EXTMODULE_TX_GPIO_PinSource, 0);

  // GPIO_InitTypeDef GPIO_InitStructure;
  // GPIO_InitStructure.GPIO_Pin = EXTMODULE_TX_GPIO_PIN;
  // GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
  // GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
  // GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  // GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
  // GPIO_Init(EXTMODULE_TX_GPIO, &GPIO_InitStructure);
  // GPIO_SetBits(EXTMODULE_TX_GPIO, EXTMODULE_TX_GPIO_PIN);  // Set high

  // EXTMODULE_TIMER->CR1 &= ~TIM_CR1_CEN;
  // EXTMODULE_TIMER->PSC = EXTMODULE_TIMER_FREQ / 2000000 - 1;  // 0.5uS (2Mhz)
  // EXTMODULE_TIMER->ARR = (2000 * period);
  // EXTMODULE_TIMER->CCR2 = (2000 * period) - 1000;
  // EXTMODULE_TIMER->EGR = 1;  // Restart
  // EXTMODULE_TIMER->SR &= ~TIM_SR_CC2IF;
  // EXTMODULE_TIMER->DIER |= TIM_DIER_CC2IE;  // Enable this interrupt
  // EXTMODULE_TIMER->CR1 |= TIM_CR1_CEN;

  // NVIC_EnableIRQ(EXTMODULE_TIMER_IRQn);
  // NVIC_SetPriority(EXTMODULE_TIMER_IRQn, 7);

  EXTMODULE_TIMER->CR1 &= ~TIM_CR1_CEN;
  EXTMODULE_TIMER->PSC = EXTMODULE_TIMER_FREQ / 2000000 - 1;  // 0.5uS
  EXTMODULE_TIMER->ARR = 65535;
  EXTMODULE_TIMER->CCR2 = GET_PPM_DELAY(EXTERNAL_MODULE) * 2;
  EXTMODULE_TIMER->CCMR1 = TIM_CCMR1_OC2M_1 | TIM_CCMR1_OC2M_0;
  EXTMODULE_TIMER->BDTR |= TIM_BDTR_MOE;
  // Channel 1: PPM IN         CC1 as input  | frequency used to sample input
  EXTMODULE_TIMER->CCMR1 |= TIM_CCMR1_CC1S_0 | TIM_CCMR1_IC1F_1 | TIM_CCMR1_IC1F_0;
  EXTMODULE_TIMER->CCER |= TIM_CCER_CC1E | TIM_CCER_CC1P;  // 01: inverted/falling edge
  EXTMODULE_TIMER->EGR = 1;                                // Restart

  WRITE_REG(EXTMODULE_TIMER->SR, ~(TIM_SR_CC1IF));  // Clear capture interrupt flag (PPMIN)
  EXTMODULE_TIMER->DIER |= TIM_DIER_CC1IE;          // Enable capture interrupt     (PPMIN)

  WRITE_REG(EXTMODULE_TIMER->SR, ~(TIM_SR_CC2IF));  // Clear compare interrupt flag (PPMOUT)
  EXTMODULE_TIMER->DIER |= TIM_DIER_CC2IE;          // Enable compare interrupt     (PPMOUT)

  NVIC_EnableIRQ(EXTMODULE_TIMER_IRQn);
  NVIC_SetPriority(EXTMODULE_TIMER_IRQn, 2);

  EXTMODULE_TIMER->CR1 |= TIM_CR1_CEN; // Start counter
}

void extmodulePpmStart() {
  TRACE("extmodulePpmStart");
  /**EXTMODULE_TIMER GPIO Configuration
  PF9   ------> TIM15_CH1
  PF10   ------> TIM15_CH2
  */

  EXTERNAL_MODULE_ON();

  GPIO_PinAFConfig(EXTMODULE_TX_GPIO, EXTMODULE_TX_GPIO_PinSource, EXTMODULE_TX_GPIO_AF);

  GPIO_InitTypeDef GPIO_InitStructure;
  GPIO_InitStructure.GPIO_Pin = EXTMODULE_TX_GPIO_PIN;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
  GPIO_Init(EXTMODULE_TX_GPIO, &GPIO_InitStructure);

  EXTMODULE_TIMER->CCER |= TIM_CCER_CC2E;
}

inline void extmoduleSendNextFrame() {
  static bool delay = true;
  static uint16_t delay_halfus = GET_PPM_DELAY(EXTERNAL_MODULE) * 2;
  if (moduleState[EXTERNAL_MODULE].protocol == PROTOCOL_CHANNELS_PPM) {
    //TRACE("extmodulePulsesData.ppm: %p",(void*)&extmodulePulsesData.ppm);
    //DUMP((uint8_t*)(extmodulePulsesData.ppm.pulses), 40);
    static uint16_t *pulsePtr = extmodulePulsesData.ppm.ptr;

    if (*pulsePtr != 0) {
      if (delay) {
        EXTMODULE_TIMER->CCR2 = EXTMODULE_TIMER->CCR2 + delay_halfus;
      } else {
        //TRACE("ptr %d val %d", (uint8_t)(pulsePtr - extmodulePulsesData.ppm.pulses), *pulsePtr);
        EXTMODULE_TIMER->CCR2 = EXTMODULE_TIMER->CCR2 + *pulsePtr - delay_halfus;
        pulsePtr += 1;
      }
    } else {
      pulsePtr = extmodulePulsesData.ppm.pulses;
      // polarity 1 +
      // polarity 0 -
      EXTMODULE_TIMER->CCER = TIM_CCER_CC2E | (GET_PPM_POLARITY(EXTERNAL_MODULE) ? 0 : TIM_CCER_CC2P);
      EXTMODULE_TIMER->CCR2 = EXTMODULE_TIMER->CCR2 + delay_halfus;
      delay_halfus = GET_PPM_DELAY(EXTERNAL_MODULE) * 2;
      setupPulses(EXTERNAL_MODULE);
    }
    delay = !delay;
#if defined(CROSSFIRE)
  } else if (moduleState[EXTERNAL_MODULE].protocol == PROTOCOL_CHANNELS_CROSSFIRE) {
    if (extmodulePulsesData.crossfire.length > 0) {
      sportSendBuffer(
          extmodulePulsesData.crossfire.pulses,
          extmodulePulsesData.crossfire.length);
    }
#endif
  } else {
    EXTMODULE_TIMER->DIER |= TIM_DIER_CC2IE;
  }
}

extern "C" void EXTMODULE_TIMER_IRQHandler() {
  if (EXTMODULE_TIMER->SR & TIM_SR_CC2IF) {  // Compare PPM-OUT
    EXTMODULE_TIMER->SR &= ~TIM_SR_CC2IF;    // Clears interrupt on ch2
    if ((moduleState[EXTERNAL_MODULE].protocol == PROTOCOL_CHANNELS_CROSSFIRE && g_model.moduleData[EXTERNAL_MODULE].type != MODULE_TYPE_CROSSFIRE) ||
        (moduleState[EXTERNAL_MODULE].protocol == PROTOCOL_CHANNELS_PPM && g_model.moduleData[EXTERNAL_MODULE].type != MODULE_TYPE_PPM) ||
         moduleState[EXTERNAL_MODULE].protocol == PROTOCOL_CHANNELS_NONE ||
         moduleState[EXTERNAL_MODULE].protocol == PROTOCOL_CHANNELS_UNINITIALIZED) {
      setupPulses(EXTERNAL_MODULE);
    }
    // Only for PPM, CRSF is handled in sendSynchronousPulses
    if (moduleState[EXTERNAL_MODULE].protocol == PROTOCOL_CHANNELS_PPM) {
      extmoduleSendNextFrame();
    }
  }
  if (EXTMODULE_TIMER->SR & TIM_SR_CC1IF) {  // Capture PPM-IN
    EXTMODULE_TIMER->SR &= ~TIM_SR_CC1IF;    // Clears interrupt on ch1
    if (currentTrainerMode == TRAINER_MODE_MASTER_TRAINER_JACK) {
      captureTrainerPulses(EXTMODULE_TIMER->CCR1);
    }
  }
}

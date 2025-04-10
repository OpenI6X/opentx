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

#if defined(__cplusplus) && !defined(SIMU)
extern "C"
{
#endif
#include "usb_dcd_int.h"
#include "usb_bsp.h"
#if defined(__cplusplus) && !defined(SIMU)
}
#endif

#if defined(STM32F0) && defined(BOOT)
volatile uint32_t __attribute__((section(".ram_vector,\"aw\",%nobits @"))) ram_vector[VECTOR_TABLE_SIZE];
extern volatile uint32_t g_pfnVectors[VECTOR_TABLE_SIZE];
#endif

//audio
void buzzerInit()
{
  LL_GPIO_InitTypeDef gpio_init = {0};
  gpio_init.Pin        = BUZZER_GPIO_PIN;
  gpio_init.Mode       = LL_GPIO_MODE_ALTERNATE;
  gpio_init.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
  gpio_init.Pull       = LL_GPIO_PULL_NO;
  gpio_init.Speed      = LL_GPIO_SPEED_FREQ_LOW;
  LL_GPIO_Init(BUZZER_GPIO_PORT, &gpio_init);
}

#define __HAL_SYSCFG_REMAPMEMORY_SYSTEMFLASH()  do {SYSCFG->CFGR1 &= ~(SYSCFG_CFGR1_MEM_MODE); \
                                             SYSCFG->CFGR1 |= SYSCFG_CFGR1_MEM_MODE_0;  \
                                            }while(0)

void SystemBootloaderJump() {
    typedef void (*pFunction)(void);
    pFunction JumpToApplication;

    LL_RCC_DeInit();

    SysTick->CTRL = 0;
    SysTick->LOAD = 0;
    SysTick->VAL = 0;

    __disable_irq();

    __DSB();
    __HAL_SYSCFG_REMAPMEMORY_SYSTEMFLASH();
    __DSB();
    __ISB();

    JumpToApplication = (void (*)(void)) (*((uint32_t *) ((0x1FFFC800 + 4))));

    __set_MSP(*(__IO uint32_t*) 0x1FFFC800);

    __enable_irq();

    JumpToApplication();
}

void watchdogInit(unsigned int duration)
{
  IWDG->KR = 0x5555;    // Unlock registers
  IWDG->PR = 3;         // Divide by 32 => 1kHz clock
  IWDG->KR = 0x5555;    // Unlock registers
  IWDG->RLR = duration; // 1.5 seconds nominal
  IWDG->KR = 0xAAAA;    // reload
  IWDG->KR = 0xCCCC;    // start
}

void initBuzzerTimer()
{
  PWM_TIMER->PSC = 48 - 1; // 48MHz -> 1MHz
  PWM_TIMER->CR1 &= ~(LL_TIM_COUNTERMODE_DOWN | LL_TIM_COUNTERMODE_CENTER_UP_DOWN | TIM_CR1_CKD);
  PWM_TIMER->CR1 |= LL_TIM_COUNTERMODE_UP | LL_TIM_CLOCKDIVISION_DIV1;
  PWM_TIMER->ARR = 400; // count up to
  PWM_TIMER->CCR1 = 200; // ARR/2 = PWM duty 50%
  // PWM_TIMER->RCR = 0;
  PWM_TIMER->CCMR1 |= LL_TIM_OCMODE_PWM1;
  PWM_TIMER->CCER |= LL_TIM_OCPOLARITY_LOW | LL_TIM_CHANNEL_CH1;
  PWM_TIMER->BDTR |= TIM_BDTR_MOE;
}

void boardInit()
{
#if defined(BOOT)
  // Move vect table to beginning of RAM
  RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN;
  for (uint32_t i = 0; i < VECTOR_TABLE_SIZE; i++) {
    ram_vector[i] = g_pfnVectors[i];
  }
  SYSCFG->CFGR1 = (SYSCFG->CFGR1 & ~SYSCFG_CFGR1_MEM_MODE) | (SYSCFG_CFGR1_MEM_MODE__SRAM * SYSCFG_CFGR1_MEM_MODE_0);  // remap 0x0000000 to RAM
#endif

#if !defined(SIMU)
  LL_AHB1_GRP1_EnableClock(RCC_AHB1_GRP1_LIST);
  LL_APB1_GRP1_EnableClock(RCC_APB1_GRP1_LIST);
  LL_APB1_GRP2_EnableClock(RCC_APB1_GRP2_LIST);

  pwrInit();
  keysInit();

  if (readTrims() == BOOTLOADER_KEYS) {
    SystemBootloaderJump();
  }

#if defined(DEBUG) && defined(AUX_SERIAL_GPIO)
  auxSerialInit(UART_MODE_DEBUG, 0); // default serial mode (None if DEBUG not defined)
  // TRACE("\ni6X board started :)");
  // TRACE("RCC->CSR = %08x", RCC->CSR);
#endif

  adcInit();
//   delaysInit();
  lcdInit(); // delaysInit() must be called before
  initBuzzerTimer();
  init2MhzTimer();
  init5msTimer();
  __enable_irq();
#if defined(FLYSKY_GIMBAL)
  flysky_gimbal_init();
#endif
  buzzerInit();
  i2cInit();
  usbInit();

#if defined(DEBUG)
  LL_DBGMCU_APB1_GRP2_FreezePeriph(LL_DBGMCU_APB1_GRP2_TIM1_STOP);
  LL_DBGMCU_APB1_GRP1_FreezePeriph(
      LL_DBGMCU_APB1_GRP1_IWDG_STOP |
      LL_DBGMCU_APB1_GRP1_TIM2_STOP |
      LL_DBGMCU_APB1_GRP1_TIM3_STOP |
      LL_DBGMCU_APB1_GRP1_TIM6_STOP |
      LL_DBGMCU_APB1_GRP1_TIM14_STOP
  );
#endif

  backlightInit();

#if defined(DFPLAYER)
  dfplayerInit();
#endif

#endif // !defined(SIMU)
}

void boardOff()
{
#if !defined(PWR_BUTTON_SWITCH) // not really useful on i6X
  BACKLIGHT_DISABLE();

//#if defined(PWR_BUTTON_PRESS)
//  while (pwrPressed())
//  {
//    wdt_reset();
//  }
//#endif
  lcdOff();
  SysTick->CTRL = 0; // turn off systick
  pwrOff();

  // disable interrupts
  __disable_irq();
#endif // PWR_BUTTON_SWITCH
  // this function must not return!
}

uint16_t getBatteryVoltage()
{
  int32_t instant_vbat = anaIn(TX_VOLTAGE);
  instant_vbat = (instant_vbat * 100 * (128 + g_eeGeneral.txVoltageCalibration)) / (421 * 64);
  instant_vbat += 20; // add 0.2V because of the diode
  return (uint16_t)instant_vbat;
}

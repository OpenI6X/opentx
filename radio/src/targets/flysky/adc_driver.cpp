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

#if defined(SIMU)
// not needed
#else
const int8_t ana_direction[NUM_ANALOGS] = {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0};
#if defined (FLYSKY_GIMBAL)
const uint8_t ana_mapping[NUM_ANALOGS] =  {0, 1, 2, 3, 6, 7, 4, 5, 8, 9, 10};
#else
const uint8_t ana_mapping[NUM_ANALOGS] = {3, 2, 1, 0, 6, 7, 4, 5, 8, 9, 10};
#endif // FLYSKY_GIMBAL                   
#endif // SIMU

#if NUM_PWMANALOGS > 0
#define FIRST_ANALOG_ADC (ANALOGS_PWM_ENABLED() ? NUM_PWMANALOGS : 0)
#define NUM_ANALOGS_ADC (ANALOGS_PWM_ENABLED() ? (NUM_ANALOGS - NUM_PWMANALOGS) : NUM_ANALOGS)
#elif defined (FLYSKY_GIMBAL)
#define FIRST_ANALOG_ADC 4
#define NUM_ANALOGS_ADC (NUM_ANALOGS - 4)
#else
#define FIRST_ANALOG_ADC 0
#define NUM_ANALOGS_ADC (NUM_ANALOGS)
#endif

uint16_t adcValues[NUM_ANALOGS] __DMA;

static void adc_dma_arm(void)
{
  ADC_StartOfConversion(ADC_MAIN);
}

void adcInit()
{
  // -- init rcc --
  // ADC CLOCK = 24 / 4 = 6MHz
  RCC_ADCCLKConfig(RCC_ADCCLK_PCLK_Div2);

  // init gpio
  LL_GPIO_InitTypeDef gpio_init = {0};

  // Set up analog inputs ADC0...ADC7 (PA0...PA7)
  #if defined(FLYSKY_GIMBAL)
  gpio_init.Pin = LL_GPIO_PIN_4 | LL_GPIO_PIN_5 | LL_GPIO_PIN_6 | LL_GPIO_PIN_7;
  #else
  gpio_init.Pin = LL_GPIO_PIN_0 | LL_GPIO_PIN_1 | LL_GPIO_PIN_2 | LL_GPIO_PIN_3 |
                  LL_GPIO_PIN_4 | LL_GPIO_PIN_5 | LL_GPIO_PIN_6 | LL_GPIO_PIN_7;
  #endif

  gpio_init.Mode = LL_GPIO_MODE_ANALOG;
  gpio_init.Pull = LL_GPIO_PULL_NO;
  LL_GPIO_Init(GPIOA, &gpio_init);

  // Set up analog inputs ADC8, ADC9 (PB0, PB1)
  gpio_init.Pin = LL_GPIO_PIN_0 | LL_GPIO_PIN_1;
  LL_GPIO_Init(GPIOB, &gpio_init);

  // Battery voltage input on PC0 (ADC10)
  gpio_init.Pin = LL_GPIO_PIN_0;
  LL_GPIO_Init(GPIOC, &gpio_init);

  /* Initialize ADC structures */
  LL_ADC_InitTypeDef ADC_InitStruct = {0};
  LL_ADC_REG_InitTypeDef ADC_REG_InitStruct = {0};

  /* Configure ADC initialization structure */
  ADC_InitStruct.Resolution = LL_ADC_RESOLUTION_12B;               // 12-bit resolution
  ADC_InitStruct.DataAlignment = LL_ADC_DATA_ALIGN_RIGHT;          // Right data alignment
  ADC_InitStruct.ScanConvMode = LL_ADC_REG_SEQ_SCAN_DIR_FORWARD;   // Upward scan direction
  ADC_InitStruct.LowPowerMode = LL_ADC_LP_MODE_NONE;               // No low power mode

  /* Configure regular ADC group */
  ADC_REG_InitStruct.TriggerSource = LL_ADC_REG_TRIG_SOFTWARE;     // No external trigger
  ADC_REG_InitStruct.SequencerDiscont = LL_ADC_REG_SEQ_DISCONT_DISABLE; // No discontinuous mode
  ADC_REG_InitStruct.ContinuousMode = LL_ADC_REG_CONV_CONTINUOUS;  // Continuous conversion mode
  ADC_REG_InitStruct.DMATransfer = LL_ADC_REG_DMA_TRANSFER_UNLIMITED; // Enable DMA for ADC
  ADC_REG_InitStruct.Overrun = LL_ADC_REG_OVR_DATA_PRESERVED;      // Preserve data on overrun

  /* Initialize ADC */
  LL_ADC_Init(ADC_MAIN, &ADC_InitStruct);
  LL_ADC_REG_Init(ADC_MAIN, &ADC_REG_InitStruct);

  /* Configure ADC channels */
  LL_ADC_REG_SetSequencerChannels(ADC_MAIN,
#if !defined(FLYSKY_GIMBAL)
    LL_ADC_CHANNEL_0 | LL_ADC_CHANNEL_1 | LL_ADC_CHANNEL_2 | LL_ADC_CHANNEL_3 |
#endif
    LL_ADC_CHANNEL_4 | LL_ADC_CHANNEL_5 | LL_ADC_CHANNEL_6 | LL_ADC_CHANNEL_7 |
    LL_ADC_CHANNEL_8 | LL_ADC_CHANNEL_9 | LL_ADC_CHANNEL_10);

  LL_ADC_SetSamplingTimeCommonChannels(ADC_MAIN, ADC_SAMPTIME);

  // Enable ADC
  LL_ADC_Enable(ADC_MAIN);

  /* Wait for ADC ready */
  while (LL_ADC_IsActiveFlag_ADRDY(ADC_MAIN) == 0);

  // reset DMA channel to default values
  LL_DMA_DeInit(ADC_MAIN, ADC_DMA_Channel);

  ADC_DMA_Channel->CPAR = (uint32_t) &ADC_MAIN->DR;
  ADC_DMA_Channel->CMAR = (uint32_t)&adcValues[FIRST_ANALOG_ADC];
  ADC_DMA_Channel->CNDTR = NUM_ANALOGS;
  ADC_DMA_Channel->CCR = DMA_MemoryInc_Enable
                              | DMA_M2M_Disable
                              | DMA_Mode_Circular
                              | DMA_Priority_High
                              | DMA_DIR_PeripheralSRC
                              | DMA_PeripheralInc_Disable
                              | DMA_PeripheralDataSize_HalfWord
                              | DMA_MemoryDataSize_HalfWord;

  // enable the DMA1 - Channel1
  DMA_Cmd(ADC_DMA_Channel, ENABLE);

  // start conversion:
  adc_dma_arm();
}

void adcRead()
{
  // adc dma finished?
  if (DMA_GetITStatus(ADC_DMA_TC_FLAG))
  {

#if NUM_PWMANALOGS > 0
    if (ANALOGS_PWM_ENABLED())
    {
      analogPwmRead(adcValues);
    }
#endif
    // fine, arm DMA again:
    adc_dma_arm();
  }
}

// TODO
void adcStop()
{
}

#if !defined(SIMU)
uint16_t getAnalogValue(uint8_t index)
{
  if (IS_POT(index) && !IS_POT_SLIDER_AVAILABLE(index))
  {
    // Use fixed analog value for non-existing and/or non-connected pots.
    // Non-connected analog inputs will slightly follow the adjacent connected analog inputs,
    // which produces ghost readings on these inputs.
    return 0;
  }
  index = ana_mapping[index];
  if (ana_direction[index] < 0)
    return 4095 - adcValues[index];
  else
    return adcValues[index];
}
#if defined(FLYSKY_GIMBAL)
uint16_t* getAnalogValues()
{
  return adcValues;
}
#endif // FLYSKY_GIMBAL
#endif // #if !defined(SIMU)

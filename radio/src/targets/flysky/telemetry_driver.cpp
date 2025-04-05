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

// uint32_t telemetryErrors = 0;
DMAFifo<TELEMETRY_FIFO_SIZE> telemetryDMAFifo __DMA (TELEMETRY_DMA_Channel_RX);

#define TELEMETRY_USART_IRQ_PRIORITY 0 // was 6
#define TELEMETRY_DMA_IRQ_PRIORITY   0 // was 7

void telemetryPortInit(uint32_t baudrate, uint8_t mode) {
  TRACE("telemetryPortInit %d", baudrate);

  if (baudrate == 0) {
    LL_USART_DeInit(TELEMETRY_USART);
    return;
  }

  NVIC_SetPriority(TELEMETRY_DMA_TX_IRQn, 1);
  NVIC_EnableIRQ(TELEMETRY_DMA_TX_IRQn);

  LL_GPIO_InitTypeDef GPIO_InitStruct = {0};

  GPIO_InitStruct.Pin        = TELEMETRY_TX_GPIO_PIN;
  GPIO_InitStruct.Mode       = LL_GPIO_MODE_ALTERNATE;
  GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
#if defined(CRSF_FULLDUPLEX)
  GPIO_InitStruct.Pull       = LL_GPIO_PULL_UP;
#else
  GPIO_InitStruct.Pull       = LL_GPIO_PULL_DOWN;
#endif
  GPIO_InitStruct.Speed      = LL_GPIO_SPEED_FREQ_HIGH;
  LL_GPIO_Init(TELEMETRY_GPIO, &GPIO_InitStruct);

#if defined(CRSF_FULLDUPLEX)
  GPIO_InitStruct.Pin        = TELEMETRY_RX_GPIO_PIN;
  LL_GPIO_Init(TELEMETRY_RX_GPIO, &GPIO_InitStruct);
#endif

  LL_USART_DeInit(TELEMETRY_USART);

  // IDLE
//  LL_USART_EnableIT_IDLE(TELEMETRY_USART);

  LL_USART_InitTypeDef USART_InitStruct = {0};
  USART_InitStruct.BaudRate = baudrate;
  USART_InitStruct.DataWidth = LL_USART_DATAWIDTH_8B;
  USART_InitStruct.StopBits = LL_USART_STOPBITS_1;
  USART_InitStruct.Parity = LL_USART_PARITY_NONE;
  USART_InitStruct.TransferDirection = LL_USART_DIRECTION_TX_RX;
  USART_InitStruct.HardwareFlowControl = LL_USART_HWCONTROL_NONE;
  USART_InitStruct.OverSampling = LL_USART_OVERSAMPLING_8;

  LL_USART_Init(TELEMETRY_USART, &USART_InitStruct);
  LL_USART_Enable(TELEMETRY_USART);

#if !defined(CRSF_FULLDUPLEX)
  LL_USART_SetTXPinLevel(TELEMETRY_USART, LL_USART_TXPIN_LEVEL_INVERTED);
  LL_USART_SetRXPinLevel(TELEMETRY_USART, LL_USART_RXPIN_LEVEL_INVERTED);
#endif

  LL_DMA_DisableChannel(DMA1, TELEMETRY_DMA_Channel_RX_CH);
  LL_USART_DisableDMAReq_RX(TELEMETRY_USART);
  LL_DMA_DeInit(DMA1, TELEMETRY_DMA_Channel_RX_CH);

  telemetryDMAFifo.channel = TELEMETRY_DMA_Channel_RX; // workaround, CNDTR reading do not work otherwise
  telemetryDMAFifo.clear();

  LL_USART_DisableIT_RXNE(TELEMETRY_USART);
  LL_USART_DisableIT_TXE(TELEMETRY_USART);
  NVIC_SetPriority(TELEMETRY_USART_IRQn, TELEMETRY_USART_IRQ_PRIORITY);
  NVIC_EnableIRQ(TELEMETRY_USART_IRQn);

  TELEMETRY_DMA_Channel_RX->CPAR = (uint32_t) &TELEMETRY_USART->RDR;
  TELEMETRY_DMA_Channel_RX->CMAR = (uint32_t) telemetryDMAFifo.buffer();
  TELEMETRY_DMA_Channel_RX->CNDTR = telemetryDMAFifo.size();
  TELEMETRY_DMA_Channel_RX->CCR = LL_DMA_MEMORY_INCREMENT
                                | LL_DMA_MODE_CIRCULAR
                                | LL_DMA_PRIORITY_LOW
                                | LL_DMA_DIRECTION_PERIPH_TO_MEMORY
                                | LL_DMA_PERIPH_NOINCREMENT
                                | LL_DMA_PDATAALIGN_BYTE
                                | LL_DMA_MDATAALIGN_BYTE;

#if !defined(CRSF_FULLDUPLEX)
  LL_USART_EnableHalfDuplex(TELEMETRY_USART);
#endif
  LL_USART_EnableDMAReq_RX(TELEMETRY_USART);

  LL_USART_Enable(TELEMETRY_USART);
  LL_DMA_EnableChannel(DMA1, TELEMETRY_DMA_Channel_RX_CH);
}

void telemetryPortSetDirectionOutput() {
  // Disable RX
#if !defined(CRSF_FULLDUPLEX)
  TELEMETRY_DMA_Channel_RX->CCR &= ~DMA_CCR_EN;
  TELEMETRY_USART->CR1 &= ~(USART_CR1_RE /* | USART_CR1_IDLEIE*/);
#endif

  // Enable TX
  TELEMETRY_USART->CR1 |= USART_CR1_TE;
}

void telemetryPortSetDirectionInput() {
  // Disable TX
#if !defined(CRSF_FULLDUPLEX)
  TELEMETRY_DMA_Channel_TX->CCR &= ~DMA_CCR_EN;
  TELEMETRY_USART->CR1 &= ~USART_CR1_TE;
#endif

  // Enable RX
  TELEMETRY_USART->CR1 |= (USART_CR1_RE/* | USART_CR1_IDLEIE*/);
  TELEMETRY_DMA_Channel_RX->CCR |= DMA_CCR_EN;
}

void sportSendBuffer(const uint8_t* buffer, uint32_t count) {
  telemetryPortSetDirectionOutput();

  LL_DMA_DisableChannel(DMA1, TELEMETRY_DMA_Channel_TX_CH);
  LL_DMA_DeInit(DMA1, TELEMETRY_DMA_Channel_TX_CH);

  TELEMETRY_DMA_Channel_TX->CPAR = (uint32_t) &TELEMETRY_USART->TDR;
  TELEMETRY_DMA_Channel_TX->CMAR = (uint32_t) buffer;
  TELEMETRY_DMA_Channel_TX->CNDTR = count;
  TELEMETRY_DMA_Channel_TX->CCR = LL_DMA_MEMORY_INCREMENT
                                | LL_DMA_MODE_NORMAL
                                | LL_DMA_PRIORITY_VERYHIGH
                                | LL_DMA_DIRECTION_MEMORY_TO_PERIPH
                                | LL_DMA_PERIPH_NOINCREMENT
                                | LL_DMA_PDATAALIGN_BYTE
                                | LL_DMA_MDATAALIGN_BYTE;

  LL_DMA_EnableChannel(DMA1, TELEMETRY_DMA_Channel_TX_CH);
  LL_USART_EnableDMAReq_TX(TELEMETRY_USART);
  LL_DMA_EnableIT_TC(DMA1, TELEMETRY_DMA_Channel_TX_CH);

  // enable interrupt and set it's priority
  NVIC_EnableIRQ(TELEMETRY_DMA_TX_IRQn);
  NVIC_SetPriority(TELEMETRY_DMA_TX_IRQn, TELEMETRY_DMA_IRQ_PRIORITY);
}

extern "C" void TELEMETRY_DMA_TX_IRQHandler(void) {
  DEBUG_INTERRUPT(INT_TELEM_DMA);
  if (LL_DMA_IsActiveFlag_TC4(DMA1)) {
    LL_DMA_ClearFlag_TC4(DMA1);
    // clear TC flag before enabling interrupt
    TELEMETRY_USART->ISR &= ~USART_ISR_TC;
    TELEMETRY_USART->CR1 |= USART_CR1_TCIE;
  }
}

extern "C" void TELEMETRY_USART_IRQHandler(void) {
  DEBUG_INTERRUPT(INT_TELEM_USART);
  uint32_t status = TELEMETRY_USART->ISR;

  // TX, transfer complete
  if ((status & USART_ISR_TC) && (TELEMETRY_USART->CR1 & USART_CR1_TCIE)) {
    TELEMETRY_USART->CR1 &= ~USART_CR1_TCIE;
    telemetryPortSetDirectionInput();
    while (status & (USART_FLAG_RXNE)) {
      status = TELEMETRY_USART->RDR;
      status = TELEMETRY_USART->ISR;
    }
  }

  // RX, handled by DMA

  // TODO IDLE disabled, it is always triggering, spamming ISR
//  if ((TELEMETRY_USART->CR1 & USART_CR1_IDLEIE) && (status & USART_ISR_IDLE)) {
//    TRACE_NOCRLF("x");
//     pendingTelemetryPollFrame = true;
//    TELEMETRY_USART->ICR = USART_ICR_IDLECF;
//  }
}

// TODO we should have telemetry in an higher layer, functions above should move to a sport_driver.cpp
uint8_t telemetryGetByte(uint8_t* byte) {
  return telemetryDMAFifo.pop(*byte);
}

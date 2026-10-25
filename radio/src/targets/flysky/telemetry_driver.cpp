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

#define TELEMETRY_USART_IRQ_PRIORITY 0

// Receiver timeout in bit times after the stop bit of the last received byte
// (~1 characters at 8N1, baud-rate independent). Used as "frame received".
#define TELEMETRY_RX_TIMEOUT_BITS    10

void telemetryPortInit(uint32_t baudrate, uint8_t mode) {
  TRACE("telemetryPortInit %d", baudrate);

  NVIC_SetPriority(TELEMETRY_DMA_TX_IRQn, 1);
  NVIC_EnableIRQ(TELEMETRY_DMA_TX_IRQn);

  LL_GPIO_InitTypeDef GPIO_InitStruct; // = {0};

  GPIO_InitStruct.Pin        = TELEMETRY_TX_GPIO_PIN;
  GPIO_InitStruct.Mode       = LL_GPIO_MODE_ALTERNATE;
  GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
  GPIO_InitStruct.Alternate  = TELEMETRY_TX_GPIO_AF;
  GPIO_InitStruct.Pull       = g_eeGeneral.crsfFullDuplex ? LL_GPIO_PULL_UP : LL_GPIO_PULL_DOWN;
  GPIO_InitStruct.Speed      = LL_GPIO_SPEED_FREQ_HIGH;
  LL_GPIO_Init(TELEMETRY_GPIO, &GPIO_InitStruct);

  if (g_eeGeneral.crsfFullDuplex) {
    GPIO_InitStruct.Pin        = TELEMETRY_RX_GPIO_PIN;
    GPIO_InitStruct.Alternate  = TELEMETRY_RX_GPIO_AF;
    LL_GPIO_Init(TELEMETRY_RX_GPIO, &GPIO_InitStruct);
  }

  LL_USART_DeInit(TELEMETRY_USART);

  LL_USART_InitTypeDef USART_InitStruct; // = {0};
  USART_InitStruct.BaudRate = baudrate;
  USART_InitStruct.DataWidth = LL_USART_DATAWIDTH_8B;
  USART_InitStruct.StopBits = LL_USART_STOPBITS_1;
  USART_InitStruct.Parity = LL_USART_PARITY_NONE;
  USART_InitStruct.TransferDirection = LL_USART_DIRECTION_TX_RX;
  USART_InitStruct.HardwareFlowControl = LL_USART_HWCONTROL_NONE;
  USART_InitStruct.OverSampling = LL_USART_OVERSAMPLING_8;
  LL_USART_Init(TELEMETRY_USART, &USART_InitStruct);

  // Receiver timeout (RTO) calls telemetryFrameTrigger_ISR() at the end of every RX
  // burst. Unlike IDLE, the RTO counter only starts after a received stop bit,
  // so re-enabling the receiver at the half-duplex turnaround (which sets IDLE
  // on STM32F0) does not raise it: works the same in half and full duplex.
  // Configured while UE=0 (LL_USART_Init above leaves the USART disabled).
  LL_USART_SetRxTimeout(TELEMETRY_USART, TELEMETRY_RX_TIMEOUT_BITS);
  LL_USART_EnableRxTimeout(TELEMETRY_USART);
  LL_USART_ClearFlag_RTO(TELEMETRY_USART);
  LL_USART_EnableIT_RTO(TELEMETRY_USART);

  LL_USART_SetTXPinLevel(TELEMETRY_USART, ((g_eeGeneral.crsfFullDuplex) ? LL_USART_TXPIN_LEVEL_STANDARD : LL_USART_TXPIN_LEVEL_INVERTED));
  LL_USART_SetRXPinLevel(TELEMETRY_USART, ((g_eeGeneral.crsfFullDuplex) ? LL_USART_TXPIN_LEVEL_STANDARD : LL_USART_RXPIN_LEVEL_INVERTED));

  LL_DMA_DisableChannel(DMA1, TELEMETRY_DMA_Channel_RX_CH); // TODO done by DMA_DeInit?
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

  if (g_eeGeneral.crsfFullDuplex) {
    TELEMETRY_USART->CR3 |= USART_CR3_DMAR;
  } else {
    TELEMETRY_USART->CR3 |= USART_CR3_DMAR | USART_CR3_HDSEL /*Half duplex*/;
  }

  LL_USART_Enable(TELEMETRY_USART);
  LL_DMA_EnableChannel(DMA1, TELEMETRY_DMA_Channel_RX_CH);
}

void telemetryPortSetDirectionOutput() {
  // Disable RX
  if (!g_eeGeneral.crsfFullDuplex) {
    TELEMETRY_DMA_Channel_RX->CCR &= ~DMA_CCR_EN;
    TELEMETRY_USART->CR1 &= ~USART_CR1_RE;
  }

  // Enable TX
  TELEMETRY_USART->CR1 |= USART_CR1_TE;
}

void telemetryPortSetDirectionInput() {
  // Disable TX
  if (!g_eeGeneral.crsfFullDuplex) {
    TELEMETRY_DMA_Channel_TX->CCR &= ~DMA_CCR_EN;
    TELEMETRY_USART->CR1 &= ~USART_CR1_TE;
  }

  // Enable RX
  TELEMETRY_USART->CR1 |= USART_CR1_RE;
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
}

extern "C" void TELEMETRY_DMA_TX_IRQHandler(void) {
  DEBUG_INTERRUPT(INT_TELEM_DMA);
  if (LL_DMA_IsActiveFlag_TC4(DMA1)) {
    LL_DMA_ClearFlag_TC4(DMA1);
    // clear TC flag before enabling interrupt
    TELEMETRY_USART->ICR = USART_ICR_TCCF;
    TELEMETRY_USART->CR1 |= USART_CR1_TCIE;
  }
}

extern "C" void TELEMETRY_USART_IRQHandler(void) {
  DEBUG_INTERRUPT(INT_TELEM_USART);
  uint32_t status = TELEMETRY_USART->ISR;

  // RX, end of burst (receiver timeout); handled before the TX turnaround
  if (status & USART_ISR_RTOF) {
    TELEMETRY_USART->ICR = USART_ICR_RTOCF;
    TELEMETRY_STATS_INC(telemetryStatsRto);
    telemetryFrameTrigger_ISR();
  }

  // TX, transfer complete
  if ((status & USART_ISR_TC) && (TELEMETRY_USART->CR1 & USART_CR1_TCIE)) {
    // disable TC IRQ
    TELEMETRY_USART->CR1 &= ~USART_CR1_TCIE;

    telemetryPortSetDirectionInput();

    while (TELEMETRY_USART->ISR & USART_ISR_RXNE) {
      (void)TELEMETRY_USART->RDR;
    }
  }

  // RX data is moved by DMA.
}

// TODO we should have telemetry in an higher layer, functions above should move to a sport_driver.cpp
uint8_t telemetryGetByte(uint8_t* byte) {
  return telemetryDMAFifo.pop(*byte);
}

// EdgeTX copyRxBuffer(): move up to len received bytes to buf, returns the count
uint8_t telemetryCopyRxBuffer(uint8_t* buf, uint8_t len) {
  uint8_t n = 0;
  while (n < len && telemetryDMAFifo.pop(buf[n])) {
    n++;
  }
  return n;
}

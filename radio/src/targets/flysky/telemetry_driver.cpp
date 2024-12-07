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
    USART_DeInit(TELEMETRY_USART);
    return;
  }

  NVIC_InitTypeDef NVIC_InitStructure;
  NVIC_InitStructure.NVIC_IRQChannel = TELEMETRY_DMA_TX_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPriority = 1;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);

  GPIO_InitTypeDef GPIO_InitStructure;
  USART_InitTypeDef USART_InitStructure;

  GPIO_InitStructure.GPIO_Pin = TELEMETRY_TX_GPIO_PIN;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
#if defined(CRSF_FULLDUPLEX)
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
#else
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_DOWN;
#endif
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(TELEMETRY_GPIO, &GPIO_InitStructure);

#if defined(CRSF_FULLDUPLEX)
  GPIO_InitStructure.GPIO_Pin = TELEMETRY_RX_GPIO_PIN;
  // GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
  // GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  // GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
  // GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(TELEMETRY_RX_GPIO, &GPIO_InitStructure);
  GPIO_PinAFConfig(TELEMETRY_RX_GPIO, TELEMETRY_GPIO_PinSource_RX, TELEMETRY_RX_GPIO_AF);
#endif

  USART_DeInit(TELEMETRY_USART);

  // OverSampling + IDLE
  TELEMETRY_USART->CR1 |= ( USART_CR1_OVER8 | USART_CR1_IDLEIE );

  GPIO_PinAFConfig(TELEMETRY_GPIO, TELEMETRY_GPIO_PinSource_TX, TELEMETRY_TX_GPIO_AF);

  USART_InitStructure.USART_BaudRate = baudrate;
  USART_InitStructure.USART_WordLength = USART_WordLength_8b;
  USART_InitStructure.USART_StopBits = USART_StopBits_1;
  USART_InitStructure.USART_Parity = USART_Parity_No;
  USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
  USART_InitStructure.USART_Mode = USART_Mode_Tx | USART_Mode_Rx;
  USART_Init(TELEMETRY_USART, &USART_InitStructure);

#if !defined(CRSF_FULLDUPLEX)
  USART_InvPinCmd(TELEMETRY_USART, USART_InvPin_Tx | USART_InvPin_Rx, ENABLE);
#endif

  DMA_Cmd(TELEMETRY_DMA_Channel_RX, DISABLE);
  USART_DMACmd(TELEMETRY_USART, USART_DMAReq_Rx, DISABLE);
  DMA_DeInit(TELEMETRY_DMA_Channel_RX);

  telemetryDMAFifo.stream = TELEMETRY_DMA_Channel_RX; // workaround, CNDTR reading do not work otherwise
  telemetryDMAFifo.clear();

  USART_ITConfig(TELEMETRY_USART, USART_IT_RXNE, DISABLE);
  USART_ITConfig(TELEMETRY_USART, USART_IT_TXE, DISABLE);
  NVIC_SetPriority(TELEMETRY_USART_IRQn, TELEMETRY_USART_IRQ_PRIORITY);
  NVIC_EnableIRQ(TELEMETRY_USART_IRQn);

  TELEMETRY_DMA_Channel_RX->CPAR = (uint32_t) &TELEMETRY_USART->RDR;
  TELEMETRY_DMA_Channel_RX->CMAR = (uint32_t) telemetryDMAFifo.buffer();
  TELEMETRY_DMA_Channel_RX->CNDTR = telemetryDMAFifo.size();
  TELEMETRY_DMA_Channel_RX->CCR = DMA_MemoryInc_Enable
                                | DMA_M2M_Disable
                                | DMA_Mode_Circular
                                | DMA_Priority_Low
                                | DMA_DIR_PeripheralSRC
                                | DMA_PeripheralInc_Disable
                                | DMA_PeripheralDataSize_Byte
                                | DMA_MemoryDataSize_Byte;

#if defined(CRSF_FULLDUPLEX)
  TELEMETRY_USART->CR3 |= USART_DMAReq_Rx;
#else
  TELEMETRY_USART->CR3 |= USART_CR3_HDSEL /*Half duplex*/ | USART_DMAReq_Rx;
#endif

  USART_Cmd(TELEMETRY_USART, ENABLE);
  DMA_Cmd(TELEMETRY_DMA_Channel_RX, ENABLE);
}

void telemetryPortSetDirectionOutput() {
  // Disable RX
#if !defined(CRSF_FULLDUPLEX)
  TELEMETRY_DMA_Channel_RX->CCR &= ~DMA_CCR_EN;
  TELEMETRY_USART->CR1 &= ~USART_CR1_RE;
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
  TELEMETRY_USART->CR1 |= USART_CR1_RE;
  TELEMETRY_DMA_Channel_RX->CCR |= DMA_CCR_EN;
}

void sportSendBuffer(const uint8_t* buffer, unsigned long count) {
  telemetryPortSetDirectionOutput();

  DMA_DeInit(TELEMETRY_DMA_Channel_TX);

  TELEMETRY_DMA_Channel_TX->CPAR = (uint32_t) &TELEMETRY_USART->TDR;
  TELEMETRY_DMA_Channel_TX->CMAR = (uint32_t) buffer;
  TELEMETRY_DMA_Channel_TX->CNDTR = count;
  TELEMETRY_DMA_Channel_TX->CCR = DMA_MemoryInc_Enable
                                | DMA_M2M_Disable
                                | DMA_Mode_Normal
                                | DMA_Priority_VeryHigh
                                | DMA_DIR_PeripheralDST
                                | DMA_PeripheralInc_Disable
                                | DMA_PeripheralDataSize_Byte
                                | DMA_MemoryDataSize_Byte;

  DMA_Cmd(TELEMETRY_DMA_Channel_TX, ENABLE);
  USART_DMACmd(TELEMETRY_USART, USART_DMAReq_Tx, ENABLE);
  DMA_ITConfig(TELEMETRY_DMA_Channel_TX, DMA_IT_TC, ENABLE);

  // enable interrupt and set it's priority
  NVIC_EnableIRQ(TELEMETRY_DMA_TX_IRQn);
  NVIC_SetPriority(TELEMETRY_DMA_TX_IRQn, TELEMETRY_DMA_IRQ_PRIORITY);
}

extern "C" void TELEMETRY_DMA_TX_IRQHandler(void) {
  DEBUG_INTERRUPT(INT_TELEM_DMA);
  if (DMA_GetITStatus(TELEMETRY_DMA_TX_FLAG_TC)) {
    DMA_ClearITPendingBit(TELEMETRY_DMA_TX_FLAG_TC);
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

  // IDLE
  if (status & USART_FLAG_IDLE) {
    TELEMETRY_USART->ICR = USART_ICR_IDLECF;
    pendingTelemetryPollFrame = true;
    TELEMETRY_USART->CR1 |= USART_CR1_IDLEIE;
  }
}

// TODO we should have telemetry in an higher layer, functions above should move to a sport_driver.cpp
uint8_t telemetryGetByte(uint8_t* byte) {
  return telemetryDMAFifo.pop(*byte);
}

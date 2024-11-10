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
#if defined(CRSF_UNINVERTED)
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
#else
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_DOWN;  // was GPIO_PuPd_UP;
#endif
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(TELEMETRY_GPIO, &GPIO_InitStructure);

  USART_DeInit(TELEMETRY_USART);

  USART_OverSampling8Cmd(TELEMETRY_USART, ENABLE);

  GPIO_PinAFConfig(TELEMETRY_GPIO, TELEMETRY_GPIO_PinSource_TX, TELEMETRY_GPIO_AF);

  USART_InitStructure.USART_BaudRate = baudrate;
  USART_InitStructure.USART_WordLength = USART_WordLength_8b;
  USART_InitStructure.USART_StopBits = USART_StopBits_1;
  USART_InitStructure.USART_Parity = USART_Parity_No;
  USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
  USART_InitStructure.USART_Mode = USART_Mode_Tx | USART_Mode_Rx;
  USART_Init(TELEMETRY_USART, &USART_InitStructure);

  TELEMETRY_USART->CR1 |= USART_CR1_IDLEIE;

  // Half duplex
  USART_HalfDuplexCmd(TELEMETRY_USART, ENABLE);

  // Level inversion
#if !defined(CRSF_UNINVERTED)
  USART_InvPinCmd(TELEMETRY_USART, USART_InvPin_Tx | USART_InvPin_Rx, ENABLE);
#endif

  DMA_Cmd(TELEMETRY_DMA_Channel_RX, DISABLE);
  USART_DMACmd(TELEMETRY_USART, USART_DMAReq_Rx, DISABLE);
  DMA_DeInit(TELEMETRY_DMA_Channel_RX);

  telemetryDMAFifo.stream = TELEMETRY_DMA_Channel_RX; // workaround, CNDTR reading do not work otherwise
  telemetryDMAFifo.clear();

  USART_ITConfig(TELEMETRY_USART, USART_IT_RXNE, DISABLE);
  USART_ITConfig(TELEMETRY_USART, USART_IT_TXE, DISABLE);
  NVIC_SetPriority(TELEMETRY_USART_IRQn, 6);
  NVIC_EnableIRQ(TELEMETRY_USART_IRQn);

  // RX DMA
  DMA_InitTypeDef DMA_InitStructure;
  DMA_InitStructure.DMA_PeripheralBaseAddr = CONVERT_PTR_UINT(&TELEMETRY_USART->RDR);
  DMA_InitStructure.DMA_Priority = DMA_Priority_Low;
  DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;
  DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
  DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
  DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
  DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
  DMA_InitStructure.DMA_MemoryBaseAddr = CONVERT_PTR_UINT(telemetryDMAFifo.buffer());
  DMA_InitStructure.DMA_BufferSize = telemetryDMAFifo.size();
  DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralSRC;
  DMA_InitStructure.DMA_Mode = DMA_Mode_Circular;

  DMA_Init(TELEMETRY_DMA_Channel_RX, &DMA_InitStructure);
  USART_DMACmd(TELEMETRY_USART, USART_DMAReq_Rx, ENABLE);
  USART_Cmd(TELEMETRY_USART, ENABLE);
  DMA_Cmd(TELEMETRY_DMA_Channel_RX, ENABLE);
}

void telemetryPortSetDirectionOutput() {
  // Disable RX
  TELEMETRY_DMA_Channel_RX->CCR &= ~DMA_CCR_EN;
  TELEMETRY_USART->CR1 &= ~USART_CR1_RE;  // disable receive

  // Enable TX
  TELEMETRY_USART->CR1 |= USART_CR1_TE;   // enable transmit
}

void telemetryPortSetDirectionInput() {
  // Disable TX
  TELEMETRY_DMA_Channel_TX->CCR &= ~DMA_CCR_EN;
  TELEMETRY_USART->CR1 &= ~USART_CR1_TE;  // disable transmit

  // Enable RX
  TELEMETRY_USART->CR1 |= USART_CR1_RE;   // enable receive
  TELEMETRY_DMA_Channel_RX->CCR |= DMA_CCR_EN;
}

void sportSendBuffer(const uint8_t* buffer, unsigned long count) {
  telemetryPortSetDirectionOutput();

  DMA_InitTypeDef DMA_InitStructure;
  DMA_DeInit(TELEMETRY_DMA_Channel_TX);

  DMA_InitStructure.DMA_PeripheralBaseAddr = CONVERT_PTR_UINT(&TELEMETRY_USART->TDR);
  DMA_InitStructure.DMA_Priority = DMA_Priority_VeryHigh;  
  DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;
  DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
  DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
  DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
  DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
  DMA_InitStructure.DMA_BufferSize = count;
  DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralDST;
  DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;
  DMA_InitStructure.DMA_MemoryBaseAddr = CONVERT_PTR_UINT(buffer);

  DMA_Init(TELEMETRY_DMA_Channel_TX, &DMA_InitStructure);
  DMA_Cmd(TELEMETRY_DMA_Channel_TX, ENABLE);
  USART_DMACmd(TELEMETRY_USART, USART_DMAReq_Tx, ENABLE);
  DMA_ITConfig(TELEMETRY_DMA_Channel_TX, DMA_IT_TC, ENABLE);
  USART_ClearITPendingBit(TELEMETRY_USART, USART_IT_TC);

  // enable interrupt and set it's priority
  NVIC_EnableIRQ(TELEMETRY_DMA_TX_IRQn);
  NVIC_SetPriority(TELEMETRY_DMA_TX_IRQn, 7);
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

  // TX
  if ((status & USART_FLAG_TC) && (TELEMETRY_USART->CR1 & USART_CR1_TCIE)) {
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
#if defined(AUX_SERIAL) && !defined(PCBI6X)
  if (telemetryProtocol == PROTOCOL_FRSKY_D_SECONDARY) {
    if (auxSerialMode == UART_MODE_TELEMETRY)
      return auxSerialRxFifo.pop(*byte);
    else
      return false;
  } else {
    return telemetryDMAFifo.pop(*byte);
  }
#else
  return telemetryDMAFifo.pop(*byte);
#endif
}

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

#if defined(AUX_SERIAL)
uint8_t auxSerialMode = UART_MODE_COUNT;  // Prevent debug output before port is setup
#if defined(PCBI6X)
#if defined(DEBUG)
Fifo<uint8_t, 256> auxSerialTxFifo;
#elif defined(FLYSKY_GIMBAL)
Fifo<uint8_t, 64> auxSerialTxFifo;
#else
Fifo<uint8_t, 128> auxSerialTxFifo;
#endif
DMAFifo<32> auxSerialRxFifo __DMA (AUX_SERIAL_DMA_Channel_RX);
#else
Fifo<uint8_t, 512> auxSerialTxFifo;
DMAFifo<32> auxSerialRxFifo __DMA (AUX_SERIAL_DMA_Stream_RX);
#endif

void auxSerialSetup(unsigned int baudrate, bool dma, uint16_t lenght = USART_WordLength_8b, uint16_t parity = USART_Parity_No, uint16_t stop = USART_StopBits_1)
{
  USART_InitTypeDef USART_InitStructure;
  GPIO_InitTypeDef GPIO_InitStructure;

  GPIO_PinAFConfig(AUX_SERIAL_GPIO, AUX_SERIAL_GPIO_PinSource_TX | AUX_SERIAL_GPIO_PinSource_RX, AUX_SERIAL_GPIO_AF);
  // GPIO_PinAFConfig(AUX_SERIAL_GPIO, AUX_SERIAL_GPIO_PinSource_TX, AUX_SERIAL_GPIO_AF);

  GPIO_InitStructure.GPIO_Pin = AUX_SERIAL_GPIO_PIN_TX | AUX_SERIAL_GPIO_PIN_RX;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
  GPIO_Init(AUX_SERIAL_GPIO, &GPIO_InitStructure);

  USART_InitStructure.USART_BaudRate = baudrate;
  USART_InitStructure.USART_WordLength = lenght;
  USART_InitStructure.USART_StopBits = stop;
  USART_InitStructure.USART_Parity = parity;
  USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
  USART_InitStructure.USART_Mode = USART_Mode_Tx | USART_Mode_Rx;
  USART_Init(AUX_SERIAL_USART, &USART_InitStructure);

  if (dma) {
    auxSerialRxFifo.stream = AUX_SERIAL_DMA_Channel_RX; // workaround, CNDTR reading do not work otherwise
    DMA_InitTypeDef DMA_InitStructure;
    auxSerialRxFifo.clear();
    USART_ITConfig(AUX_SERIAL_USART, USART_IT_RXNE, DISABLE);
    USART_ITConfig(AUX_SERIAL_USART, USART_IT_TXE, DISABLE);
#if defined(STM32F0)
    DMA_InitStructure.DMA_PeripheralBaseAddr = CONVERT_PTR_UINT(&AUX_SERIAL_USART->RDR);
    DMA_InitStructure.DMA_MemoryBaseAddr = CONVERT_PTR_UINT(auxSerialRxFifo.buffer());
    DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralSRC;
    DMA_InitStructure.DMA_BufferSize = auxSerialRxFifo.size();
    DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
    DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
    DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
    DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
    DMA_InitStructure.DMA_Mode = DMA_Mode_Circular;
    DMA_InitStructure.DMA_Priority = DMA_Priority_Low;
    DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;
    DMA_Init(AUX_SERIAL_DMA_Channel_RX, &DMA_InitStructure);
    USART_InvPinCmd(AUX_SERIAL_USART, USART_InvPin_Rx, ENABLE); // Only for SBUS!
    USART_DMACmd(AUX_SERIAL_USART, USART_DMAReq_Rx, ENABLE);
    USART_Cmd(AUX_SERIAL_USART, ENABLE);
    DMA_Cmd(AUX_SERIAL_DMA_Channel_RX, ENABLE);
#else
    DMA_InitStructure.DMA_Channel = AUX_SERIAL_DMA_Channel_RX;
    DMA_InitStructure.DMA_PeripheralBaseAddr = CONVERT_PTR_UINT(&AUX_SERIAL_USART->DR);
    DMA_InitStructure.DMA_Memory0BaseAddr = CONVERT_PTR_UINT(auxSerialRxFifo.buffer());
    DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralToMemory;
    DMA_InitStructure.DMA_BufferSize = auxSerialRxFifo.size();
    DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
    DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
    DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
    DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
    DMA_InitStructure.DMA_Mode = DMA_Mode_Circular;
    DMA_InitStructure.DMA_Priority = DMA_Priority_Low;
    DMA_InitStructure.DMA_FIFOMode = DMA_FIFOMode_Disable;
    DMA_InitStructure.DMA_FIFOThreshold = DMA_FIFOThreshold_Full;
    DMA_InitStructure.DMA_MemoryBurst = DMA_MemoryBurst_Single;
    DMA_InitStructure.DMA_PeripheralBurst = DMA_PeripheralBurst_Single;
    DMA_Init(AUX_SERIAL_DMA_Stream_RX, &DMA_InitStructure);
    USART_DMACmd(AUX_SERIAL_USART, USART_DMAReq_Rx, ENABLE);
    USART_Cmd(AUX_SERIAL_USART, ENABLE);
    DMA_Cmd(AUX_SERIAL_DMA_Stream_RX, ENABLE);
#endif // STM32F0
  }
  else {
    USART_Cmd(AUX_SERIAL_USART, ENABLE);
#if !defined(PCBI6X)
    USART_ITConfig(AUX_SERIAL_USART, USART_IT_RXNE, ENABLE);
#endif
    USART_ITConfig(AUX_SERIAL_USART, USART_IT_TXE, DISABLE);
    NVIC_SetPriority(AUX_SERIAL_USART_IRQn, 7);
    NVIC_EnableIRQ(AUX_SERIAL_USART_IRQn);
  }
}

void auxSerialInit(unsigned int mode, unsigned int protocol)
{
  auxSerialStop();

  auxSerialMode = mode;

  switch (mode) {
    case UART_MODE_TELEMETRY_MIRROR:
#if defined(CROSSFIRE)
      if (protocol == PROTOCOL_PULSES_CROSSFIRE) { // PROTOCOL_TELEMETRY_CROSSFIRE
        auxSerialSetup(CROSSFIRE_TELEM_MIRROR_BAUDRATE, false);
        break;
      }
#endif
      auxSerialSetup(FRSKY_TELEM_MIRROR_BAUDRATE, false);
      break;

#if defined(DEBUG) || defined(CLI)
    case UART_MODE_DEBUG:
      auxSerialSetup(DEBUG_BAUDRATE, false);
      break;
#endif

#if defined(SBUS)
    case UART_MODE_SBUS_TRAINER:
      auxSerialSetup(SBUS_BAUDRATE, true, USART_WordLength_9b, USART_Parity_Even, USART_StopBits_2); // USART_WordLength_9b due to parity bit
      break;
#endif

#if !defined(PCBI6X)
    case UART_MODE_TELEMETRY:
      if (protocol == PROTOCOL_FRSKY_D_SECONDARY) {
        auxSerialSetup(FRSKY_D_BAUDRATE, true);
      }
      break;

    case UART_MODE_LUA:
      auxSerialSetup(DEBUG_BAUDRATE, false);
#endif
  }
}

void auxSerialPutc(char c)
{
#if !defined(SIMU)
  // do not wait, it can cause reboot and EdgeTX is not doing it
  if (auxSerialTxFifo.isFull()) return;

  auxSerialTxFifo.push(c);
  USART_ITConfig(AUX_SERIAL_USART, USART_IT_TXE, ENABLE);
#endif
}

void auxSerialSbusInit()
{
#if defined(SBUS)
  auxSerialInit(UART_MODE_SBUS_TRAINER, 0);
#endif
}

void auxSerialStop()
{
#if defined(STM32F0)
  DMA_DeInit(AUX_SERIAL_DMA_Channel_RX);
#else
  DMA_DeInit(AUX_SERIAL_DMA_Stream_RX);
#endif // STM32F0
  USART_DeInit(AUX_SERIAL_USART);
}

uint8_t auxSerialTracesEnabled()
{
#if defined(DEBUG)
  return (auxSerialMode == UART_MODE_DEBUG);
#else
  return false;
#endif
}

extern "C" void AUX_SERIAL_USART_IRQHandler(void)
{
  DEBUG_INTERRUPT(INT_SER2);
  // Send
  if (USART_GetITStatus(AUX_SERIAL_USART, USART_IT_TXE) != RESET) {
    uint8_t txchar;
    if (auxSerialTxFifo.pop(txchar)) {
      /* Write one byte to the transmit data register */
      USART_SendData(AUX_SERIAL_USART, txchar);
    }
    else {
      USART_ITConfig(AUX_SERIAL_USART, USART_IT_TXE, DISABLE);
    }
  }

#if defined(CLI)
  if (!(getSelectedUsbMode() == USB_SERIAL_MODE)) {
    // Receive
    uint32_t status = AUX_SERIAL_USART->SR;
    while (status & (USART_FLAG_RXNE | USART_FLAG_ERRORS)) {
      uint8_t data = AUX_SERIAL_USART->DR;
      if (!(status & USART_FLAG_ERRORS)) {
        switch (auxSerialMode) {
          case UART_MODE_DEBUG:
            cliRxFifo.push(data);
            break;
        }
      }
      status = AUX_SERIAL_USART->SR;
    }
  }
#endif
  // Receive
#if !defined(PCBI6X) // works but not needed
#if defined(STM32F0)
  uint32_t status = AUX_SERIAL_USART->ISR;
#else
  uint32_t status = AUX_SERIAL_USART->SR;
#endif // STM32F0
  while (status & (USART_FLAG_RXNE | USART_FLAG_ERRORS)) {
#if defined(STM32F0)
    uint8_t data = AUX_SERIAL_USART->RDR;
#else
    uint8_t data = AUX_SERIAL_USART->DR;
#endif // STM32F0
    UNUSED(data);
    if (!(status & USART_FLAG_ERRORS)) {
#if defined(LUA) & !defined(CLI)
      if (luaRxFifo && auxSerialMode == UART_MODE_LUA)
        luaRxFifo->push(data);
#endif
    }
#if defined(STM32F0)
    status = AUX_SERIAL_USART->ISR;
#else
    status = AUX_SERIAL_USART->SR;
#endif // STM32F0
  }
#endif // PCBI6X
}
#endif // AUX_SERIAL

/**
 * AUX2 Serial
 * reduced implementation to use IDLE irq
*/
#if defined(AUX2_SERIAL)
Fifo<uint8_t, 16> aux2SerialTxFifo;
DMAFifo<AUX2_SERIAL_RXFIFO_SIZE> aux2SerialRxFifo __DMA (AUX2_SERIAL_DMA_Channel_RX);
void (*aux2SerialIdleCb)(void);

void aux2SerialSetup(unsigned int baudrate, bool dma, uint16_t lenght = USART_WordLength_8b, uint16_t parity = USART_Parity_No, uint16_t stop = USART_StopBits_1)
{
  USART_InitTypeDef USART_InitStructure;
  GPIO_InitTypeDef GPIO_InitStructure;

  GPIO_PinAFConfig(AUX2_SERIAL_GPIO, AUX2_SERIAL_GPIO_PinSource_TX | AUX2_SERIAL_GPIO_PinSource_RX, AUX2_SERIAL_GPIO_AF);
  //  GPIO_PinAFConfig(AUX2_SERIAL_GPIO, AUX2_SERIAL_GPIO_PinSource_TX, AUX2_SERIAL_GPIO_AF);

  GPIO_InitStructure.GPIO_Pin = AUX2_SERIAL_GPIO_PIN_TX | AUX2_SERIAL_GPIO_PIN_RX;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
  GPIO_Init(AUX2_SERIAL_GPIO, &GPIO_InitStructure);

  USART_InitStructure.USART_BaudRate = baudrate;
  USART_InitStructure.USART_WordLength = lenght;
  USART_InitStructure.USART_StopBits = stop;
  USART_InitStructure.USART_Parity = parity;
  USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
  USART_InitStructure.USART_Mode = USART_Mode_Tx | USART_Mode_Rx;
  USART_Init(AUX2_SERIAL_USART, &USART_InitStructure);

  // if (dma) {
    aux2SerialRxFifo.stream = AUX2_SERIAL_DMA_Channel_RX; // workaround, CNDTR reading do not work otherwise
    DMA_InitTypeDef DMA_InitStructure;
    aux2SerialRxFifo.clear();
    USART_ITConfig(AUX2_SERIAL_USART, USART_IT_RXNE, DISABLE);
    USART_ITConfig(AUX2_SERIAL_USART, USART_IT_TXE, DISABLE);

    DMA_InitStructure.DMA_PeripheralBaseAddr = CONVERT_PTR_UINT(&AUX2_SERIAL_USART->RDR);
    DMA_InitStructure.DMA_MemoryBaseAddr = CONVERT_PTR_UINT(aux2SerialRxFifo.buffer());
    DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralSRC;
    DMA_InitStructure.DMA_BufferSize = aux2SerialRxFifo.size();
    DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
    DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
    DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
    DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
    DMA_InitStructure.DMA_Mode = DMA_Mode_Circular;
    DMA_InitStructure.DMA_Priority = DMA_Priority_Low;
    DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;
    DMA_Init(AUX2_SERIAL_DMA_Channel_RX, &DMA_InitStructure);
    USART_DMACmd(AUX2_SERIAL_USART, USART_DMAReq_Rx, ENABLE);
    USART_Cmd(AUX2_SERIAL_USART, ENABLE);
    DMA_Cmd(AUX2_SERIAL_DMA_Channel_RX, ENABLE);

    // AUX2_SERIAL_USART->CR1 |= USART_CR1_RXNEIE; // USART_ITConfig(AUX2_SERIAL_USART, USART_IT_RXNE, ENABLE);

  // else {
  //   USART_Cmd(AUX2_SERIAL_USART, ENABLE);
  //   USART_ITConfig(AUX2_SERIAL_USART, USART_IT_RXNE, ENABLE);
    // USART_ITConfig(AUX2_SERIAL_USART, USART_IT_TXE, DISABLE);
    NVIC_SetPriority(AUX2_SERIAL_USART_IRQn, 7);
    NVIC_EnableIRQ(AUX2_SERIAL_USART_IRQn);
  // }
}

void aux2SerialInit()
{
//   aux2SerialStop();

//   aux2SerialMode = mode;

  aux2SerialSetup(AUX2_SERIAL_BAUDRATE, true);
}

void aux2SerialPutc(char c)
{
#if !defined(SIMU)
  if (aux2SerialTxFifo.isFull()) return;

  aux2SerialTxFifo.push(c);
  USART_ITConfig(AUX2_SERIAL_USART, USART_IT_TXE, ENABLE);
#endif
}

void aux2SerialStop()
{
#if defined(STM32F0)
  DMA_DeInit(AUX2_SERIAL_DMA_Channel_RX);
#else
  DMA_DeInit(AUX2_SERIAL_DMA_Stream_RX);
#endif
  USART_DeInit(AUX2_SERIAL_USART);
}

void aux2SerialSetIdleCb(void (*cb)()) {
  if (cb == nullptr) {
    AUX2_SERIAL_USART->CR1 &= ~USART_CR1_IDLEIE;
  } else {
    AUX2_SERIAL_USART->CR1 |= USART_CR1_IDLEIE;
  }
  aux2SerialIdleCb = cb;
}

#if !defined(SIMU)
extern "C" void AUX2_SERIAL_USART_IRQHandler(void)
{
  // Send
  if (USART_GetITStatus(AUX2_SERIAL_USART, USART_IT_TXE) != RESET) {
    uint8_t txchar;
    if (aux2SerialTxFifo.pop(txchar)) {
      /* Write one byte to the transmit data register */
      USART_SendData(AUX2_SERIAL_USART, txchar);
    }
    else {
      USART_ITConfig(AUX2_SERIAL_USART, USART_IT_TXE, DISABLE);
    }
  }

  // Receive
  uint32_t status = AUX2_SERIAL_USART->ISR;
  // while (status & (USART_FLAG_RXNE | USART_FLAG_ERRORS)) {
  //   uint8_t data = AUX2_SERIAL_USART->RDR;
  //   UNUSED(data);
  //   if (!(status & USART_FLAG_ERRORS)) {
  //     if (aux2SerialIdleCb != nullptr) {
  //       aux2SerialIdleCb();
  //     }
  //   }
  //   status = AUX2_SERIAL_USART->ISR;
  // }

  // Idle
  if (status & USART_FLAG_IDLE) {
    AUX2_SERIAL_USART->ICR = USART_ICR_IDLECF;
    aux2SerialIdleCb();
  }
}
#endif // SIMU
#endif // AUX2_SERIAL

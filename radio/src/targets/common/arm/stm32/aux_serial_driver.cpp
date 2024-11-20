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
#if defined(DEBUG)
Fifo<uint8_t, 256> auxSerialTxFifo;
#else
Fifo<uint8_t, 128> auxSerialTxFifo;
#endif
DMAFifo<32> auxSerialRxFifo __DMA (AUX_SERIAL_DMA_Channel_RX);

void auxSerialSetup(unsigned int baudrate, bool dma, uint16_t lenght = USART_WordLength_8b, uint16_t parity = USART_Parity_No, uint16_t stop = USART_StopBits_1)
{
  USART_InitTypeDef USART_InitStructure;
  GPIO_InitTypeDef GPIO_InitStructure;

#if defined(SBUS_TRAINER)
  GPIO_PinAFConfig(AUX_SERIAL_GPIO, AUX_SERIAL_GPIO_PinSource_RX, AUX_SERIAL_GPIO_AF);
#endif
  GPIO_PinAFConfig(AUX_SERIAL_GPIO, AUX_SERIAL_GPIO_PinSource_TX, AUX_SERIAL_GPIO_AF);

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
#if defined(SBUS_TRAINER)
    auxSerialRxFifo.stream = AUX_SERIAL_DMA_Channel_RX; // workaround, CNDTR reading do not work otherwise
    auxSerialRxFifo.clear();
    USART_ITConfig(AUX_SERIAL_USART, USART_IT_RXNE, DISABLE);
    USART_ITConfig(AUX_SERIAL_USART, USART_IT_TXE, DISABLE);

    AUX_SERIAL_DMA_Channel_RX->CPAR = (uint32_t) &AUX_SERIAL_USART->RDR;
    AUX_SERIAL_DMA_Channel_RX->CMAR = (uint32_t) auxSerialRxFifo.buffer();
    AUX_SERIAL_DMA_Channel_RX->CNDTR = auxSerialRxFifo.size();
    AUX_SERIAL_DMA_Channel_RX->CCR = DMA_MemoryInc_Enable
                                | DMA_M2M_Disable
                                | DMA_Mode_Circular
                                | DMA_Priority_Low
                                | DMA_DIR_PeripheralSRC
                                | DMA_PeripheralInc_Disable
                                | DMA_PeripheralDataSize_Byte
                                | DMA_MemoryDataSize_Byte;

    USART_InvPinCmd(AUX_SERIAL_USART, USART_InvPin_Rx, ENABLE); // Only for SBUS
    USART_DMACmd(AUX_SERIAL_USART, USART_DMAReq_Rx, ENABLE);
    USART_Cmd(AUX_SERIAL_USART, ENABLE);
    DMA_Cmd(AUX_SERIAL_DMA_Channel_RX, ENABLE);
#endif // SBUS_TRAINER
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
// #if defined(CROSSFIRE)
//       if (protocol == PROTOCOL_PULSES_CROSSFIRE) { // PROTOCOL_TELEMETRY_CROSSFIRE
//         auxSerialSetup(CROSSFIRE_TELEM_MIRROR_BAUDRATE, false);
//         break;
//       }
// #endif
      // The same baudrate for Crossfire and AFHDS2A, but CROSSFIRE is optional
      auxSerialSetup(AFHDS2A_TELEM_MIRROR_BAUDRATE, false);
      break;

#if defined(DEBUG) || defined(CLI)
    case UART_MODE_DEBUG:
      auxSerialSetup(DEBUG_BAUDRATE, false);
      break;
#endif

#if defined(SBUS_TRAINER)
    case UART_MODE_SBUS_TRAINER:
      auxSerialSetup(SBUS_BAUDRATE, true, USART_WordLength_9b, USART_Parity_Even, USART_StopBits_2); // USART_WordLength_9b due to parity bit
//      AUX_SERIAL_POWER_ON();
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

#if defined(SBUS_TRAINER)
void auxSerialSbusInit()
{
  auxSerialInit(UART_MODE_SBUS_TRAINER, 0);
}
#endif

void auxSerialStop()
{
#if defined(SBUS_TRAINER)
  DMA_DeInit(AUX_SERIAL_DMA_Channel_RX);
#endif
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
  uint32_t status = AUX_SERIAL_USART->ISR;
  while (status & (USART_FLAG_RXNE | USART_FLAG_ERRORS)) {
    uint8_t data = AUX_SERIAL_USART->RDR;
    UNUSED(data);
    if (!(status & USART_FLAG_ERRORS)) {
#if defined(LUA) & !defined(CLI)
      if (luaRxFifo && auxSerialMode == UART_MODE_LUA)
        luaRxFifo->push(data);
#endif
    }
    status = AUX_SERIAL_USART->ISR;
  }
#endif // PCBI6X
}
#endif // AUX_SERIAL

/**
 * AUX3 Serial
 * reduced implementation, only TX
*/
#if defined(AUX3_SERIAL)
Fifo<uint8_t, 16> aux3SerialTxFifo;

void aux3SerialSetup(unsigned int baudrate, bool dma, uint16_t lenght = USART_WordLength_8b, uint16_t parity = USART_Parity_No, uint16_t stop = USART_StopBits_1)
{
  USART_InitTypeDef USART_InitStructure;
  GPIO_InitTypeDef GPIO_InitStructure;

  GPIO_PinAFConfig(AUX3_SERIAL_GPIO, AUX3_SERIAL_GPIO_PinSource_TX, AUX3_SERIAL_GPIO_AF);

  GPIO_InitStructure.GPIO_Pin = AUX3_SERIAL_GPIO_PIN_TX;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
  GPIO_Init(AUX3_SERIAL_GPIO, &GPIO_InitStructure);

  USART_InitStructure.USART_BaudRate = baudrate;
  USART_InitStructure.USART_WordLength = lenght;
  USART_InitStructure.USART_StopBits = stop;
  USART_InitStructure.USART_Parity = parity;
  USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
  USART_InitStructure.USART_Mode = USART_Mode_Tx;
  USART_Init(AUX3_SERIAL_USART, &USART_InitStructure);

  USART_Cmd(AUX3_SERIAL_USART, ENABLE);

  NVIC_SetPriority(AUX34_SERIAL_USART_IRQn, 7);
  NVIC_EnableIRQ(AUX34_SERIAL_USART_IRQn);
}

void aux3SerialInit(void)
{
  aux3SerialSetup(AUX3_SERIAL_BAUDRATE, true);
}

void aux3SerialPutc(char c)
{
#if !defined(SIMU)
  if (aux3SerialTxFifo.isFull()) return;

  aux3SerialTxFifo.push(c);
  USART_ITConfig(AUX3_SERIAL_USART, USART_IT_TXE, ENABLE);
#endif
}
#endif // AUX3_SERIAL

/**
 * AUX4 Serial
 * Reduced implementation to use IDLE irq, only RX
*/
#if defined(AUX4_SERIAL)
DMAFifo<AUX4_SERIAL_RXFIFO_SIZE> aux4SerialRxFifo __DMA (AUX4_SERIAL_DMA_Channel_RX);
void (*aux4SerialIdleCb)(void);

void aux4SerialSetup(unsigned int baudrate, bool dma, uint16_t lenght = USART_WordLength_8b, uint16_t parity = USART_Parity_No, uint16_t stop = USART_StopBits_1)
{
  USART_InitTypeDef USART_InitStructure;
  GPIO_InitTypeDef GPIO_InitStructure;

  GPIO_PinAFConfig(AUX4_SERIAL_GPIO, AUX4_SERIAL_GPIO_PinSource_RX, AUX4_SERIAL_GPIO_AF);

  GPIO_InitStructure.GPIO_Pin = AUX4_SERIAL_GPIO_PIN_RX;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
  GPIO_Init(AUX4_SERIAL_GPIO, &GPIO_InitStructure);

  USART_InitStructure.USART_BaudRate = baudrate;
  USART_InitStructure.USART_WordLength = lenght;
  USART_InitStructure.USART_StopBits = stop;
  USART_InitStructure.USART_Parity = parity;
  USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
  USART_InitStructure.USART_Mode = USART_Mode_Rx;
  USART_Init(AUX4_SERIAL_USART, &USART_InitStructure);

    aux4SerialRxFifo.stream = AUX4_SERIAL_DMA_Channel_RX; // workaround, CNDTR reading do not work otherwise
    aux4SerialRxFifo.clear();
    // USART_ITConfig(AUX4_SERIAL_USART, USART_IT_RXNE, DISABLE);
    AUX4_SERIAL_USART->CR1 &= ~(USART_CR1_RXNEIE /*| USART_CR1_TXEIE*/);

    AUX4_SERIAL_DMA_Channel_RX->CPAR = (uint32_t) &AUX4_SERIAL_USART->RDR;
    AUX4_SERIAL_DMA_Channel_RX->CMAR = (uint32_t) aux4SerialRxFifo.buffer();
    AUX4_SERIAL_DMA_Channel_RX->CNDTR = aux4SerialRxFifo.size();
    AUX4_SERIAL_DMA_Channel_RX->CCR = DMA_MemoryInc_Enable
                                | DMA_M2M_Disable
                                | DMA_Mode_Circular
                                | DMA_Priority_Low
                                | DMA_DIR_PeripheralSRC
                                | DMA_PeripheralInc_Disable
                                | DMA_PeripheralDataSize_Byte
                                | DMA_MemoryDataSize_Byte;

    USART_DMACmd(AUX4_SERIAL_USART, USART_DMAReq_Rx, ENABLE);
    USART_Cmd(AUX4_SERIAL_USART, ENABLE);
    DMA_Cmd(AUX4_SERIAL_DMA_Channel_RX, ENABLE);

    NVIC_SetPriority(AUX34_SERIAL_USART_IRQn, 7);
    NVIC_EnableIRQ(AUX34_SERIAL_USART_IRQn);
}

void aux4SerialInit(void)
{
  aux4SerialSetup(AUX4_SERIAL_BAUDRATE, true);
}

void aux4SerialStop(void)
{
  DMA_DeInit(AUX4_SERIAL_DMA_Channel_RX);
  USART_DeInit(AUX4_SERIAL_USART);
}

void aux4SerialSetIdleCb(void (*cb)()) {
  aux4SerialIdleCb = cb;
  AUX4_SERIAL_USART->CR1 |= USART_CR1_IDLEIE;
}
#endif // AUX4_SERIAL

#if defined(AUX3_SERIAL) || defined(AUX4_SERIAL)
#if !defined(SIMU)
extern "C" void AUX34_SERIAL_USART_IRQHandler(void)
{
  // Send
#if defined(AUX3_SERIAL)
  if (USART_GetITStatus(AUX3_SERIAL_USART, USART_IT_TXE) != RESET) {
    uint8_t txchar;
    if (aux3SerialTxFifo.pop(txchar)) {
      /* Write one byte to the transmit data register */
      USART_SendData(AUX3_SERIAL_USART, txchar);
    }
    else {
      USART_ITConfig(AUX3_SERIAL_USART, USART_IT_TXE, DISABLE);
    }
  }
#endif

  // Receive
  // uint32_t status = AUX4_SERIAL_USART->ISR;
  // while (status & (USART_FLAG_RXNE | USART_FLAG_ERRORS)) {
  //   uint8_t data = AUX4_SERIAL_USART->RDR;
  //   UNUSED(data);
  //   if (!(status & USART_FLAG_ERRORS)) {
  //
  //     }
  //   }
  //   status = AUX4_SERIAL_USART->ISR;
  // }

  // Idle
#if defined(AUX4_SERIAL)
  uint32_t status = AUX4_SERIAL_USART->ISR;
  if (status & USART_FLAG_IDLE) {
    AUX4_SERIAL_USART->ICR = USART_ICR_IDLECF;
    aux4SerialIdleCb();
  }
#endif
}
#endif // SIMU
#endif // AUX3 || AUX4

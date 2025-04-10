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
Fifo<uint8_t, 128> auxSerialTxFifo;
DMAFifo<32> auxSerialRxFifo __DMA (AUX_SERIAL_DMA_Channel_RX);

void auxSerialSetup(unsigned int baudrate, bool dma, uint16_t lenght = LL_USART_DATAWIDTH_8B, uint16_t parity = LL_USART_PARITY_NONE, uint16_t stop = LL_USART_STOPBITS_1)
{
  LL_GPIO_InitTypeDef GPIO_InitStruct = {0};
  GPIO_InitStruct.Pin        = AUX_SERIAL_GPIO_PIN_TX | AUX_SERIAL_GPIO_PIN_RX;
  GPIO_InitStruct.Mode       = LL_GPIO_MODE_ALTERNATE;
  GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
  GPIO_InitStruct.Pull       = LL_GPIO_PULL_UP;
  GPIO_InitStruct.Speed      = LL_GPIO_SPEED_FREQ_LOW;
  LL_GPIO_Init(AUX_SERIAL_GPIO, &GPIO_InitStruct);

  LL_USART_InitTypeDef usart_initstruct = {0};
  usart_initstruct.BaudRate            = baudrate;
  usart_initstruct.DataWidth           = lenght;
  usart_initstruct.StopBits            = stop;
  usart_initstruct.Parity              = parity;
  usart_initstruct.TransferDirection   = LL_USART_DIRECTION_TX_RX;
  usart_initstruct.HardwareFlowControl = LL_USART_HWCONTROL_NONE;

  LL_USART_Init(AUX_SERIAL_USART, &usart_initstruct);
//  LL_USART_ConfigAsyncMode(AUX_SERIAL_USART);

  if (dma) {
#if defined(SBUS_TRAINER)
    auxSerialRxFifo.channel = AUX_SERIAL_DMA_Channel_RX; // workaround, CNDTR reading do not work otherwise
    auxSerialRxFifo.clear();
    LL_USART_DisableIT_RXNE(AUX_SERIAL_USART);
    LL_USART_DisableIT_TXE(AUX_SERIAL_USART);

    AUX_SERIAL_DMA_Channel_RX->CPAR = (uint32_t) &AUX_SERIAL_USART->RDR;
    AUX_SERIAL_DMA_Channel_RX->CMAR = (uint32_t) auxSerialRxFifo.buffer();
    AUX_SERIAL_DMA_Channel_RX->CNDTR = auxSerialRxFifo.size();
    AUX_SERIAL_DMA_Channel_RX->CCR = LL_DMA_MEMORY_INCREMENT
                                | LL_DMA_MODE_CIRCULAR
                                | LL_DMA_PRIORITY_LOW
                                | LL_DMA_DIRECTION_PERIPH_TO_MEMORY
                                | LL_DMA_PERIPH_NOINCREMENT
                                | LL_DMA_PDATAALIGN_BYTE
                                | LL_DMA_MDATAALIGN_BYTE;

    LL_USART_SetRXPinLevel(AUX_SERIAL_USART, LL_USART_RXPIN_LEVEL_INVERTED); // Only for SBUS
    LL_USART_EnableDMAReq_RX(AUX_SERIAL_USART);
    LL_USART_Enable(AUX_SERIAL_USART);
    LL_DMA_EnableChannel(DMA1, AUX_SERIAL_DMA_Channel_RX_CH);
#endif // SBUS_TRAINER
  }
  else {
    LL_USART_Enable(AUX_SERIAL_USART);
#if !defined(PCBI6X)
    LL_USART_EnableIT_RXNE(AUX_SERIAL_USART);
#endif
    LL_USART_DisableIT_TXE(AUX_SERIAL_USART);
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
// The same baudrate for Crossfire and AFHDS2A, but CROSSFIRE is optional
// #if defined(CROSSFIRE)
//       if (protocol == PROTOCOL_TELEMETRY_CROSSFIRE) {
//         auxSerialSetup(CROSSFIRE_TELEM_MIRROR_BAUDRATE, false);
//         break;
//       }
// #endif
      auxSerialSetup(AFHDS2A_TELEM_MIRROR_BAUDRATE, false);
      break;

#if defined(DEBUG) || defined(CLI)
    case UART_MODE_DEBUG:
      auxSerialSetup(DEBUG_BAUDRATE, false);
      break;
#endif

#if defined(SBUS_TRAINER)
    case UART_MODE_SBUS_TRAINER:
      auxSerialSetup(SBUS_BAUDRATE, true, LL_USART_DATAWIDTH_9B, LL_USART_PARITY_EVEN, LL_USART_STOPBITS_2); // LL_USART_DATAWIDTH_9B due to parity bit
//      AUX_SERIAL_POWER_ON();
      break;
#endif

#if !defined(PCBI6X)
    case UART_MODE_TELEMETRY:
      if (protocol == PROTOCOL_TELEMETRY_FRSKY_D_SECONDARY) {
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
  LL_USART_EnableIT_TXE(AUX_SERIAL_USART);
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
  LL_DMA_DisableChannel(DMA1, AUX_SERIAL_DMA_Channel_RX_CH);
  LL_DMA_DeInit(DMA1, AUX_SERIAL_DMA_Channel_RX_CH);
#endif
  LL_USART_DeInit(AUX_SERIAL_USART);
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
  if (LL_USART_IsActiveFlag_TXE(AUX_SERIAL_USART)) {
    uint8_t txchar;
    if (auxSerialTxFifo.pop(txchar)) {
      /* Write one byte to the transmit data register */
      LL_USART_TransmitData8(AUX_SERIAL_USART, txchar);
   }
   else {
        /* Disable TXE interrupt when FIFO is empty */
        LL_USART_DisableIT_TXE(AUX_SERIAL_USART);
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

void aux3SerialSetup(unsigned int baudrate, bool dma, uint16_t lenght = LL_USART_DATAWIDTH_8B, uint16_t parity = LL_USART_PARITY_NONE, uint16_t stop = LL_USART_STOPBITS_1)
{
  LL_GPIO_InitTypeDef GPIO_InitStruct = {0};
  GPIO_InitStruct.Pin        = AUX3_SERIAL_GPIO_PIN_TX;
  GPIO_InitStruct.Mode       = LL_GPIO_MODE_ALTERNATE;
  GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
  GPIO_InitStruct.Pull       = LL_GPIO_PULL_UP;
  GPIO_InitStruct.Speed      = LL_GPIO_SPEED_FREQ_LOW;
  LL_GPIO_Init(AUX3_SERIAL_GPIO, &GPIO_InitStruct);

  LL_USART_InitTypeDef USART_InitStruct = {0};
  USART_InitStruct.BaudRate = baudrate;
  USART_InitStruct.DataWidth = lenght;
  USART_InitStruct.StopBits = stop;
  USART_InitStruct.Parity = parity;
  USART_InitStruct.TransferDirection = LL_USART_DIRECTION_TX;
  USART_InitStruct.HardwareFlowControl = LL_USART_HWCONTROL_NONE;
//  USART_InitStruct.OverSampling = LL_USART_OVERSAMPLING_16;
  LL_USART_Init(AUX3_SERIAL_USART, &USART_InitStruct);

  LL_USART_Enable(AUX3_SERIAL_USART);

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
  LL_USART_EnableIT_TXE(AUX3_SERIAL_USART);
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

void aux4SerialSetup(unsigned int baudrate, bool dma, uint16_t lenght = LL_USART_DATAWIDTH_8B, uint16_t parity = LL_USART_PARITY_NONE, uint16_t stop = LL_USART_STOPBITS_1)
{
  LL_GPIO_InitTypeDef GPIO_InitStruct = {0};
  GPIO_InitStruct.Pin        = AUX4_SERIAL_GPIO_PIN_RX;
  GPIO_InitStruct.Mode       = LL_GPIO_MODE_ALTERNATE;
  GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
  GPIO_InitStruct.Pull       = LL_GPIO_PULL_UP;
  GPIO_InitStruct.Speed      = LL_GPIO_SPEED_FREQ_LOW;
  LL_GPIO_Init(AUX4_SERIAL_GPIO, &GPIO_InitStruct);

  LL_USART_InitTypeDef USART_InitStruct = {0};
  USART_InitStruct.BaudRate = baudrate;
  USART_InitStruct.DataWidth = lenght;
  USART_InitStruct.StopBits = stop;
  USART_InitStruct.Parity = parity;
  USART_InitStruct.TransferDirection = LL_USART_DIRECTION_RX;
  USART_InitStruct.HardwareFlowControl = LL_USART_HWCONTROL_NONE;
//  USART_InitStruct.OverSampling = LL_USART_OVERSAMPLING_16;
  LL_USART_Init(AUX4_SERIAL_USART, &USART_InitStruct)

    aux4SerialRxFifo.channel = AUX4_SERIAL_DMA_Channel_RX; // workaround, CNDTR reading do not work otherwise
    aux4SerialRxFifo.clear();
    // USART_ITConfig(AUX4_SERIAL_USART, USART_IT_RXNE, DISABLE);
    AUX4_SERIAL_USART->CR1 &= ~(USART_CR1_RXNEIE /*| USART_CR1_TXEIE*/);

    AUX4_SERIAL_DMA_Channel_RX->CPAR = (uint32_t) &AUX4_SERIAL_USART->RDR;
    AUX4_SERIAL_DMA_Channel_RX->CMAR = (uint32_t) aux4SerialRxFifo.buffer();
    AUX4_SERIAL_DMA_Channel_RX->CNDTR = aux4SerialRxFifo.size();
    AUX4_SERIAL_DMA_Channel_RX->CCR = LL_DMA_MEMORY_INCREMENT
                                | LL_DMA_MODE_CIRCULAR
                                | LL_DMA_PRIORITY_LOW
                                | LL_DMA_DIRECTION_PERIPH_TO_MEMORY
                                | LL_DMA_PERIPH_NOINCREMENT
                                | LL_DMA_PDATAALIGN_BYTE
                                | LL_DMA_MDATAALIGN_BYTE;

    LL_USART_EnableDMAReq_RX(AUX4_SERIAL_USART);
    LL_USART_Enable(AUX4_SERIAL_USART);
    LL_DMA_EnableChannel(DMA1, AUX4_SERIAL_DMA_Channel_RX);

    NVIC_SetPriority(AUX34_SERIAL_USART_IRQn, 7);
    NVIC_EnableIRQ(AUX34_SERIAL_USART_IRQn);
}

void aux4SerialInit(void)
{
  aux4SerialSetup(AUX4_SERIAL_BAUDRATE, true);
}

void aux4SerialStop(void)
{
  LL_DMA_DisableChannel(DMA1, AUX4_SERIAL_DMA_Channel_RX);
  LL_DMA_DeInit(DMA1, AUX4_SERIAL_DMA_Channel_RX);
  LL_USART_DeInit(AUX4_SERIAL_USART);
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
if (LL_USART_IsActiveFlag_TXE(AUX3_SERIAL_USART)) {
    uint8_t txchar;
    if (aux3SerialTxFifo.pop(txchar)) {
      /* Write one byte to the transmit data register */
      LL_USART_TransmitData8(AUX3_SERIAL_USART, txchar);
    }
    else {
      LL_USART_DisableIT_TXE(AUX3_SERIAL_USART);
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

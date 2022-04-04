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
#include <stdio.h>

uint8_t auxSerialMode = 0;

void auxSerialSetup(unsigned int baudrate, bool dma)
{
  USART_InitTypeDef USART_InitStructure;
  GPIO_InitTypeDef GPIO_InitStructure;

  RCC_AHBPeriphClockCmd(AUX_SERIAL_RCC_AHB1Periph, ENABLE);
  RCC_APB2PeriphClockCmd(AUX_SERIAL_RCC_APB2Periph, ENABLE);

  GPIO_PinAFConfig(AUX_SERIAL_GPIO, AUX_SERIAL_GPIO_PinSource_RX, AUX_SERIAL_GPIO_AF);
  GPIO_PinAFConfig(AUX_SERIAL_GPIO, AUX_SERIAL_GPIO_PinSource_TX, AUX_SERIAL_GPIO_AF);

  GPIO_InitStructure.GPIO_Pin = AUX_SERIAL_GPIO_PIN_TX | AUX_SERIAL_GPIO_PIN_RX;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
  GPIO_Init(AUX_SERIAL_GPIO, &GPIO_InitStructure);

  USART_InitStructure.USART_BaudRate = baudrate;
  USART_InitStructure.USART_WordLength = USART_WordLength_8b;
  USART_InitStructure.USART_StopBits = USART_StopBits_1;
  USART_InitStructure.USART_Parity = USART_Parity_No;
  USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
  USART_InitStructure.USART_Mode = USART_Mode_Tx | USART_Mode_Rx;
  USART_Init(AUX_SERIAL_USART, &USART_InitStructure);

  USART_Cmd(AUX_SERIAL_USART, ENABLE);
}

void auxSerialInit(unsigned int mode, unsigned int protocol)
{
  auxSerialStop();

  auxSerialMode = mode;
  switch (mode)
  {
  case UART_MODE_TELEMETRY_MIRROR:
#if defined(CROSSFIRE)
      if (protocol == PROTOCOL_PULSES_CROSSFIRE) { // PROTOCOL_TELEMETRY_CROSSFIRE
        auxSerialSetup(CROSSFIRE_TELEM_MIRROR_BAUDRATE, false);
        break;
      }
#endif
    auxSerialSetup(FRSKY_SPORT_BAUDRATE, false);
    break;
#if defined(DEBUG) || defined(CLI)
  case UART_MODE_DEBUG:
    auxSerialSetup(DEBUG_BAUDRATE, false);
    break;
#endif
#if !defined(PCBI6X)
  case UART_MODE_TELEMETRY:
    if (protocol == PROTOCOL_FRSKY_D_SECONDARY)
    {
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
  while (USART_GetFlagStatus(AUX_SERIAL_USART, USART_FLAG_TXE) == RESET);
  USART_SendData(AUX_SERIAL_USART, c);
#endif
}

void auxSerialSbusInit()
{
  // auxSerialSetup(SBUS_BAUDRATE, true);
  // AUX_SERIAL_USART->CR1 |= USART_CR1_M | USART_CR1_PCE ;
}

void auxSerialStop()
{
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

// extern "C" void SERIAL_USART_IRQHandler(void)
// {
//   DEBUG_INTERRUPT(INT_SER2);
//   // Send
//   if (USART_GetITStatus(SERIAL_USART, USART_IT_TXE) != RESET) {
//     uint8_t txchar;
//     if (auxSerialTxFifo.pop(txchar)) {
//       /* Write one byte to the transmit data register */
//       USART_SendData(SERIAL_USART, txchar);
//     }
//     else {
//       USART_ITConfig(SERIAL_USART, USART_IT_TXE, DISABLE);
//     }
//   }

// #if defined(CLI)
//   if (!(getSelectedUsbMode() == USB_SERIAL_MODE)) {
//     // Receive
//     uint32_t status = SERIAL_USART->SR;
//     while (status & (USART_FLAG_RXNE | USART_FLAG_ERRORS)) {
//       uint8_t data = SERIAL_USART->DR;
//       if (!(status & USART_FLAG_ERRORS)) {
//         switch (auxSerialMode) {
//           case UART_MODE_DEBUG:
//             cliRxFifo.push(data);
//             break;
//         }
//       }
//       status = SERIAL_USART->SR;
//     }
//   }
// #endif
// }

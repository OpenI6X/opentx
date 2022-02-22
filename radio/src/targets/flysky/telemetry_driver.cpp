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

Fifo<uint8_t, TELEMETRY_FIFO_SIZE> telemetryFifo;
uint32_t telemetryErrors = 0;
static USART_InitTypeDef USART_InitStructure;
void uartSetDirection(bool tx);

void telemetryPortInit(uint32_t baudrate, uint8_t mode) {
  TRACE("telemetryPortInit %d", baudrate);

  if (baudrate == 0) {
    USART_DeInit(TELEMETRY_USART);
    return;
  }

  // RCC_AHBPeriphClockCmd(TELEMETRY_RCC_AHB1Periph, ENABLE);
  // RCC_APB1PeriphClockCmd(TELEMETRY_RCC_APB1Periph, ENABLE);

  /*
  Bits 13:12 PL[1:0]: Channel priority level
  These bits are set and cleared by software.
  00: Low
  01: Medium
  10: High
  11: Very high
  */

  NVIC_InitTypeDef NVIC_InitStructure;
  NVIC_InitStructure.NVIC_IRQChannel = TELEMETRY_DMA_TX_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPriority = 2; // High - In F4 NVIC_IRQChannelPreemptionPriority = 1; (0 is highest, 15 is lowest)
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);

  GPIO_InitTypeDef GPIO_InitStructure;

  /*
  From Cleanflight:
          ioConfig_t ioCfg = IO_CONFIG(GPIO_Mode_AF, GPIO_Speed_50MHz,
              ((options & SERIAL_INVERTED) || (options & SERIAL_BIDIR_PP)) ? GPIO_OType_PP : GPIO_OType_OD,
              ((options & SERIAL_INVERTED) || (options & SERIAL_BIDIR_PP)) ? GPIO_PuPd_DOWN : GPIO_PuPd_UP
          );
  */
  GPIO_InitStructure.GPIO_Pin = TELEMETRY_TX_GPIO_PIN;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_DOWN;  // was GPIO_PuPd_UP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(TELEMETRY_GPIO, &GPIO_InitStructure);

  USART_DeInit(TELEMETRY_USART);

  USART_OverSampling8Cmd(TELEMETRY_USART, ENABLE);

  GPIO_PinAFConfig(TELEMETRY_GPIO, TELEMETRY_GPIO_PinSource_TX, TELEMETRY_GPIO_AF);

  USART_InitStructure.USART_BaudRate = baudrate;
  if (mode & TELEMETRY_SERIAL_8E2) {
    USART_InitStructure.USART_WordLength = USART_WordLength_9b;
    USART_InitStructure.USART_StopBits = USART_StopBits_2;
    USART_InitStructure.USART_Parity = USART_Parity_Even;
  } else {
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;
    USART_InitStructure.USART_StopBits = USART_StopBits_1;
    USART_InitStructure.USART_Parity = USART_Parity_No;
  }
  USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
  USART_InitStructure.USART_Mode = USART_Mode_Tx | USART_Mode_Rx;
  USART_Init(TELEMETRY_USART, &USART_InitStructure);

  // Half duplex
  USART_HalfDuplexCmd(TELEMETRY_USART, ENABLE);

  // Level inversion
  USART_InvPinCmd(TELEMETRY_USART, USART_InvPin_Tx | USART_InvPin_Rx, ENABLE);

  USART_Cmd(TELEMETRY_USART, ENABLE);
  USART_ITConfig(TELEMETRY_USART, USART_IT_RXNE, ENABLE);
  NVIC_SetPriority(TELEMETRY_USART_IRQn, 6);
  NVIC_EnableIRQ(TELEMETRY_USART_IRQn);

  // // Debug pin setup
  // RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOA, ENABLE);
  // GPIO_InitTypeDef gpio_init;
  // gpio_init.GPIO_Mode = GPIO_Mode_OUT;
  // gpio_init.GPIO_OType = GPIO_OType_PP;
  // gpio_init.GPIO_Speed = GPIO_Speed_50MHz;
  // gpio_init.GPIO_PuPd = GPIO_PuPd_NOPULL;
  // gpio_init.GPIO_Pin = GPIO_Pin_15;
  // GPIO_Init(GPIOA, &gpio_init);
}

void telemetryPortSetDirectionOutput() {
  TELEMETRY_USART->CR1 &= ~USART_CR1_RE;  // disable receive
  TELEMETRY_USART->CR1 |= USART_CR1_TE;   // enable transmit
  //uartSetDirection(true);
}

void telemetryPortSetDirectionInput() {
  TELEMETRY_USART->CR1 &= ~USART_CR1_TE;  // disable trasmit
  TELEMETRY_USART->CR1 |= USART_CR1_RE;   // enable receive
  //uartSetDirection(false);
}

// With disable/enable, it seems it's not needed
// void uartSetDirection(bool tx) {
//   USART_Cmd(TELEMETRY_USART, DISABLE);
//   uint32_t inversionPins = 0;

//   if (tx) {
//     inversionPins |= USART_InvPin_Tx;
//     USART_InitStructure.USART_Mode = USART_Mode_Tx;
//   } else {
//     inversionPins |= USART_InvPin_Rx;
//     USART_InitStructure.USART_Mode = USART_Mode_Rx;
//   }

//   USART_Init(TELEMETRY_USART, &USART_InitStructure);
//   USART_Cmd(TELEMETRY_USART, ENABLE);
// }

void sportSendBuffer(const uint8_t* buffer, unsigned long count) {
  telemetryPortSetDirectionOutput();

  DMA_InitTypeDef DMA_InitStructure;
  DMA_StructInit(&DMA_InitStructure);
  DMA_DeInit(TELEMETRY_DMA_Channel_TX);
  /*
#define DMA_Priority_VeryHigh              DMA_CCR_PL
#define DMA_Priority_High                  DMA_CCR_PL_1
#define DMA_Priority_Medium                DMA_CCR_PL_0
#define DMA_Priority_Low                   ((uint32_t)0x00000000)
*/
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
    TELEMETRY_USART->CR1 |= USART_CR1_TCIE;
    if (telemetryProtocol == PROTOCOL_FRSKY_SPORT) {
      outputTelemetryBufferSize = 0;
      outputTelemetryBufferTrigger = 0x7E;
    }
  }
}

#define USART_FLAG_ERRORS (USART_FLAG_ORE | USART_FLAG_PE) // | USART_FLAG_FE, USART_FLAG_NE
extern "C" void TELEMETRY_USART_IRQHandler(void) {
  DEBUG_INTERRUPT(INT_TELEM_USART);
  uint32_t status = TELEMETRY_USART->ISR;
  if ((status & USART_FLAG_TC) && (TELEMETRY_USART->CR1 & USART_CR1_TCIE)) {
    TELEMETRY_USART->CR1 &= ~USART_CR1_TCIE;
    telemetryPortSetDirectionInput();
    while (status & (USART_FLAG_RXNE)) {
      status = TELEMETRY_USART->RDR;
      status = TELEMETRY_USART->ISR;
    }
  }
  while (status & (USART_FLAG_RXNE | USART_FLAG_ERRORS)) {
    uint8_t data = TELEMETRY_USART->RDR;
    if (status & USART_FLAG_ERRORS) {
      //TRACE("%d E", status & USART_FLAG_ERRORS);
      if (status & USART_FLAG_ORE) {
        USART_ClearITPendingBit(TELEMETRY_USART, USART_IT_ORE);
      }
//      if (status & USART_FLAG_NE) {
//        USART_ClearITPendingBit(TELEMETRY_USART, USART_FLAG_NE);
//      }
//       if (status & USART_FLAG_FE) {
//         USART_ClearITPendingBit(TELEMETRY_USART, USART_FLAG_FE);
//       }
      if (status & USART_FLAG_PE) {
        USART_ClearITPendingBit(TELEMETRY_USART, USART_FLAG_PE);
      }
    } else {
      //TRACE("O");
      telemetryFifo.push(data);

#if defined(LUA)
    if (telemetryProtocol == PROTOCOL_FRSKY_SPORT) {
      static uint8_t prevdata;
      if (prevdata == 0x7E && outputTelemetryBufferSize > 0 && data == outputTelemetryBufferTrigger) {
        sportSendBuffer(outputTelemetryBuffer, outputTelemetryBufferSize);
      }
      prevdata = data;
    }
#endif
    }
    status = TELEMETRY_USART->ISR;
  }
}

// TODO we should have telemetry in an higher layer, functions above should move to a sport_driver.cpp
uint8_t telemetryGetByte(uint8_t* byte) {
  // #if defined(AUX_SERIAL)
  //   if (telemetryProtocol == PROTOCOL_FRSKY_D_SECONDARY) {
  //     if (auxSerialMode == UART_MODE_TELEMETRY)
  //       return auxSerialRxFifo.pop(*byte);
  //     else
  //       return false;
  //   } else {
  //     return telemetryFifo.pop(*byte);
  //   }
  // #else
  return telemetryFifo.pop(*byte);
  // #endif
}

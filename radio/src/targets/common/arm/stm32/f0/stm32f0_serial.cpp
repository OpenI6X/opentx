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

#include "stm32f0_serial.h"

namespace stm32f0_serial {

static void configurePin(Pin pin, uint32_t drive)
{
  auto* gpio = reinterpret_cast<GPIO_TypeDef*>(static_cast<uintptr_t>(pin.base()));
  LL_GPIO_InitTypeDef config = {
    1u << pin.number(), LL_GPIO_MODE_ALTERNATE, drive >> 2,
    LL_GPIO_OUTPUT_PUSHPULL, drive & 3u, pin.af(),
  };
  LL_GPIO_Init(gpio, &config);
}

void configure(const Usart& port, uint16_t brr, LineProfile profile)
{
  const auto bits = static_cast<uint32_t>(profile);
  const bool singleWire = bits & line_bits::singleWire;
  const auto drive = bits & line_bits::drive;
  uint32_t cr1 = bits & line_bits::cr1;
  if (port.tx.present()) {
    configurePin(port.tx, drive);
    cr1 |= LL_USART_DIRECTION_TX;
  }
  if (port.rx.present()) {
    // Single-wire CRSF still uses the receiver, but on the TX pad.
    if (!singleWire) configurePin(port.rx, drive);
    cr1 |= LL_USART_DIRECTION_RX;
  }

  auto* usart = port.regs();
  usart->CR1 = cr1;
  usart->CR2 = bits & line_bits::cr2;
  usart->CR3 = singleWire ? USART_CR3_HDSEL : 0;
  usart->BRR = brr;
}

void resetDma(uint32_t channel)
{
  auto* dma = dmaRegs(channel);
  CLEAR_BIT(dma->CCR, DMA_CCR_EN); // disable before changing the configuration
  dma->CCR = 0;
  dma->CNDTR = 0;
  dma->CPAR = 0;
  dma->CMAR = 0;
  DMA1->IFCR = dmaFlag(channel, DMA_IFCR_CGIF1);
}

// On the slow init path, keep a single setup body for trainer and telemetry.
__attribute__((noinline))
void startRxDma(const Usart& port, uint8_t* buffer, uint32_t size, bool receiverTimeout)
{
  auto* usart = port.regs();
  auto* dma = dmaRegs(port.rxDma);
  dma->CPAR = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(&usart->RDR));
  dma->CMAR = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(buffer));
  dma->CNDTR = size;
  constexpr uint32_t rxConfig = DMA_CCR_MINC | DMA_CCR_CIRC;
  dma->CCR = rxConfig; // low priority, peripheral-to-memory, byte widths

  if (receiverTimeout) {
    // RTO starts after a received stop bit. Unlike IDLE, it doesn't fire
    // just because the half-duplex receiver was re-enabled after TX.
    usart->RTOR = 10; // bit times; the other fields are at reset values
    LL_USART_EnableRxTimeout(usart);
    LL_USART_ClearFlag_RTO(usart);
  }
  LL_USART_EnableDMAReq_RX(usart);
  ATOMIC_MODIFY_REG(usart->CR1, USART_CR1_RXNEIE | USART_CR1_TXEIE | USART_CR1_RTOIE,
                    USART_CR1_UE | (receiverTimeout ? USART_CR1_RTOIE : 0u));
  dma->CCR = rxConfig | DMA_CCR_EN;
}

void sendTxDma(const Usart& port, const uint8_t* buffer, uint32_t size)
{
  auto* usart = port.regs();
  auto* dma = dmaRegs(port.txDma);
  resetDma(port.txDma);
  dma->CPAR = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(&usart->TDR));
  dma->CMAR = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(buffer));
  dma->CNDTR = size;
  dma->CCR = DMA_CCR_MINC | DMA_CCR_DIR | LL_DMA_PRIORITY_VERYHIGH;
  SET_BIT(dma->CCR, DMA_CCR_EN);
  LL_USART_EnableDMAReq_TX(usart);
  SET_BIT(dma->CCR, DMA_CCR_TCIE);
}

} // namespace stm32f0_serial

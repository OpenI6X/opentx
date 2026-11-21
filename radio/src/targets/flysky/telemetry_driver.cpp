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
#include "serial_ports.h"

using namespace stm32f0_serial;
static constexpr auto& port = flysky_serial::telemetry;

DMAFifo<TELEMETRY_FIFO_SIZE> telemetryDMAFifo __DMA (dmaRegs(port.rxDma));

void telemetryPortInit(uint32_t baudrate, uint8_t mode) {
  TRACE("telemetryPortInit %d", baudrate);

  NVIC_SetPriority(flysky_serial::telemetryDmaTxIrq, flysky_serial::telemetryDmaTxPriority);
  NVIC_EnableIRQ(flysky_serial::telemetryDmaTxIrq);

  const bool halfDuplex = !g_eeGeneral.crsfFullDuplex;
  LL_USART_DeInit(port.regs());
  const SerialLineInit line{brr8(flysky_serial::clockHz, baudrate),
                            halfDuplex ? LineProfile::CrsfHalf : LineProfile::CrsfFull};
  configure(port, line);

  resetDma(port.rxDma);
  telemetryDMAFifo.channel = dmaRegs(port.rxDma);
  telemetryDMAFifo.clear();
  enableIrq(port);
  startRxDma(port, telemetryDMAFifo.buffer(), telemetryDMAFifo.size(), true);
}

void telemetryPortSetDirectionOutput() {
  // Disable RX
  if (!g_eeGeneral.crsfFullDuplex) {
    dmaRegs(port.rxDma)->CCR &= ~DMA_CCR_EN;
    port.regs()->CR1 &= ~USART_CR1_RE;
  }

  // Enable TX
  port.regs()->CR1 |= USART_CR1_TE;
}

void telemetryPortSetDirectionInput() {
  // Disable TX
  if (!g_eeGeneral.crsfFullDuplex) {
    dmaRegs(port.txDma)->CCR &= ~DMA_CCR_EN;
    port.regs()->CR1 &= ~USART_CR1_TE;
  }

  // Enable RX
  port.regs()->CR1 |= USART_CR1_RE;
  dmaRegs(port.rxDma)->CCR |= DMA_CCR_EN;
}

void sportSendBuffer(const uint8_t* buffer, uint32_t count) {
  telemetryPortSetDirectionOutput();

  sendTxDma(port, buffer, count);
}

extern "C" void TELEMETRY_DMA_TX_IRQHandler(void) {
  DEBUG_INTERRUPT(INT_TELEM_DMA);
  constexpr auto txComplete = dmaFlag(port.txDma, DMA_ISR_TCIF1);
  if (DMA1->ISR & txComplete) {
    DMA1->IFCR = txComplete;
    // clear TC flag before enabling interrupt
    port.regs()->ICR = USART_ICR_TCCF;
    port.regs()->CR1 |= USART_CR1_TCIE;
  }
}

extern "C" void TELEMETRY_USART_IRQHandler(void) {
  DEBUG_INTERRUPT(INT_TELEM_USART);
  uint32_t status = port.regs()->ISR;

  // RX, end of burst (receiver timeout); handled before the TX turnaround
  if (status & USART_ISR_RTOF) {
    port.regs()->ICR = USART_ICR_RTOCF;
    TELEMETRY_STATS_INC(telemetryStatsRto);
    telemetryFrameTrigger_ISR();
  }

  // TX, transfer complete
  if ((status & USART_ISR_TC) && (port.regs()->CR1 & USART_CR1_TCIE)) {
    // disable TC IRQ
    port.regs()->CR1 &= ~USART_CR1_TCIE;

    telemetryPortSetDirectionInput();

    while (port.regs()->ISR & USART_ISR_RXNE) {
      (void)port.regs()->RDR;
    }
  }

  // RX data is moved by DMA.
}

// TODO we should have telemetry in an higher layer, functions above should move to a sport_driver.cpp
uint8_t telemetryGetByte(uint8_t* byte) {
  return telemetryDMAFifo.pop(*byte);
}

// EdgeTX copyRxBuffer(): move up to len received bytes to buf, returns the count
int telemetryCopyRxBuffer(uint8_t* buf, uint32_t len) {
  return telemetryDMAFifo.copy(buf, len);
}

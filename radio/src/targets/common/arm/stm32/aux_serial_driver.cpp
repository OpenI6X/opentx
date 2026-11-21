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

#if defined(PCBI6X)
#include "serial_ports.h"

// Keep direct USART accesses in the IRQs, while making the wiring a single
// source of truth in the board's port descriptors.
#define AUX_SERIAL_USART  (flysky_serial::aux1.regs())
#define AUX3_SERIAL_USART (flysky_serial::aux3.regs())
#define AUX4_SERIAL_USART (flysky_serial::aux4.regs())
#define AUX_SERIAL_DMA_Channel_RX (stm32f0_serial::dmaRegs(flysky_serial::aux1.rxDma))
#endif

#if defined(AUX_SERIAL)
uint8_t auxSerialMode = UART_MODE_COUNT;  // Prevent debug output before port is setup
#include "serial_buffer_union.h"
extern SerialBufferUnion serialBuffer;
DMAFifo<32> auxSerialRxFifo __DMA (AUX_SERIAL_DMA_Channel_RX);

using stm32f0_serial::LineProfile;
using stm32f0_serial::SerialLineInit;

enum class AuxRx : uint8_t { NoDma, DmaWithTimeout };

static void auxSerialStart(const SerialLineInit& line, AuxRx rx)
{
  using namespace stm32f0_serial;
  configure(flysky_serial::aux1, line);

  if (rx == AuxRx::DmaWithTimeout) {
#if defined(SBUS_TRAINER)
    auxSerialRxFifo.channel = AUX_SERIAL_DMA_Channel_RX; // workaround, CNDTR reading do not work otherwise
    auxSerialRxFifo.clear();
    startRxDma(flysky_serial::aux1, auxSerialRxFifo.buffer(), auxSerialRxFifo.size(), true);
#endif // SBUS_TRAINER
  }
  else {
    serialTxBufferClear(); // clear for all TX modes
    LL_USART_Enable(AUX_SERIAL_USART);
#if !defined(PCBI6X) // defined(LUA) || defined(CLI)
    LL_USART_EnableIT_RXNE(AUX_SERIAL_USART);
#endif
    LL_USART_DisableIT_TXE(AUX_SERIAL_USART);
  }

  enableIrq(flysky_serial::aux1);
}

// Select AUX1 settings only. If no mode matches, leave BRR zero and keep it stopped.
static AuxRx serialSetupPort(unsigned int mode, SerialLineInit& line)
{
  using stm32f0_serial::brr16;
  using flysky_serial::clockHz;
  switch (mode) {
    case UART_MODE_TELEMETRY_MIRROR:
      // The same baudrate for Crossfire and AFHDS2A, CROSSFIRE_TELEM_MIRROR_BAUDRATE
      line.brr = brr16(clockHz, AFHDS2A_TELEM_MIRROR_BAUDRATE);
      break;

#if defined(DEBUG) || defined(CLI)
    case UART_MODE_DEBUG:
      line.brr = brr16(clockHz, DEBUG_BAUDRATE);
      break;
#endif

#if defined(SBUS_TRAINER)
    case UART_MODE_SBUS_TRAINER:
      line.brr = brr16(clockHz, SBUS_BAUDRATE);
      line.profile = LineProfile::Sbus;
      return AuxRx::DmaWithTimeout;
#endif

#if !defined(PCBI6X)
    case UART_MODE_TELEMETRY:
      if (g_model.telemetryProtocol == PROTOCOL_TELEMETRY_FRSKY_D_SECONDARY) {
        line.brr = brr16(clockHz, FRSKY_D_BAUDRATE);
        line.profile = LineProfile::Aux8N1Inverted;
        return AuxRx::DmaWithTimeout;
      }
      break;

    case UART_MODE_LUA:
      line.brr = brr16(clockHz, DEBUG_BAUDRATE);
      break;
#endif
  }
  return AuxRx::NoDma;
}

void auxSerialInit(unsigned int mode)
{
  auxSerialStop();
  auxSerialMode = mode;

  using namespace stm32f0_serial;
  SerialLineInit line{0, LineProfile::Aux8N1};  // disabled, 8N1
  const auto rx = serialSetupPort(mode, line);
  if (line.brr != 0) {
    auxSerialStart(line, rx);
  }
}

void auxSerialPutc(char c)
{
#if !defined(SIMU)
  // do not wait, it can cause reboot and EdgeTX is not doing it
  if (serialBuffer.auxSerialTxFifo.isFull()) return;

  serialBuffer.auxSerialTxFifo.push(c);
  LL_USART_EnableIT_TXE(AUX_SERIAL_USART);
#endif
}

#if defined(SBUS_TRAINER)
void auxSerialSbusInit()
{
  auxSerialInit(UART_MODE_SBUS_TRAINER);
}
#endif

void auxSerialStop()
{
#if defined(SBUS_TRAINER)
  stm32f0_serial::resetDma(flysky_serial::aux1.rxDma);
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
    if (serialBuffer.auxSerialTxFifo.pop(txchar)) {
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
    uint32_t status = AUX_SERIAL_USART->ISR;
    while (status & (USART_ISR_RXNE | USART_FLAG_ERRORS)) {
      uint8_t data = AUX_SERIAL_USART->RDR;
      if (!(status & USART_FLAG_ERRORS)) {
        switch (auxSerialMode) {
          case UART_MODE_DEBUG:
            cliRxFifo.push(data);
            break;
        }
      }
      status = AUX_SERIAL_USART->ISR;
    }
  }
#endif

#if defined(SBUS_TRAINER)
  // RX, end of burst (receiver timeout); handled before the TX turnaround
  if (AUX_SERIAL_USART->ISR & USART_ISR_RTOF) {
    AUX_SERIAL_USART->ICR = USART_ICR_RTOCF;
    timer_start(&trainerSbusFrameTimer);
  }
#endif

  // Receive (LUA) - PCBI6X: works but not needed
#if defined(LUA)
  uint32_t status = AUX_SERIAL_USART->ISR;
  while (status & (USART_ISR_RXNE | USART_FLAG_ERRORS)) {
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
#endif
}
#endif // AUX_SERIAL

/**
 * AUX3 Serial
 * reduced implementation, only TX
*/
#if defined(AUX3_SERIAL)
Fifo<uint8_t, 16> aux3SerialTxFifo;

void aux3SerialInit(void)
{
  using namespace stm32f0_serial;
  const auto& port = flysky_serial::aux3;
  constexpr SerialLineInit line{brr16(flysky_serial::clockHz, AUX3_SERIAL_BAUDRATE), LineProfile::Aux8N1};
  LL_USART_DeInit(port.regs());
  configure(port, line);
  LL_USART_Enable(port.regs());
  enableIrq(port);
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
DMAFifo<AUX4_SERIAL_RXFIFO_SIZE> aux4SerialRxFifo __DMA
    (stm32f0_serial::dmaRegs(flysky_serial::aux4.rxDma));
void (*aux4SerialIdleCb)(void);

void aux4SerialInit()
{
  using namespace stm32f0_serial;
  const auto& port = flysky_serial::aux4;
  constexpr SerialLineInit line{brr16(flysky_serial::clockHz, AUX4_SERIAL_BAUDRATE), LineProfile::Aux8N1};
  aux4SerialStop();
  configure(port, line);
  aux4SerialRxFifo.channel = dmaRegs(port.rxDma);
  aux4SerialRxFifo.clear();
  // Hall keeps its own IDLE framing, armed when the callback is installed.
  startRxDma(port, aux4SerialRxFifo.buffer(), aux4SerialRxFifo.size(), false);
  enableIrq(port);
}

void aux4SerialStop()
{
  stm32f0_serial::resetDma(flysky_serial::aux4.rxDma);
  LL_USART_DeInit(flysky_serial::aux4.regs());
}

void aux4SerialSetIdleCb(void (*cb)()) {
  aux4SerialIdleCb = cb;
  LL_USART_EnableIT_IDLE(AUX4_SERIAL_USART);
}
#endif // AUX4_SERIAL

#if defined(AUX3_SERIAL) || defined(AUX4_SERIAL)
#if !defined(SIMU)
extern "C" void AUX34_SERIAL_USART_IRQHandler(void)
{
  // Send
#if defined(AUX3_SERIAL)
  if (LL_USART_IsEnabledIT_TXE(AUX3_SERIAL_USART) && LL_USART_IsActiveFlag_TXE(AUX3_SERIAL_USART)) {
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
  // while (status & (USART_ISR_RXNE | USART_FLAG_ERRORS)) {
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
  if (LL_USART_IsActiveFlag_IDLE(AUX4_SERIAL_USART)) {
    LL_USART_ClearFlag_IDLE(AUX4_SERIAL_USART);
    if (aux4SerialIdleCb) aux4SerialIdleCb();
  }
#endif
}
#endif // SIMU
#endif // AUX3 || AUX4

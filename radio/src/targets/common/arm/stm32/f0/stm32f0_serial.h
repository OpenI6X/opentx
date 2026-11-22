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

#pragma once

#include "stm32_hal.h" // Load the simulator CMSIS stubs before the LL headers
#include "stm32_hal_ll.h"

namespace stm32f0_serial {

// F072 GPIO banks start at GPIOA_BASE, on 1 KiB boundaries. Keep the
// GPIO bank offset in bits 10..12, AF in bits 4..6 and pin number in 0..3.
struct Pin {
  uint16_t packed;
  constexpr bool present() const { return (packed & 0x8000u) == 0; }
  constexpr uint32_t number() const { return packed & 0xfu; }
  constexpr uint32_t af() const { return (packed >> 4) & 7u; }
  constexpr uint32_t base() const { return GPIOA_BASE + (packed & 0x1c00u); }
};

static_assert(sizeof(Pin) == 2, "Pin descriptors must stay packed");
constexpr Pin NoPin{0x8000};

template<uint32_t Base, unsigned Number, unsigned AF>
constexpr Pin gpioAfPin()
{
  static_assert(Base >= GPIOA_BASE && Base <= GPIOF_BASE && ((Base - GPIOA_BASE) % 0x400 == 0),
                "Invalid STM32F072 GPIO bank");
  static_assert(Number < 16 && AF < 8, "Invalid STM32F072 GPIO pin or AF");
  return Pin{static_cast<uint16_t>((Base - GPIOA_BASE) | (AF << 4) | Number)};
}

#define STM32F0_GPIO_AF_PIN(gpio, pin, af) ::stm32f0_serial::gpioAfPin<gpio##_BASE, pin, af>()

struct Usart {
  uint32_t base;
  Pin tx;
  Pin rx;
  uint8_t irq;      // external NVIC IRQ number
  uint8_t priority;
  uint8_t rxDma;  // DMA1 channel number, 0 if unused
  uint8_t txDma;  // DMA1 channel number, 0 if unused
  uint16_t oversampling;  // LL_USART_OVERSAMPLING_*
  uint8_t gpioSpeed;      // LL_GPIO_SPEED_FREQ_*

  USART_TypeDef* regs() const { return reinterpret_cast<USART_TypeDef*>(static_cast<uintptr_t>(base)); }
};

enum class SerialEncoding : uint8_t { Bits8N1, Bits8E2 };
enum class SerialPolarity : uint8_t { Normal, RxInverted, TxRxInverted };

// Match LL_USART_SetBaudRate's rounding and BRR encoding. AUX rates fold
// at compile time; the caller still supplies the telemetry rate at runtime.
constexpr uint16_t brr16(uint32_t clockHz, uint32_t baudRate)
{
  return baudRate ? static_cast<uint16_t>(__LL_USART_DIV_SAMPLING16(clockHz, baudRate)) : 0;
}

inline uint16_t brr8(uint32_t clockHz, uint32_t baudRate)
{
  if (!baudRate) return 0;
  const uint16_t div = __LL_USART_DIV_SAMPLING8(clockHz, baudRate);
  return (div & 0xfff0u) | ((div & 0xfu) >> 1);
}

// The caller supplies BRR; TX/RX direction follows the port's wired pins.
struct SerialInit {
  uint16_t brr;
  SerialEncoding encoding;
  SerialPolarity polarity;
};

// Configure a freshly reset USART; leave UE and interrupt/DMA enables to the caller.
void configure(const Usart& port, SerialInit params, bool halfDuplex = false);

// Keep IRQ number and priority constant at each call site.
__attribute__((always_inline)) inline void enableIrq(const Usart& port)
{
  const auto irq = static_cast<IRQn_Type>(port.irq);
  NVIC_SetPriority(irq, port.priority);
  NVIC_EnableIRQ(irq);
}

// F072 DMA1 has seven channels with equally spaced register blocks and flags.
// Only call these helpers with a channel number in [1, 7].
inline DMA_Channel_TypeDef* dmaRegs(uint32_t channel)
{
  return reinterpret_cast<DMA_Channel_TypeDef*>(static_cast<uintptr_t>(DMA1_Channel1_BASE +
      (channel - 1) * (DMA1_Channel2_BASE - DMA1_Channel1_BASE)));
}

constexpr uint32_t dmaFlag(uint32_t channel, uint32_t channel1Flag)
{
  return channel1Flag << (4 * (channel - 1));
}

// Disable and reset one DMA1 channel (including its pending flags).
void resetDma(uint32_t channel);

// The caller resets the DMA channel and FIFO cursor. USART must still be off.
// Set receiverTimeout for 10-bit-time framing; leave it off for IDLE users.
void startRxDma(const Usart& port, uint8_t* buffer, uint32_t size, bool receiverTimeout);
void sendTxDma(const Usart& port, const uint8_t* buffer, uint32_t size);

} // namespace stm32f0_serial

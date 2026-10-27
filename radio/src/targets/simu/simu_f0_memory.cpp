/*
 * Copyright (C) OpenTX
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

// STM32F0 simulator/gtests: back the MCU's register address ranges with plain RAM.
//
// F0 drivers use the LL library on the real peripheral addresses, and LL computes register
// addresses through uint32_t (e.g. DMA channel = (uint32_t)DMAx + offset). Fake peripheral
// objects inside the simulator library sit above 4 GB on 64-bit hosts and would be truncated,
// so instead the F0 address ranges themselves are mapped: every register access then hits RAM
// at the address the firmware expects, and the uint32_t arithmetic is lossless.
//
// Deliberately includes neither opentx.h nor CMSIS: windows.h does not mix with them.
// Addresses are from CMSIS stm32f072xb.h.

#include <cstdint>
#include <cstdio>
#include <cstdlib>

#if defined(_WIN32)
  #include <windows.h>
#else
  #include <sys/mman.h>
#endif

static void simuMapRegion(uintptr_t address, size_t size, const char * what)
{
#if defined(_WIN32)
  void * p = VirtualAlloc((void *)address, size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
#else
  void * p = mmap((void *)address, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (p == MAP_FAILED)
    p = nullptr;
#endif
  if (p != (void *)address) {
    fprintf(stderr, "simu: cannot map STM32F0 %s at 0x%08lx (address range in use)\n", what, (unsigned long)address);
    abort();
  }
}

static void simuUnmapRegion(uintptr_t address, size_t size)
{
#if defined(_WIN32)
  VirtualFree((void *)address, 0, MEM_RELEASE);
#else
  munmap((void *)address, size);
#endif
}

static inline volatile uint32_t & reg(uintptr_t address)
{
  return *(volatile uint32_t *)address;
}

static const struct {
  uintptr_t address;
  size_t size;
  const char * what;
} simuRegions[] = {
  { 0x1FFF0000, 0x10000, "system memory" },        // UID_BASE 0x1FFFF7AC, FLASHSIZE_BASE 0x1FFFF7CC
  { 0x40000000, 0x30000, "APB/AHB peripherals" },  // PERIPH_BASE .. CRC_BASE 0x40023000
  { 0x48000000, 0x10000, "GPIO" },                 // AHB2PERIPH_BASE, GPIOA..GPIOF
};

// Mapped while the simulator library (or gtests) is loaded. Companion loads the library once to
// list the radios and unloads it again, so the ranges must be released on unload.
static struct SimuF0Memory {
  SimuF0Memory()
  {
    for (auto & region : simuRegions)
      simuMapRegion(region.address, region.size, region.what);

    // Status flags drivers busy-wait on: present an idle peripheral that is always ready.
    reg(0x40013008) = (1u << 1) | (1u << 0);  // SPI1->SR: SPI_SR_TXE | SPI_SR_RXNE
    reg(0x40012400) = (1u << 0);              // ADC1->ISR: ADC_ISR_ADRDY
  }

  ~SimuF0Memory()
  {
    for (auto & region : simuRegions)
      simuUnmapRegion(region.address, region.size);
  }
} simuF0Memory;

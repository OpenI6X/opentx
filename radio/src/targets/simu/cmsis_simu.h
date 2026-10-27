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

// Host replacement for CMSIS cmsis_gcc.h in simulator and gtests builds of STM32F0 targets.
// It keeps the compiler attribute macros but turns every core intrinsic into host code:
// the real ones are ARM inline assembly, which the ST LL headers expand into inline
// functions (e.g. ATOMIC_SET_BIT -> __get_PRIMASK) long before simpgmspace.h is included.

#pragma once

#if !defined(SIMU)
  #error "cmsis_simu.h is for simulator/gtests builds only"
#endif

#include <stdint.h>

#define __CMSIS_GCC_H  // cmsis_compiler.h -> cmsis_gcc.h becomes a no-op

#ifndef __has_builtin
  #define __has_builtin(x) (0)
#endif

#define __ASM                       __asm
#define __INLINE                    inline
#define __STATIC_INLINE             static inline
#define __STATIC_FORCEINLINE        __attribute__((always_inline)) static inline
#define __NO_RETURN                 __attribute__((__noreturn__))
#define __USED                      __attribute__((used))
#define __WEAK                      __attribute__((weak))
#define __PACKED                    __attribute__((packed, aligned(1)))
#define __PACKED_STRUCT             struct __attribute__((packed, aligned(1)))
#define __PACKED_UNION              union __attribute__((packed, aligned(1)))
#define __ALIGNED(x)                __attribute__((aligned(x)))
#define __RESTRICT                  __restrict

__PACKED_STRUCT T_UINT32 { uint32_t v; };
__PACKED_STRUCT T_UINT16_WRITE { uint16_t v; };
__PACKED_STRUCT T_UINT16_READ { uint16_t v; };
__PACKED_STRUCT T_UINT32_WRITE { uint32_t v; };
__PACKED_STRUCT T_UINT32_READ { uint32_t v; };
#define __UNALIGNED_UINT32(x)                (((struct T_UINT32 *)(x))->v)
#define __UNALIGNED_UINT16_WRITE(addr, val)  (void)((((struct T_UINT16_WRITE *)(void *)(addr))->v) = (val))
#define __UNALIGNED_UINT16_READ(addr)        (((const struct T_UINT16_READ *)(const void *)(addr))->v)
#define __UNALIGNED_UINT32_WRITE(addr, val)  (void)((((struct T_UINT32_WRITE *)(void *)(addr))->v) = (val))
#define __UNALIGNED_UINT32_READ(addr)        (((const struct T_UINT32_READ *)(const void *)(addr))->v)

// Interrupt masking: the simulator has no interrupts, PRIMASK is just remembered.
extern uint32_t simuPrimask;
__STATIC_FORCEINLINE void __enable_irq(void) { simuPrimask = 0; }
__STATIC_FORCEINLINE void __disable_irq(void) { simuPrimask = 1; }
__STATIC_FORCEINLINE uint32_t __get_PRIMASK(void) { return simuPrimask; }
__STATIC_FORCEINLINE void __set_PRIMASK(uint32_t priMask) { simuPrimask = priMask; }

// Core registers: nothing to read or switch on the host.
__STATIC_FORCEINLINE uint32_t __get_CONTROL(void) { return 0; }
__STATIC_FORCEINLINE void __set_CONTROL(uint32_t) { }
__STATIC_FORCEINLINE uint32_t __get_IPSR(void) { return 0; }
__STATIC_FORCEINLINE uint32_t __get_APSR(void) { return 0; }
__STATIC_FORCEINLINE uint32_t __get_xPSR(void) { return 0; }
__STATIC_FORCEINLINE uint32_t __get_PSP(void) { return 0; }
__STATIC_FORCEINLINE void __set_PSP(uint32_t) { }
__STATIC_FORCEINLINE uint32_t __get_MSP(void) { return 0; }
__STATIC_FORCEINLINE void __set_MSP(uint32_t) { }

// Hints and barriers.
#define __NOP()        ((void)0)
#define __WFI()        ((void)0)
#define __WFE()        ((void)0)
#define __SEV()        ((void)0)
#define __BKPT(value)  ((void)0)
__STATIC_FORCEINLINE void __ISB(void) { }
__STATIC_FORCEINLINE void __DSB(void) { }
__STATIC_FORCEINLINE void __DMB(void) { }

// Bit manipulation, computed on the host.
__STATIC_FORCEINLINE uint32_t __REV(uint32_t value) { return __builtin_bswap32(value); }
__STATIC_FORCEINLINE uint32_t __REV16(uint32_t value) { return ((value & 0xFF00FF00u) >> 8) | ((value & 0x00FF00FFu) << 8); }
__STATIC_FORCEINLINE int16_t __REVSH(int16_t value) { return (int16_t)__builtin_bswap16((uint16_t)value); }
__STATIC_FORCEINLINE uint32_t __ROR(uint32_t op1, uint32_t op2)
{
  op2 %= 32u;
  return op2 ? (op1 >> op2) | (op1 << (32u - op2)) : op1;
}
__STATIC_FORCEINLINE uint32_t __RBIT(uint32_t value)
{
  uint32_t result = 0;
  for (int i = 0; i < 32; i++, value >>= 1)
    result = (result << 1) | (value & 1u);
  return result;
}
__STATIC_FORCEINLINE uint8_t __CLZ(uint32_t value) { return value ? (uint8_t)__builtin_clz(value) : 32u; }

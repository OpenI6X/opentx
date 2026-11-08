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
#include "board.h"

void eepromWaitEepromStandbyState(void);

#define I2C_TIMEOUT             1000U

#define EEPROM_READ_CHUNK         0xFFU

// Waits until one of `flags` is set. Returns false on NACK or timeout.
__attribute__((noinline))
static bool i2cWaitFlags(uint32_t flags)
{
  uint32_t timeout = I2C_TIMEOUT;
  while (timeout--) {
    uint32_t isr = I2C->ISR;
    if (isr & I2C_ISR_NACKF) {
      break;
    }
    if (isr & flags) {
      return true;
    }
  }
  return false;
}

void i2cInit()
{
  // PB10 = SCL, PB11 = SDA: alternate function 1, open-drain, medium speed,
  // internal pull-ups. Only the nibbles/mode bits of these pins are touched.
  GPIOB->AFR[1]  = (GPIOB->AFR[1]  & ~0x0000FF00U) | 0x00001100U;
  GPIOB->MODER   = (GPIOB->MODER   & ~0x00F00000U) | 0x00A00000U;
  GPIOB->OTYPER |= 0x00000C00U;
  GPIOB->OSPEEDR = (GPIOB->OSPEEDR & ~0x00F00000U) | 0x00500000U;
  GPIOB->PUPDR   = (GPIOB->PUPDR   & ~0x00F00000U) | 0x00500000U;

  // PE = 0 resets the I2C state machine and aborts any transfer left over
  // from a timeout. TIMINGR may only be written while PE = 0.
  I2C->CR1     = 0;
  I2C->TIMINGR = I2C_TIMING;
  I2C->ICR     = I2C_ICR_STOPCF | I2C_ICR_NACKCF | I2C_ICR_BERRCF |
                 I2C_ICR_ARLOCF | I2C_ICR_OVRCF;
  I2C->CR1     = I2C_CR1_PE | I2C_CR1_ANFOFF; // analog filter off, matches I2C_TIMING
}

static bool i2cSendAddress(uint16_t address, bool read)
{
  I2C->CR2 = I2C_ADDRESS_EEPROM | (2U << I2C_CR2_NBYTES_Pos) |
             (read ? 0U : I2C_CR2_RELOAD) | I2C_CR2_START;
  if (!i2cWaitFlags(I2C_ISR_TXIS)) {
    return false;
  }
  I2C->TXDR = (uint8_t)(address >> 8);
  if (!i2cWaitFlags(I2C_ISR_TXIS)) {
    return false;
  }
  I2C->TXDR = (uint8_t)address;
  return i2cWaitFlags(read ? I2C_ISR_TC : I2C_ISR_TCR);
}

/**
  * @brief  Transfers one chunk (1..255 bytes) to or from the EEPROM.
  * @param  buffer : data buffer
  * @param  address : EEPROM's internal address
  * @param  size : number of bytes, 1..255
  * @param  read : true = read, false = write
  * @retval true on success, false on NACK/timeout (bus needs re-init)
  */
__attribute__((noinline))
static bool i2cTransfer(uint8_t* buffer, uint16_t address, uint32_t size, bool read)
{
  // Never start a transfer while the bus is still busy; a STOP from the
  // previous transfer clears BUSY, so this only trips when something went
  // wrong or the slave is stretching.
  uint32_t timeout = I2C_TIMEOUT;
  while ((I2C->ISR & I2C_ISR_BUSY) != 0) {
    if (timeout-- == 0) {
      return false;
    }
  }

  if (!i2cSendAddress(address, read)) {
    return false;
  }

  if (read) {
    // Repeated start, turn the bus round, and let AUTOEND close it.
    I2C->CR2 = I2C_ADDRESS_EEPROM | I2C_CR2_RD_WRN | I2C_CR2_START |
               ((uint32_t)size << I2C_CR2_NBYTES_Pos) | I2C_CR2_AUTOEND;
    while (size) {
      if (!i2cWaitFlags(I2C_ISR_RXNE)) {
        return false;
      }
      *buffer++ = (uint8_t)I2C->RXDR;
      size--;
    }
  }
  else {
    // Continue in the same direction, no restart.
    I2C->CR2 = I2C_ADDRESS_EEPROM | ((uint32_t)size << I2C_CR2_NBYTES_Pos) | I2C_CR2_AUTOEND;
    while (size) {
      if (!i2cWaitFlags(I2C_ISR_TXIS)) {
        return false;
      }
      I2C->TXDR = *buffer++;
      size--;
    }
  }

  if (!i2cWaitFlags(I2C_ISR_STOPF)) {
    return false;
  }
  I2C->ICR = I2C_ICR_STOPCF;
  return true;
}

#if !defined(SIMU)  // the simulator's emulated EEPROM (targets/simu/simueeprom.cpp) provides these
void eepromReadBlock(uint8_t* buffer, size_t address, size_t size)
{
  while (size) {
    uint32_t chunk = (size > EEPROM_READ_CHUNK) ? EEPROM_READ_CHUNK : (uint32_t)size;

    while (!i2cTransfer(buffer, (uint16_t)address, chunk, true)) {
      i2cInit();
    }
    buffer += chunk;
    address += chunk;
    size -= chunk;
  }
}

/**
  * @brief  Writes a buffer to the EEPROM, splitting it on page boundaries.
  * @param  buffer : pointer to the data
  * @param  address : EEPROM's internal address to write to
  * @param  size : number of bytes to write
  * @retval None
  */
void eepromWriteBlock(uint8_t* buffer, size_t address, size_t size)
{
  while (size) {
    uint32_t count = I2C_FLASH_PAGESIZE - (address % I2C_FLASH_PAGESIZE);
    if (count > size) {
      count = (uint32_t)size;
    }

    while (!i2cTransfer(buffer, (uint16_t)address, count, false)) {
      i2cInit();
    }

    eepromWaitEepromStandbyState();
    address += count;
    buffer += count;
    size -= count;
  }
}

uint8_t eepromIsTransferComplete()
{
  return 1;
}
#endif // !defined(SIMU)

/**
  * @brief  Wait for the EEPROM's internal write cycle to finish.
  *
  * The default is the datasheet's 5 ms tWR (smallest code). Define
  * I2C_PROPER_WAIT to poll ACK instead: that is faster but bigger.
  */
//#define I2C_PROPER_WAIT // +56B
#if defined(I2C_PROPER_WAIT)
static bool i2cWaitIdle()
{
  uint32_t timeout = I2C_TIMEOUT;
  while (I2C->ISR & I2C_ISR_BUSY) {
    if (timeout-- == 0) {
      return false;
    }
  }
  return true;
}

static bool i2cWaitReady()
{
  for (uint32_t retry = 0; retry < 100; retry++) {
    if (!i2cWaitIdle()) {
      return false;
    }
    // A NACK while the EEPROM is busy is expected; both flags are cleared
    // before every attempt so a stale STOPF can not fake a ready EEPROM.
    I2C->ICR = I2C_ICR_NACKCF | I2C_ICR_STOPCF;
    I2C->CR2 = I2C_ADDRESS_EEPROM | I2C_CR2_AUTOEND | I2C_CR2_START;

    uint32_t timeout = I2C_TIMEOUT;
    while (!(I2C->ISR & (I2C_ISR_STOPF | I2C_ISR_NACKF))) {
      if (timeout-- == 0) {
        return false;
      }
    }

    bool nack = (I2C->ISR & I2C_ISR_NACKF) != 0;
    I2C->ICR = I2C_ICR_NACKCF | I2C_ICR_STOPCF;
    if (!nack && i2cWaitIdle()) {
      return true;
    }
  }

  return false;
}
#endif

void eepromWaitEepromStandbyState()
{
#if defined(I2C_PROPER_WAIT)
  while (!i2cWaitReady()) {
    i2cInit();
  }
#else
  delay_ms(5);
#endif
}

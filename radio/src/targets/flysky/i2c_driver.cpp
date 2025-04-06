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

void eepromPageWrite(uint8_t* pBuffer, uint16_t WriteAddr, uint8_t NumByteToWrite);
void eepromWaitEepromStandbyState(void);

void i2cInit()
{
  LL_I2C_DeInit(I2C1);

  LL_I2C_InitTypeDef I2C_InitStruct = {0};
  I2C_InitStruct.PeripheralMode = LL_I2C_MODE_I2C;
  I2C_InitStruct.Timing = I2C_TIMING;
  I2C_InitStruct.OwnAddress1 = 0x00;
  I2C_InitStruct.TypeAcknowledge = LL_I2C_ACK;
  I2C_InitStruct.OwnAddrSize = LL_I2C_OWNADDRESS1_7BIT;
  I2C_InitStruct.AnalogFilter = LL_I2C_ANALOGFILTER_DISABLE;
  I2C_InitStruct.DigitalFilter = 0x00;
  LL_I2C_Init(I2C1, &I2C_InitStruct);
  LL_I2C_Enable(I2C1);

  LL_GPIO_SetAFPin_8_15(I2C_GPIO, I2C_SCL_GPIO_PIN, I2C_GPIO_AF);
  LL_GPIO_SetAFPin_8_15(I2C_GPIO, I2C_SDA_GPIO_PIN, I2C_GPIO_AF);

  LL_GPIO_InitTypeDef GPIO_InitStruct = {0};
  GPIO_InitStruct.Pin        = I2C_SCL_GPIO_PIN | I2C_SDA_GPIO_PIN;
  GPIO_InitStruct.Mode       = LL_GPIO_MODE_ALTERNATE;
  GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_OPENDRAIN;
  GPIO_InitStruct.Pull       = LL_GPIO_PULL_UP;
  GPIO_InitStruct.Speed      = LL_GPIO_SPEED_FREQ_HIGH;
  LL_GPIO_Init(I2C_GPIO, &GPIO_InitStruct);
}

uint32_t I2C_GetFlagStatus(const I2C_TypeDef *I2Cx, uint32_t flag)
{
  switch(flag) {
    case I2C_ISR_TXIS:
      return LL_I2C_IsActiveFlag_TXIS(I2Cx);
    case I2C_ISR_TC:
      return LL_I2C_IsActiveFlag_TC(I2Cx);
    case I2C_ISR_TCR:
      return LL_I2C_IsActiveFlag_TCR(I2Cx);
    case I2C_ISR_RXNE:
      return LL_I2C_IsActiveFlag_RXNE(I2Cx);
    case I2C_ISR_STOPF:
      return LL_I2C_IsActiveFlag_STOP(I2Cx);
  }
  return 0;
}

#define I2C_TIMEOUT_MAX 1000
bool I2C_WaitEvent(uint32_t flag)
{
  uint32_t timeout = I2C_TIMEOUT_MAX;
  while (!I2C_GetFlagStatus(I2C, flag)) {
    if ((timeout--) == 0) return false;
  }
  return true;
}

bool I2C_WaitEventCleared(uint32_t flag)
{
  uint32_t timeout = I2C_TIMEOUT_MAX;
  while (I2C_GetFlagStatus(I2C, flag)) {
    if ((timeout--) == 0) return false;
  }
  return true;
}

/**
  * @brief  Reads a block of data from the EEPROM.
  * @param  pBuffer : pointer to the buffer that receives the data read
  *   from the EEPROM.
  * @param  ReadAddr : EEPROM's internal address to read from.
  * @param  NumByteToRead : number of bytes to read from the EEPROM.
  * @retval None
  */
bool I2C_EE_ReadBlock(uint8_t* pBuffer, uint16_t ReadAddr, uint16_t NumByteToRead)
{
  if (!I2C_WaitEventCleared(I2C_FLAG_BUSY))
    return false;

  LL_I2C_HandleTransfer(I2C, I2C_ADDRESS_EEPROM, LL_I2C_ADDRSLAVE_7BIT, 2, LL_I2C_MODE_SOFTEND, LL_I2C_GENERATE_START_WRITE);
  if (!I2C_WaitEvent(I2C_ISR_TXIS))
    return false;

  LL_I2C_TransmitData8(I2C, (uint8_t)((ReadAddr & 0xFF00) >> 8));
  if (!I2C_WaitEvent(I2C_ISR_TXIS))
    return false;

  LL_I2C_TransmitData8(I2C, (uint8_t)(ReadAddr & 0x00FF));
  if (!I2C_WaitEvent(I2C_ISR_TC))
    return false;

  LL_I2C_HandleTransfer(I2C, I2C_ADDRESS_EEPROM, LL_I2C_ADDRSLAVE_7BIT, NumByteToRead, LL_I2C_MODE_AUTOEND, LL_I2C_GENERATE_START_READ);

  while (NumByteToRead) {
    if (!I2C_WaitEvent(I2C_ISR_RXNE))
      return false;

    *pBuffer++ = LL_I2C_ReceiveData8(I2C);
    NumByteToRead--;
  }

  if (!I2C_WaitEvent(I2C_ISR_STOPF))
    return false;

  return true;
}

void eepromReadBlock(uint8_t * buffer, size_t address, size_t size)
{
  const uint8_t maxSize = 255; // LL_I2C_HandleTransfer can handle up to 255 bytes at once
  uint32_t offset = 0;
  while (size > maxSize) {
    size -= maxSize;
    while (!I2C_EE_ReadBlock(buffer + offset, address + offset, maxSize)) {
      i2cInit();
    }
    offset += maxSize;
  }
  if (size) {
    while (!I2C_EE_ReadBlock(buffer + offset, address + offset, size)) {
      i2cInit();
    }
  }
}

/**
  * @brief  Writes buffer of data to the I2C EEPROM.
  * @param  buffer : pointer to the buffer containing the data to be
  *   written to the EEPROM.
  * @param  address : EEPROM's internal address to write to.
  * @param  size : number of bytes to write to the EEPROM.
  * @retval None
  */
void eepromWriteBlock(uint8_t * buffer, size_t address, size_t size)
{
  uint8_t offset = address % I2C_FLASH_PAGESIZE;
  uint8_t count = I2C_FLASH_PAGESIZE - offset;
  if (size < count) {
    count = size;
  }
  while (count > 0) {
    eepromPageWrite(buffer, address, count);
    eepromWaitEepromStandbyState();
    address += count;
    buffer += count;
    size -= count;
    count = I2C_FLASH_PAGESIZE;
    if (size < I2C_FLASH_PAGESIZE) {
      count = size;
    }
  }
}

uint8_t eepromIsTransferComplete()
{
  return 1;
}

/**
  * @brief  Writes more than one byte to the EEPROM with a single WRITE cycle.
  * @note   The number of byte can't exceed the EEPROM page size.
  * @param  pBuffer : pointer to the buffer containing the data to be
  *   written to the EEPROM.
  * @param  WriteAddr : EEPROM's internal address to write to.
  * @param  NumByteToWrite : number of bytes to write to the EEPROM.
  * @retval None
  */
bool I2C_EE_PageWrite(uint8_t* pBuffer, uint16_t WriteAddr, uint8_t NumByteToWrite)
{
  if (!I2C_WaitEventCleared(I2C_ISR_BUSY))
    return false;

  LL_I2C_HandleTransfer(I2C, I2C_ADDRESS_EEPROM, LL_I2C_ADDRSLAVE_7BIT, 2, LL_I2C_MODE_RELOAD, LL_I2C_GENERATE_START_WRITE);

  if (!I2C_WaitEvent(I2C_ISR_TXIS))
    return false;

  LL_I2C_TransmitData8(I2C, (uint8_t)((WriteAddr & 0xFF00) >> 8));
  if (!I2C_WaitEvent(I2C_ISR_TXIS))
    return false;

  LL_I2C_TransmitData8(I2C, (uint8_t)(WriteAddr & 0x00FF));
  if (!I2C_WaitEvent(I2C_ISR_TCR))
    return false;

  LL_I2C_HandleTransfer(I2C, I2C_ADDRESS_EEPROM, LL_I2C_ADDRSLAVE_7BIT, NumByteToWrite, LL_I2C_MODE_AUTOEND, LL_I2C_GENERATE_NOSTARTSTOP);

  /* While there is data to be written */
  while (NumByteToWrite--) {
    if (!I2C_WaitEvent(I2C_ISR_TXIS))
      return false;

    LL_I2C_TransmitData8(I2C, *pBuffer);
    pBuffer++;
  }

  if (!I2C_WaitEvent(I2C_ISR_STOPF))
    return false;

  return true;
}

void eepromPageWrite(uint8_t* pBuffer, uint16_t WriteAddr, uint8_t NumByteToWrite)
{
  while (!I2C_EE_PageWrite(pBuffer, WriteAddr, NumByteToWrite)) {
    i2cInit();
  }
}

/**
  * @brief  Wait for EEPROM Standby state
  * @param  None
  * @retval None
  */
// #define I2C_PROPER_WAIT // +128B
#define I2C_STANDBY_WAIT_MAX 100
bool I2C_EE_WaitEepromStandbyState(void)
{
#if defined(I2C_PROPER_WAIT)
  __IO uint32_t trials = 0;
  I2C_TransferHandling(I2C, I2C_ADDRESS_EEPROM, 0, I2C_AutoEnd_Mode, I2C_No_StartStop);
  do {
    I2C_ClearFlag(I2C, I2C_ICR_NACKCF | I2C_ICR_STOPCF);
    I2C_GenerateSTART(I2C, ENABLE);
    delay_ms(1);
    if (trials++ == I2C_STANDBY_WAIT_MAX) {
      return false;
    }
  } while (I2C_GetFlagStatus(I2C, I2C_ISR_NACKF) != RESET);

  I2C_ClearFlag(I2C, I2C_ISR_STOPF);
#else
  delay_ms(5);
#endif
  return true;
}

void eepromWaitEepromStandbyState(void)
{
  while (!I2C_EE_WaitEepromStandbyState()) {
    i2cInit();
  }
}

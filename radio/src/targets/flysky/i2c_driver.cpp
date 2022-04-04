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
  I2C_DeInit(I2C);

  I2C_InitTypeDef I2C_InitStructure;
  I2C_InitStructure.I2C_Timing = I2C_TIMING;
  I2C_InitStructure.I2C_OwnAddress1 = 0x00;
  I2C_InitStructure.I2C_Mode = I2C_Mode_I2C;
  I2C_InitStructure.I2C_Ack = I2C_Ack_Enable;
  I2C_InitStructure.I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit;
  I2C_InitStructure.I2C_AnalogFilter = I2C_AnalogFilter_Disable;
  I2C_InitStructure.I2C_DigitalFilter = 0x00;
  I2C_Init(I2C, &I2C_InitStructure);
  I2C_Cmd(I2C, ENABLE);

  GPIO_PinAFConfig(I2C_GPIO, I2C_SCL_GPIO_PinSource, I2C_GPIO_AF);
  GPIO_PinAFConfig(I2C_GPIO, I2C_SDA_GPIO_PinSource, I2C_GPIO_AF);

  GPIO_InitTypeDef GPIO_InitStructure;
  GPIO_InitStructure.GPIO_Pin = I2C_SCL_GPIO_PIN | I2C_SDA_GPIO_PIN;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_OD;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
  GPIO_Init(I2C_GPIO, &GPIO_InitStructure);
}

#define I2C_TIMEOUT_MAX 1000
bool I2C_WaitEvent(uint32_t event)
{
  uint32_t timeout = I2C_TIMEOUT_MAX;
  while (!I2C_GetFlagStatus(I2C, event)) {
    if ((timeout--) == 0) return false;
  }
  return true;
}

bool I2C_WaitEventCleared(uint32_t event)
{
  uint32_t timeout = I2C_TIMEOUT_MAX;
  while (I2C_GetFlagStatus(I2C, event)) {
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

  I2C_TransferHandling(I2C, I2C_ADDRESS_EEPROM, 2, I2C_SoftEnd_Mode, I2C_Generate_Start_Write);
  if (!I2C_WaitEvent(I2C_FLAG_TXIS))
    return false;

  I2C_SendData(I2C, (uint8_t)((ReadAddr & 0xFF00) >> 8));
  if (!I2C_WaitEvent(I2C_FLAG_TXIS))
    return false;

  I2C_SendData(I2C, (uint8_t)(ReadAddr & 0x00FF));
  if (!I2C_WaitEvent(I2C_FLAG_TC))
    return false;

  I2C_TransferHandling(I2C, I2C_ADDRESS_EEPROM, NumByteToRead,  I2C_AutoEnd_Mode, I2C_Generate_Start_Read);

  while (NumByteToRead) {
    if (!I2C_WaitEvent(I2C_FLAG_RXNE))
      return false;

    *pBuffer++ = I2C_ReceiveData(I2C);
    NumByteToRead--;
  }

  if (!I2C_WaitEvent(I2C_FLAG_STOPF))
    return false;

  return true;
}

void eepromReadBlock(uint8_t * buffer, size_t address, size_t size)
{
  // I2C_TransferHandling can handle up to 255 bytes at once
  const uint8_t maxSize = 255;
  uint8_t round = 0;
  while (size > maxSize) {
    size -= maxSize;
    while (!I2C_EE_ReadBlock(buffer + (round * maxSize), address, maxSize)) {
      i2cInit();
    }
    round++;
  }
  if (size) {
    while (!I2C_EE_ReadBlock(buffer + (round * maxSize), address, size)) {
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
  if (!I2C_WaitEventCleared(I2C_FLAG_BUSY))
    return false;

  I2C_TransferHandling(I2C, I2C_ADDRESS_EEPROM, 2, I2C_Reload_Mode, I2C_Generate_Start_Write);
  if (!I2C_WaitEvent(I2C_FLAG_TXIS))
    return false;

  I2C_SendData(I2C, (uint8_t)((WriteAddr & 0xFF00) >> 8));
  if (!I2C_WaitEvent(I2C_FLAG_TXIS))
    return false;

  I2C_SendData(I2C, (uint8_t)(WriteAddr & 0x00FF));
  if (!I2C_WaitEvent(I2C_FLAG_TCR))
    return false;

  I2C_TransferHandling(I2C, I2C_ADDRESS_EEPROM, NumByteToWrite, I2C_AutoEnd_Mode, I2C_No_StartStop);

  /* While there is data to be written */
  while (NumByteToWrite--) {
    if (!I2C_WaitEvent(I2C_FLAG_TXIS))
      return false;

    I2C_SendData(I2C, *pBuffer);
    pBuffer++;
  }

  if (!I2C_WaitEvent(I2C_FLAG_STOPF))
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
bool I2C_EE_WaitEepromStandbyState(void)
{
  RTOS_WAIT_MS(5);
  return true;
}

void eepromWaitEepromStandbyState(void)
{
  while (!I2C_EE_WaitEepromStandbyState()) {
    i2cInit();
  }
}

void i2c_test() {
  static uint8_t temp[128];
  uint8_t i;
  for (i = 0; i < 128; i++) {
    temp[i] = i;
  }
  DUMP(temp, 128);
  eepromWriteBlock(temp, 10, 128);
  memset(temp, 0, 128);
  eepromReadBlock(temp, 10, 128);
  DUMP(temp, 128);
}

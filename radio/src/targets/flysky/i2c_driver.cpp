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

#define I2C_TIMEOUT_MAX 2000

void i2cInit()
{
  TRACE("i2cInit");
  I2C_DeInit(I2C);

 GPIO_InitTypeDef GPIO_InitStructure;
//  GPIO_InitStructure.GPIO_Pin = I2C_WP_GPIO_PIN;
//  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
//  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
//  GPIO_InitStructure.GPIO_OType = GPIO_OType_OD;
//  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
//  GPIO_Init(I2C_WP_GPIO, &GPIO_InitStructure);
//  GPIO_ResetBits(I2C_WP_GPIO, I2C_WP_GPIO_PIN);

  I2C_InitTypeDef I2C_InitStructure;
  I2C_InitStructure.I2C_Timing = 0x50330309;//I2C_TIMING_400K;
  // I2C_InitStructure.I2C_ClockSpeed = I2C_SPEED;
  // I2C_InitStructure.I2C_DutyCycle = I2C_DutyCycle_2;
  I2C_InitStructure.I2C_OwnAddress1 = 0x00;
  I2C_InitStructure.I2C_Mode = I2C_Mode_I2C;
  I2C_InitStructure.I2C_Ack = I2C_Ack_Enable;
  I2C_InitStructure.I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit;
  I2C_InitStructure.I2C_AnalogFilter = I2C_AnalogFilter_Enable;
  I2C_InitStructure.I2C_DigitalFilter = 0x00;
  I2C_Init(I2C, &I2C_InitStructure);
  I2C_Cmd(I2C, ENABLE);

  GPIO_PinAFConfig(I2C_GPIO, I2C_SCL_GPIO_PinSource, I2C_GPIO_AF);
  GPIO_PinAFConfig(I2C_GPIO, I2C_SDA_GPIO_PinSource, I2C_GPIO_AF);

  GPIO_InitStructure.GPIO_Pin = I2C_SCL_GPIO_PIN | I2C_SDA_GPIO_PIN;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_OD;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
  GPIO_Init(I2C_GPIO, &GPIO_InitStructure);
}

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

//I2C_Status I2C1_Read_NBytes(uint8_t driver_Addr(I2C_ADDRESS_EEPROM), uint8_t start_Addr, uint8_t number_Bytes, uint8_t *read_Buffer)

// #define I2C_FAIL 0
// #define I2C_OK 1

static int I2C_Timeout = 0;

bool I2C_EE_ReadBlock(uint8_t* pBuffer, uint16_t ReadAddr, uint16_t NumByteToRead)
{
  
 uint16_t read_Num;
 
  I2C_Timeout = I2C_TIMEOUT_MAX;
  while(I2C_GetFlagStatus(I2C, I2C_FLAG_BUSY) != RESET)
  {
    if((I2C_Timeout--) == 0)
    {
      TRACE("I2C_EE_ReadBlock I2C_FLAG_BUSY");
      return false;
    }
  }
 
  I2C_TransferHandling(I2C, I2C_ADDRESS_EEPROM, 2, I2C_SoftEnd_Mode, I2C_Generate_Start_Write);
 
  I2C_Timeout = I2C_TIMEOUT_MAX;
  while(I2C_GetFlagStatus(I2C, I2C_FLAG_TXIS) == RESET)
  {
    if((I2C_Timeout--) == 0)
    {
      TRACE("I2C_EE_ReadBlock I2C_FLAG_TXIS");
      return false;
    }
  }

  I2C_SendData(I2C, (uint8_t)((ReadAddr & 0xFF00) >> 8));

  I2C_Timeout = I2C_TIMEOUT_MAX;
  while(I2C_GetFlagStatus(I2C, I2C_FLAG_TXIS) == RESET)
  {
    if((I2C_Timeout--) == 0)
    {
      TRACE("I2C_EE_ReadBlock I2C_FLAG_TXIS");
      return false;
    }
  }

  I2C_SendData(I2C, (uint8_t)(ReadAddr & 0x00FF));

  I2C_Timeout = I2C_TIMEOUT_MAX;
  while(I2C_GetFlagStatus(I2C, I2C_FLAG_TC) == RESET)
  {
    if((I2C_Timeout--) == 0)
    {
      TRACE("I2C_EE_ReadBlock I2C_FLAG_TC");
      return false;
    }
  }
 
  I2C_TransferHandling(I2C, I2C_ADDRESS_EEPROM, NumByteToRead,  I2C_AutoEnd_Mode, I2C_Generate_Start_Read);
 
  for(read_Num = 0; read_Num < NumByteToRead; read_Num++)
  {
    I2C_Timeout = I2C_TIMEOUT_MAX;
    while(I2C_GetFlagStatus(I2C, I2C_FLAG_RXNE) == RESET)
    {
      if((I2C_Timeout--) == 0)
      {
        TRACE("I2C_EE_ReadBlock I2C_FLAG_RXNE");
        return false;
      }
    }
 
    pBuffer[read_Num] = I2C_ReceiveData(I2C);
  }
 
  I2C_Timeout = I2C_TIMEOUT_MAX;
  while(I2C_GetFlagStatus(I2C, I2C_FLAG_STOPF) == RESET)
  {
    if((I2C_Timeout--) == 0)
    {
      TRACE("I2C_EE_ReadBlock I2C_FLAG_STOPF");
      return false;
    }
  }
 
  return true;
}

void eepromReadBlock(uint8_t * buffer, size_t address, size_t size)
{
  TRACE("eepromReadBlock");
  while (!I2C_EE_ReadBlock(buffer, address, size)) {
    i2cInit();
  }
}

void eepromPageWrite(uint8_t *buffer, uint16_t address, uint8_t size) {
//   static uint8_t temp[2 + I2C_FLASH_PAGESIZE];
//   temp[0] = (uint8_t)(address >> 8);
//   temp[1] = (uint8_t)(address & 0xFF);
//   // uint8_t start_page = address / 64;
//   // uint8_t end_page = (address + size - 1) / 64;
//   // TRACE("eepromPageWrite addr %d size %d [start page %d end page %d]", address, size, start_page, end_page);
//   memcpy(temp + 2, buffer, size);
//   //DUMP(temp, size + 2);
//   i2c_transfer7(I2C2, I2C_ADDRESS_EEPROM, temp, size + 2, NULL, 0);

// #ifdef RTOS_WAIT_MS
//   RTOS_WAIT_MS(5);
// #else
//   delay_ms(5);
// #endif

// #if defined(EEPROM_VERIFY_WRITES)
//   eepromPageRead(temp, address, size);
//   for (int i = 0; i < size; i++) {
//     if (temp[i] != buffer[i]) {
//       TRACE("--------- eeprom verify failed  ----------");
//       while (1)
//         ;
//     }
//   }

// #endif
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
  // uint8_t offset = address % I2C_FLASH_PAGESIZE;
  // uint8_t count = I2C_FLASH_PAGESIZE - offset;
  // if (size < count) {
  //   count = size;
  // }
  // while (count > 0) {
  //   eepromPageWrite(buffer, address, count);
  //   eepromWaitEepromStandbyState(); // TODO
  //   address += count;
  //   buffer += count;
  //   size -= count;
  //   count = I2C_FLASH_PAGESIZE;
  //   if (size < I2C_FLASH_PAGESIZE) {
  //     count = size;
  //   }
  // }
}

uint8_t eepromIsTransferComplete() {
  return 1;
}

void i2c_test() {
  static uint8_t temp[255];
  // uint8_t i;
  // for (i = 0; i < 128; i++) {
  //   temp[i] = i;
  // }
  // DUMP(temp, 128);
  // eepromWriteBlock(temp, 10, 128);
  // memset(temp, 0, 128);
  eepromReadBlock(temp, 10, 255);
  DUMP(temp, 255);
}
